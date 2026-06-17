// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/template_element.h"

#include <functional>
#include <future>
#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/value_utils.h"

namespace lynx {
namespace tasm {
namespace {

// These keys define the serialized Element Template payload consumed by
// FiberSerializeElementTemplate / FiberCreateElementTemplate.
static constexpr const char kTemplateTag[] = "template";
static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateKey[] = "templateKey";
static constexpr const char kTemplateTypedTag[] = "tag";
static constexpr const char kTemplateAttributes[] = "attributes";
static constexpr const char kTemplateBundleUrl[] = "bundleUrl";
static constexpr const char kTemplateAttributeSlots[] = "attributeSlots";
static constexpr const char kTemplateElementSlots[] = "elementSlots";
static constexpr const char kTemplateOptions[] = "options";
static constexpr const char kTemplateUid[] = "uid";
static constexpr const char kTemplateRootAttributeSpread[] = "rootAttributes";
static constexpr const char kDefaultPageComponentId[] = "0";
static constexpr int32_t kDefaultPageCSSId = 0;
static constexpr uint32_t kTypedTemplateRootSlotIndex = 0;

fml::RefPtr<FiberElement> ResolveInitialElementSlotChild(
    const lepus::Value& child) {
  if (!child.IsRefCounted()) {
    return nullptr;
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    return nullptr;
  }

  return fml::static_ref_ptr_cast<FiberElement>(ref_counted);
}

fml::RefPtr<TemplateElement> ResolveTemplateElementSlotChild(
    const lepus::Value& child) {
  auto element = ResolveInitialElementSlotChild(child);
  if (element == nullptr || !element->is_template()) {
    return nullptr;
  }
  return fml::static_ref_ptr_cast<TemplateElement>(element);
}

base::Vector<fml::RefPtr<TemplateElement>> CollectTemplateElementSlotChildren(
    const lepus::Value& slot_children) {
  base::Vector<fml::RefPtr<TemplateElement>> children;
  if (!slot_children.IsArrayOrJSArray()) {
    return children;
  }
  for (size_t index = 0; index < static_cast<size_t>(slot_children.GetLength());
       ++index) {
    auto child = ResolveTemplateElementSlotChild(
        slot_children.GetProperty(static_cast<uint32_t>(index)));
    if (child != nullptr) {
      children.push_back(std::move(child));
    }
  }
  return children;
}

void RemoveElementFromSlotChildren(lepus::Value* slot_children,
                                   FiberElement* child) {
  if (slot_children == nullptr || child == nullptr ||
      !slot_children->IsArray()) {
    return;
  }
  auto array = slot_children->Array();
  for (size_t i = 0; i < array->size();) {
    auto slot_child = ResolveInitialElementSlotChild(array->get(i));
    if (slot_child != nullptr && slot_child.get() == child) {
      array->Erase(static_cast<uint32_t>(i));
      continue;
    }
    ++i;
  }
}

size_t FindSlotChildIndex(const lepus::Value& slot_children,
                          FiberElement* child) {
  if (child == nullptr || !slot_children.IsArray()) {
    return static_cast<size_t>(slot_children.GetLength());
  }
  for (size_t i = 0; i < static_cast<size_t>(slot_children.GetLength()); ++i) {
    auto slot_child = ResolveInitialElementSlotChild(
        slot_children.GetProperty(static_cast<uint32_t>(i)));
    if (slot_child != nullptr && slot_child.get() == child) {
      return i;
    }
  }
  return static_cast<size_t>(slot_children.GetLength());
}

void EnsureMutableArrayForWrite(lepus::Value* value) {
  if (value == nullptr || !value->IsArray()) {
    return;
  }
  auto array = value->Array();
  if (array != nullptr && array->IsConst()) {
    *value = lepus::Value::ShallowCopy(*value);
  }
}

lepus::Value CopyArrayHeader(const lepus::Value& value) {
  if (!value.IsArrayOrJSArray()) {
    return value;
  }

  auto array = lepus::CArray::Create();
  array->reserve(value.GetLength());
  for (size_t i = 0; i < static_cast<size_t>(value.GetLength()); ++i) {
    array->emplace_back(value.GetProperty(static_cast<uint32_t>(i)));
  }
  return lepus::Value(std::move(array));
}

lepus::Value CopyTemplateValueForStorage(const lepus::Value& value) {
  // Clone would deep-convert JS functions to empty values. Keep callables by
  // reference, while preserving snapshot semantics for ordinary attributes.
  return value.IsCallable() ? value : lepus::Value::Clone(value);
}

lepus::Value CopyTemplateObjectForStorage(const lepus::Value& value) {
  if (!value.IsObject()) {
    return lepus::Value();
  }

  auto object = lepus::Dictionary::Create();
  lepus::Value::ForEachLepusValue(
      value, [&object](const lepus::Value& key, const lepus::Value& item) {
        if (!key.IsString()) {
          return;
        }
        object->SetValue(key.String(), CopyTemplateValueForStorage(item));
      });
  return lepus::Value(std::move(object));
}

lepus::Value CopyAttributeSlotsForStorage(const lepus::Value& attribute_slots) {
  if (!attribute_slots.IsArrayOrJSArray()) {
    return lepus::Value();
  }

  auto copied_slots = lepus::CArray::Create();
  copied_slots->reserve(attribute_slots.GetLength());
  for (size_t index = 0;
       index < static_cast<size_t>(attribute_slots.GetLength()); ++index) {
    auto slot = attribute_slots.GetProperty(static_cast<uint32_t>(index));
    copied_slots->emplace_back(slot.IsObject()
                                   ? CopyTemplateObjectForStorage(slot)
                                   : CopyTemplateValueForStorage(slot));
  }
  return lepus::Value(std::move(copied_slots));
}

lepus::Value CreateRootAttributeSlots(const lepus::Value& root_attributes) {
  auto attribute_slots = lepus::CArray::Create();
  attribute_slots->emplace_back(
      root_attributes.IsObject() ? CopyTemplateObjectForStorage(root_attributes)
                                 : lepus::Value());
  return lepus::Value(std::move(attribute_slots));
}

SharedTemplateAttributes CreateRootSpreadTemplateAttributes();

template <typename Apply>
void ApplyRootTemplateAttributes(FiberElement* root, Apply&& apply) {
  if (root == nullptr) {
    return;
  }
  auto compiled_attributes = root->template_attributes();
  root->SetTemplateAttributes(CreateRootSpreadTemplateAttributes());
  apply();
  root->SetTemplateAttributes(compiled_attributes);
}

void ApplyRootTemplateAttributes(FiberElement* root,
                                 const lepus::Value& previous_root_attributes,
                                 const lepus::Value& root_attributes) {
  if (!previous_root_attributes.IsObject() && !root_attributes.IsObject()) {
    return;
  }
  ApplyRootTemplateAttributes(
      root, [root, &previous_root_attributes, &root_attributes]() {
        if (previous_root_attributes.IsObject()) {
          TreeResolver::ApplyTemplateAttributesToElement(
              root, CreateRootAttributeSlots(previous_root_attributes),
              CreateRootAttributeSlots(root_attributes));
          return;
        }
        TreeResolver::ApplyTemplateAttributesToElement(
            root, CreateRootAttributeSlots(root_attributes));
      });
}

void ApplyRootTemplateNonEventAttributes(FiberElement* root,
                                         const lepus::Value& root_attributes) {
  if (!root_attributes.IsObject()) {
    return;
  }
  ApplyRootTemplateAttributes(root, [root, &root_attributes]() {
    TreeResolver::ApplyTemplateNonEventAttributesToElement(
        root, CreateRootAttributeSlots(root_attributes));
  });
}

void ApplyRootTemplateEventAttributes(FiberElement* root,
                                      const lepus::Value& root_attributes) {
  if (!root_attributes.IsObject()) {
    return;
  }
  ApplyRootTemplateAttributes(root, [root, &root_attributes]() {
    TreeResolver::ApplyTemplateEventAttributesToElement(
        root, CreateRootAttributeSlots(root_attributes));
  });
}

SharedTemplateAttributes CreateRootSpreadTemplateAttributes() {
  return std::make_shared<const TemplateAttributes>(TemplateAttributes{
      Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                BASE_STATIC_STRING(kTemplateRootAttributeSpread),
                lepus::Value(), kTypedTemplateRootSlotIndex}});
}

fml::RefPtr<FiberElement> CreateTypedRootElement(ElementManager* manager,
                                                 TemplateAssembler* tasm,
                                                 const base::String& tag) {
  if (tag.IsEqual(kElementPageTag)) {
    auto page = manager->CreateFiberPage(
        BASE_STATIC_STRING(kDefaultPageComponentId), kDefaultPageCSSId);
    if (tasm != nullptr) {
      page->set_style_sheet_manager(
          tasm->style_sheet_manager(tasm::DEFAULT_ENTRY_NAME));
    }
    return page;
  }
  if (tag.IsEqual(kElementListTag)) {
    return manager->CreateFiberList(tasm, tag, lepus::Value(), lepus::Value(),
                                    lepus::Value());
  }
  return manager->CreateFiberElement(tag);
}

template <typename Apply>
void ApplyInitialAttributeSlots(
    const base::Vector<fml::RefPtr<FiberElement>>& targets,
    const lepus::Value& attribute_slots, Apply apply) {
  FiberElement* previous_element = nullptr;
  for (const auto& target : targets) {
    auto* element = target.get();
    if (element == nullptr || element == previous_element) {
      continue;
    }
    apply(element, attribute_slots);
    previous_element = element;
  }
}

void ApplyInitialAttributeSlots(
    const base::Vector<fml::RefPtr<FiberElement>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](FiberElement* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateAttributesToElement(element, slots);
      });
}

void ApplyInitialNonEventAttributeSlots(
    const base::Vector<fml::RefPtr<FiberElement>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](FiberElement* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateNonEventAttributesToElement(element, slots);
      });
}

void ApplyInitialEventAttributeSlots(
    const base::Vector<fml::RefPtr<FiberElement>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](FiberElement* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateEventAttributesToElement(element, slots);
      });
}

void ApplyStaticEventAttributes(
    const base::Vector<fml::RefPtr<FiberElement>>& targets) {
  for (const auto& target : targets) {
    TreeResolver::ApplyStaticTemplateEventAttributesToElement(target.get());
  }
}

void PrepareGeneratedElementsResult(GeneratedElementsResult* generated,
                                    const lepus::Value& attribute_slots,
                                    const lepus::Value& root_attributes,
                                    uint32_t root_attributes_generation,
                                    const lepus::Value& element_slots) {
  if (generated == nullptr) {
    return;
  }

  ApplyInitialNonEventAttributeSlots(generated->attribute_slot_targets_,
                                     attribute_slots);
  ApplyRootTemplateNonEventAttributes(generated->result_.get(),
                                      root_attributes);
  generated->prepared_root_attributes_ = root_attributes;
  generated->root_attributes_generation_ = root_attributes_generation;

  if (!element_slots.IsArrayOrJSArray()) {
    return;
  }

  // Resolve slot children early, but defer insertion until GetRoot consumes the
  // prepared tree on the main render path.
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }

    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveInitialElementSlotChild(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child == nullptr) {
        continue;
      }
      PreparedElementSlotInsertion insertion;
      insertion.slot_index_ = static_cast<uint32_t>(slot_index);
      insertion.child_ = std::move(child);
      generated->prepared_element_slot_insertions_.push_back(
          std::move(insertion));
    }
  }
}

GeneratedElementsResult GeneratePreparedElementsResult(
    TemplateEntry* entry, const base::String& template_key,
    const lepus::Value& attribute_slots, const lepus::Value& root_attributes,
    uint32_t root_attributes_generation, const lepus::Value& element_slots) {
  GeneratedElementsResult generated;
  if (entry != nullptr) {
    auto& info = entry->GetElementTemplateInfo(template_key.str());
    generated = TreeResolver::GenerateElementsFromTemplateInfo(info);
  }
  PrepareGeneratedElementsResult(&generated, attribute_slots, root_attributes,
                                 root_attributes_generation, element_slots);
  return generated;
}

}  // namespace

TemplateElement::TemplateElement(ElementManager* element_manager)
    : FiberElement(element_manager, BASE_STATIC_STRING(kTemplateTag)),
      bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {
  MarkTemplateElement();
}

TemplateElement::~TemplateElement() = default;

void TemplateElement::SetTypedTag(const base::String& typed_tag) {
  typed_tag_ = typed_tag;
  if (IsPageTemplate()) {
    MarkInTemplateTreeAndPrepare();
  }
  if (IsInTemplateTree()) {
    MarkTemplateChildrenInElementSlotsInTree();
  }
}

void TemplateElement::SetRootAttributes(const lepus::Value& attributes) {
  if (!attributes.IsObject() && !attributes.IsNil() &&
      !attributes.IsUndefined()) {
    return;
  }
  auto previous_root_attributes = root_attributes_;
  root_attributes_ = attributes.IsObject()
                         ? CopyTemplateObjectForStorage(attributes)
                         : lepus::Value();
  ++root_attributes_generation_;
  ApplyRootAttributes(previous_root_attributes);
}

void TemplateElement::SetElementSlots(const lepus::Value& element_slots) {
  element_slots_ = element_slots;
  if (IsPageTemplate()) {
    MarkInTemplateTreeAndPrepare();
  }
  if (IsInTemplateTree()) {
    MarkTemplateChildrenInElementSlotsInTree();
  }
}

void TemplateElement::SetOptions(const lepus::Value& options) {
  options_ = options.IsObject() ? options.ToLepusValue() : lepus::Value();
}

bool TemplateElement::CanUseListItemTemplateTreeCache() const {
  return is_list_item() && !template_key_.str().empty();
}

bool TemplateElement::HasSameTemplateIdentity(
    const TemplateElement& other) const {
  if (IsTypedTemplate() || other.IsTypedTemplate()) {
    return IsTypedTemplate() && other.IsTypedTemplate() &&
           typed_tag_.IsEqual(other.typed_tag_);
  }
  return !template_key_.str().empty() &&
         template_key_.IsEqual(other.template_key_) &&
         bundle_url_.IsEqual(other.bundle_url_);
}

bool TemplateElement::TryPrepareCachedTemplateTree() {
  if (!CanUseListItemTemplateTreeCache()) {
    return false;
  }
  if (prepared_cached_template_tree_ != nullptr) {
    return true;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return false;
  }
  prepared_cached_template_tree_ =
      manager->TakeCachedTemplateElementTree(this, bundle_url_, template_key_);
  return prepared_cached_template_tree_ != nullptr;
}

bool TemplateElement::ActivateCachedTemplateTreeIfNeeded() {
  if (!IsInTemplateCache() || result_ == nullptr) {
    return false;
  }
  auto* manager = element_manager();
  if (manager != nullptr) {
    manager->RemoveCachedTemplateElementTreeForOwner(this);
  }
  MarkCachedTemplateTreeActiveRecursively();
  return true;
}

bool TemplateElement::MoveToTemplateTreeCacheIfNeeded() {
  if (IsInTemplateCache() || result_ == nullptr ||
      !CanUseListItemTemplateTreeCache()) {
    return false;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return false;
  }
  MarkCachedTemplateTreeInactiveRecursively();
  manager->CacheListItemTemplateElementTree(fml::RefPtr<TemplateElement>(this),
                                            bundle_url_, template_key_);
  return true;
}

void TemplateElement::MarkCachedTemplateTreeInactiveRecursively() {
  template_tree_state_ = TemplateElementTreeState::kInTemplateCache;
  async_create_task_ = nullptr;
  prepared_cached_template_tree_ = nullptr;
  if (!element_slots_.IsArrayOrJSArray()) {
    return;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    auto template_children = CollectTemplateElementSlotChildren(slot_children);
    for (const auto& child : template_children) {
      child->MarkCachedTemplateTreeInactiveRecursively();
    }
  }
}

void TemplateElement::MarkCachedTemplateTreeActiveRecursively() {
  template_tree_state_ = TemplateElementTreeState::kInTemplateTree;
  ApplyRootAttributes(lepus::Value());
  ApplyInitialAttributeSlots(attribute_slot_targets_, attribute_slots_);
  ApplyPendingOperations();
  if (!element_slots_.IsArrayOrJSArray()) {
    return;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    auto template_children = CollectTemplateElementSlotChildren(slot_children);
    for (const auto& child : template_children) {
      child->MarkCachedTemplateTreeActiveRecursively();
    }
  }
}

void TemplateElement::TransferCachedTemplateTreeFrom(TemplateElement* cached) {
  if (cached == nullptr || cached == this) {
    ActivateCachedTemplateTreeIfNeeded();
    return;
  }
  auto cached_element_slots = cached->element_slots_;
  result_ = std::move(cached->result_);
  attribute_slot_targets_ = std::move(cached->attribute_slot_targets_);
  event_attribute_slot_targets_ =
      std::move(cached->event_attribute_slot_targets_);
  static_event_targets_ = std::move(cached->static_event_targets_);
  element_slot_targets_ = std::move(cached->element_slot_targets_);
  prepared_element_slot_insertions_.clear();
  async_create_task_ = nullptr;
  prepared_cached_template_tree_ = nullptr;
  template_tree_state_ = TemplateElementTreeState::kInTemplateTree;
  cached->ClearCachedTemplateTreeShell();

  ApplyRootAttributes(lepus::Value());
  ApplyInitialAttributeSlots(attribute_slot_targets_, attribute_slots_);
  ReconcileElementSlotsFromCachedTree(cached_element_slots);
  ApplyPendingOperations();
}

void TemplateElement::ClearCachedTemplateTreeShell() {
  result_ = nullptr;
  attribute_slot_targets_.clear();
  event_attribute_slot_targets_.clear();
  static_event_targets_.clear();
  element_slot_targets_.clear();
  prepared_element_slot_insertions_.clear();
  async_create_task_ = nullptr;
  prepared_cached_template_tree_ = nullptr;
  template_tree_state_ = TemplateElementTreeState::kDetached;
}

void TemplateElement::ReleaseCachedTemplateTreeRecursively() {
  if (element_slots_.IsArrayOrJSArray()) {
    for (size_t slot_index = 0;
         slot_index < static_cast<size_t>(element_slots_.GetLength());
         ++slot_index) {
      auto slot_children =
          element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
      auto template_children =
          CollectTemplateElementSlotChildren(slot_children);
      for (const auto& child : template_children) {
        child->ReleaseCachedTemplateTreeRecursively();
      }
    }
  }
  if (result_ != nullptr && result_->parent() != nullptr) {
    auto* parent = static_cast<FiberElement*>(result_->parent());
    parent->RemoveNode(result_);
  }
  ClearCachedTemplateTreeShell();
}

void TemplateElement::ReconcileElementSlotsFromCachedTree(
    const lepus::Value& cached_element_slots) {
  const size_t slot_count =
      element_slots_.IsArrayOrJSArray()
          ? static_cast<size_t>(element_slots_.GetLength())
          : 0;
  base::Vector<base::Vector<fml::RefPtr<TemplateElement>>> cached_slots;
  if (cached_element_slots.IsArrayOrJSArray()) {
    cached_slots.reserve(cached_element_slots.GetLength());
    for (size_t slot_index = 0;
         slot_index < static_cast<size_t>(cached_element_slots.GetLength());
         ++slot_index) {
      cached_slots.push_back(CollectTemplateElementSlotChildren(
          cached_element_slots.GetProperty(static_cast<uint32_t>(slot_index))));
    }
  }

  for (size_t slot_index = 0; slot_index < slot_count; ++slot_index) {
    auto current_slot_children =
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    auto current_children =
        CollectTemplateElementSlotChildren(current_slot_children);
    if (slot_index >= cached_slots.size()) {
      cached_slots.emplace_back();
    }
    auto& cached_children = cached_slots[slot_index];
    base::Vector<bool> used_cached_children(cached_children.size(), false);

    for (const auto& current_child : current_children) {
      size_t matched_index = cached_children.size();
      for (size_t cached_index = 0; cached_index < cached_children.size();
           ++cached_index) {
        if (used_cached_children[cached_index]) {
          continue;
        }
        if (current_child.get() == cached_children[cached_index].get() ||
            current_child->HasSameTemplateIdentity(
                *cached_children[cached_index])) {
          matched_index = cached_index;
          break;
        }
      }

      if (matched_index < cached_children.size()) {
        used_cached_children[matched_index] = true;
        auto cached_child = cached_children[matched_index];
        if (current_child.get() == cached_child.get()) {
          current_child->MarkCachedTemplateTreeActiveRecursively();
        } else {
          current_child->TransferCachedTemplateTreeFrom(cached_child.get());
        }
      } else {
        current_child->MarkInTemplateTreeAndPrepareRecursively();
        current_child->ResolveGeneratedElements();
      }

      if (slot_index < element_slot_targets_.size()) {
        MountElementSlotChild(element_slot_targets_[slot_index], current_child,
                              nullptr);
      }
    }

    for (size_t cached_index = 0; cached_index < cached_children.size();
         ++cached_index) {
      if (!used_cached_children[cached_index] &&
          cached_children[cached_index] != nullptr) {
        cached_children[cached_index]->ReleaseCachedTemplateTreeRecursively();
      }
    }
  }

  for (size_t slot_index = slot_count; slot_index < cached_slots.size();
       ++slot_index) {
    for (const auto& cached_child : cached_slots[slot_index]) {
      if (cached_child != nullptr) {
        cached_child->ReleaseCachedTemplateTreeRecursively();
      }
    }
  }
}

void TemplateElement::PrepareAsyncCreateElementTree() {
  if (IsTypedTemplate()) {
    return;
  }
  if (result_ != nullptr || async_create_task_ != nullptr ||
      prepared_cached_template_tree_ != nullptr) {
    return;
  }
  if (TryPrepareCachedTemplateTree()) {
    return;
  }
  auto* manager = element_manager();
  if (manager == nullptr) {
    return;
  }

  if (entry_ == nullptr && tasm_ != nullptr) {
    entry_ = tasm_->FindEntry(bundle_url_.str()).get();
  }

  async_create_task_ = CreateAsyncCreateElementTreeTask(entry_);
  manager->EnqueuePostMTSRenderTask(
      base::closure([task = async_create_task_]() { task->Run(); }));
}

base::OnceTaskRefptr<GeneratedElementsResult>
TemplateElement::CreateAsyncCreateElementTreeTask(TemplateEntry* entry) {
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  auto template_key = template_key_;
  auto attribute_slots = CopyAttributeSlotsForStorage(attribute_slots_);
  auto root_attributes = root_attributes_;
  auto root_attributes_generation = root_attributes_generation_;
  auto element_slots = element_slots_;
  return fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
      [entry, template_key = std::move(template_key),
       attribute_slots = std::move(attribute_slots),
       root_attributes = std::move(root_attributes), root_attributes_generation,
       element_slots = std::move(element_slots),
       promise = std::move(promise)]() mutable {
        promise.set_value(GeneratePreparedElementsResult(
            entry, template_key, attribute_slots, root_attributes,
            root_attributes_generation, element_slots));
      },
      std::move(future));
}

void TemplateElement::ResolveGeneratedElements() {
  if (ActivateCachedTemplateTreeIfNeeded()) {
    return;
  }
  if (IsActiveMaterialized()) {
    return;
  }

  if (prepared_cached_template_tree_ != nullptr ||
      TryPrepareCachedTemplateTree()) {
    auto cached = std::move(prepared_cached_template_tree_);
    prepared_cached_template_tree_ = nullptr;
    TransferCachedTemplateTreeFrom(cached.get());
    return;
  }

  if (IsTypedTemplate()) {
    InitTypedRoot();
    if (result_ == nullptr) {
      return;
    }
    GeneratedElementsResult generated;
    PrepareGeneratedElementsResult(&generated, lepus::Value(), lepus::Value(),
                                   0, element_slots_);
    prepared_element_slot_insertions_ =
        std::move(generated.prepared_element_slot_insertions_);
    ApplyRootAttributes(lepus::Value());
    ApplyInitialElementSlots();
    ApplyPendingOperations();
    return;
  }

  if (async_create_task_ == nullptr) {
    PrepareAsyncCreateElementTree();
    if (async_create_task_ == nullptr) {
      return;
    }
  }

  async_create_task_->Run();
  auto generated = async_create_task_->GetFuture().get();
  async_create_task_ = nullptr;
  auto prepared_root_attributes = generated.prepared_root_attributes_;
  auto prepared_root_attributes_generation =
      generated.root_attributes_generation_;
  result_ = std::move(generated.result_);
  attribute_slot_targets_ = std::move(generated.attribute_slot_targets_);
  event_attribute_slot_targets_ =
      std::move(generated.event_attribute_slot_targets_);
  static_event_targets_ = std::move(generated.static_event_targets_);
  element_slot_targets_ = std::move(generated.element_slot_targets_);
  prepared_element_slot_insertions_ =
      std::move(generated.prepared_element_slot_insertions_);

  // Attach generated elements and mount slot children only when the template is
  // actually materialized into the Fiber tree.
  InitGeneratedElementTree(prepared_root_attributes,
                           prepared_root_attributes_generation);
  ApplyInitialElementSlots();
  ApplyPendingOperations();
}

void TemplateElement::InitGeneratedElementTree(
    const lepus::Value& prepared_root_attributes,
    uint32_t prepared_root_attributes_generation) {
  auto* manager = element_manager();
  if (result_ == nullptr || manager == nullptr || entry_ == nullptr) {
    return;
  }
  auto* root = manager->root();
  TreeResolver::InitElementTree(result_, root != nullptr ? root->impl_id() : -1,
                                manager, entry_->GetStyleSheetManager());
  // Event attributes must be applied after the generated tree is attached so
  // FiberAddEvent can sync EventListenerMap when event-refactor is enabled.
  ApplyStaticEventAttributes(static_event_targets_);
  ApplyInitialEventAttributeSlots(event_attribute_slot_targets_,
                                  attribute_slots_);
  ApplyInitialRootEventAttributes(prepared_root_attributes,
                                  prepared_root_attributes_generation);
}

void TemplateElement::ApplyInitialRootEventAttributes(
    const lepus::Value& prepared_root_attributes,
    uint32_t prepared_root_attributes_generation) {
  if (result_ == nullptr) {
    return;
  }
  if (prepared_root_attributes_generation != root_attributes_generation_) {
    ApplyRootTemplateAttributes(result_.get(), prepared_root_attributes,
                                root_attributes_);
    return;
  }
  ApplyRootTemplateEventAttributes(result_.get(), root_attributes_);
}

void TemplateElement::InitTypedRoot() {
  if (!IsTypedTemplate() || result_ != nullptr) {
    return;
  }

  auto* manager = element_manager();
  if (manager == nullptr) {
    return;
  }

  result_ = CreateTypedRootElement(manager, tasm_, typed_tag_);
  if (result_ == nullptr) {
    return;
  }
  result_->MarkTemplateElement();
  // Element Template is currently only used by RL3 in page scope. Typed roots
  // are created outside the normal FiberCreate* APIs, so seed their component
  // scope from the page root before class style resolution.
  auto* root = manager->root();
  if (root != nullptr) {
    result_->SetParentComponentUniqueIdRecursively(root->impl_id());
  }

  element_slot_targets_.clear();
  element_slot_targets_.push_back(ElementSlotMountPoint{result_, nullptr});
}

bool TemplateElement::IsPageTemplate() const {
  return IsTypedTemplate() && typed_tag_.IsEqual(kElementPageTag);
}

void TemplateElement::MarkInTemplateTreeAndPrepare() {
  if (IsInTemplateTree() || IsInTemplateCache()) {
    return;
  }
  template_tree_state_ = TemplateElementTreeState::kInTemplateTree;
  PrepareAsyncCreateElementTree();
}

void TemplateElement::MarkInTemplateTreeAndPrepareRecursively() {
  if (IsInTemplateTree() || IsInTemplateCache()) {
    return;
  }
  MarkInTemplateTreeAndPrepare();
  MarkTemplateChildrenInElementSlotsInTree();
}

void TemplateElement::MarkTemplateChildrenInElementSlotsInTree() {
  if (!element_slots_.IsArrayOrJSArray()) {
    return;
  }

  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }

    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveInitialElementSlotChild(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child == nullptr || !child->is_template()) {
        continue;
      }
      static_cast<TemplateElement*>(child.get())
          ->MarkInTemplateTreeAndPrepareRecursively();
    }
  }
}

void TemplateElement::ApplyRootAttributes(
    const lepus::Value& previous_root_attributes) {
  if (!IsActiveMaterialized() ||
      (!previous_root_attributes.IsObject() && !root_attributes_.IsObject())) {
    return;
  }

  ApplyRootTemplateAttributes(result_.get(), previous_root_attributes,
                              root_attributes_);
}

void TemplateElement::ApplyAttributeSlotToTarget(
    uint32_t slot_index, const lepus::Value& previous_attribute_slots) {
  if (!IsActiveMaterialized() || slot_index >= attribute_slot_targets_.size()) {
    return;
  }
  auto target = attribute_slot_targets_[slot_index];
  if (target == nullptr) {
    return;
  }
  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), previous_attribute_slots, attribute_slots_);
}

void TemplateElement::ApplyPendingOperations() {
  auto operations = std::move(pending_operations_);
  pending_operations_.clear();

  for (const auto& operation : operations) {
    if (operation.type_ == PendingOperation::Type::kSetAttributeSlot) {
      SetAttributeSlot(operation.slot_index_, operation.value_);
      continue;
    }
    if (operation.type_ == PendingOperation::Type::kInsertElementSlotChild) {
      InsertElementSlotChild(operation.slot_index_, operation.child_,
                             operation.ref_node_);
      continue;
    }
    if (operation.type_ == PendingOperation::Type::kRemoveElementSlotChild) {
      RemoveElementSlotChild(operation.slot_index_, operation.child_);
      continue;
    }
  }
}

void TemplateElement::ApplyInitialElementSlots() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_APPLY_INITIAL_ELEMENT_SLOTS,
              "template_key", template_key_.str(), "bundle_url",
              bundle_url_.str());
  for (const auto& insertion : prepared_element_slot_insertions_) {
    auto slot_index = static_cast<size_t>(insertion.slot_index_);
    if (slot_index >= element_slot_targets_.size()) {
      continue;
    }
    const auto& mount_point = element_slot_targets_[slot_index];
    if (mount_point.parent_ == nullptr || insertion.child_ == nullptr) {
      continue;
    }
    InsertInitialElementSlotChild(mount_point, insertion.child_);
  }
}

void TemplateElement::InsertInitialElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }
  if (mount_point.ref_node_ != nullptr) {
    mount_point.parent_->InsertNodeBefore(child, mount_point.ref_node_);
  } else {
    mount_point.parent_->InsertNode(child);
  }
}

void TemplateElement::MountElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child,
    const fml::RefPtr<FiberElement>& ref_node) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }

  auto mounted_child = child;
  if (child->is_template()) {
    auto* template_child = static_cast<TemplateElement*>(child.get());
    template_child->ActivateCachedTemplateTreeIfNeeded();
    if (template_child->IsActiveMaterialized()) {
      mounted_child = template_child->result_;
    }
  }

  auto mounted_ref_node = ref_node;
  if (mounted_ref_node != nullptr && mounted_ref_node->is_template()) {
    auto* template_ref = static_cast<TemplateElement*>(mounted_ref_node.get());
    template_ref->ActivateCachedTemplateTreeIfNeeded();
    if (template_ref->IsActiveMaterialized()) {
      mounted_ref_node = template_ref->result_;
    }
  }

  auto target_ref_node =
      mounted_ref_node != nullptr ? mounted_ref_node : mount_point.ref_node_;
  if (target_ref_node.get() == mounted_child.get()) {
    return;
  }
  if (mounted_child->parent() == mount_point.parent_.get()) {
    if (mounted_child->next_sibling() == target_ref_node.get()) {
      return;
    }
    mount_point.parent_->RemoveNode(mounted_child);
  }

  if (target_ref_node != nullptr) {
    mount_point.parent_->InsertNodeBefore(mounted_child, target_ref_node);
  } else {
    mount_point.parent_->InsertNode(mounted_child);
  }
}

void TemplateElement::UnmountElementSlotChild(
    const ElementSlotMountPoint& mount_point,
    const fml::RefPtr<FiberElement>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }

  auto mounted_child = child;
  if (child->is_template()) {
    auto* template_child = static_cast<TemplateElement*>(child.get());
    if (template_child->result_ != nullptr) {
      mounted_child = template_child->result_;
    }
  }

  if (mounted_child->parent() == mount_point.parent_.get()) {
    mount_point.parent_->RemoveNode(mounted_child);
  }
}

lepus::Value TemplateElement::GetOrCreateElementSlotChildren(
    uint32_t slot_index) {
  if (!element_slots_.IsArrayOrJSArray()) {
    element_slots_ = lepus::Value(lepus::CArray::Create());
  } else {
    EnsureMutableArrayForWrite(&element_slots_);
  }
  auto slot_children = element_slots_.GetProperty(slot_index);
  if (!slot_children.IsArrayOrJSArray()) {
    slot_children = lepus::Value(lepus::CArray::Create());
  } else {
    EnsureMutableArrayForWrite(&slot_children);
  }
  element_slots_.SetProperty(slot_index, slot_children);
  return slot_children;
}

void TemplateElement::RemoveElementSlotChildFromSlot(uint32_t slot_index,
                                                     FiberElement* child) {
  if (child == nullptr || !element_slots_.IsArrayOrJSArray() ||
      slot_index >= static_cast<uint32_t>(element_slots_.GetLength())) {
    return;
  }
  EnsureMutableArrayForWrite(&element_slots_);
  auto slot_children = element_slots_.GetProperty(slot_index);
  if (!slot_children.IsArrayOrJSArray()) {
    return;
  }
  EnsureMutableArrayForWrite(&slot_children);
  RemoveElementFromSlotChildren(&slot_children, child);
  element_slots_.SetProperty(slot_index, slot_children);
}

lepus::Value TemplateElement::Serialize() const {
  if (IsTypedTemplate()) {
    return SerializeTypedTemplate();
  }
  return SerializeCompiledTemplate();
}

lepus::Value TemplateElement::SerializeTypedTemplate() const {
  auto serialized = lepus::Dictionary::Create();
  serialized->SetValue(BASE_STATIC_STRING(kTemplateTypedTag), typed_tag_);
  auto attributes = SerializeRootAttributes();
  if (!attributes.IsEmpty()) {
    serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributes),
                         std::move(attributes));
  }
  serialized->SetValue(BASE_STATIC_STRING(kTemplateElementSlots),
                       SerializeElementSlots());
  auto options = SerializeOptions();
  if (!options.IsEmpty()) {
    serialized->SetValue(BASE_STATIC_STRING(kTemplateOptions),
                         std::move(options));
  }
  serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), uid_);
  return lepus::Value(std::move(serialized));
}

lepus::Value TemplateElement::SerializeRootAttributes() const {
  if (!root_attributes_.IsObject() || root_attributes_.GetLength() == 0) {
    return lepus::Value();
  }
  return root_attributes_;
}

lepus::Value TemplateElement::SerializeCompiledTemplate() const {
  auto serialized = lepus::Dictionary::Create();
  serialized->SetValue(BASE_STATIC_STRING(kTemplateKey), template_key_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateBundleUrl), bundle_url_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributeSlots),
                       attribute_slots_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateElementSlots),
                       SerializeElementSlots());
  auto options = SerializeOptions();
  if (!options.IsEmpty()) {
    serialized->SetValue(BASE_STATIC_STRING(kTemplateOptions),
                         std::move(options));
  }
  serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), uid_);
  return lepus::Value(std::move(serialized));
}

lepus::Value TemplateElement::SerializeElementSlots() const {
  if (!element_slots_.IsArrayOrJSArray()) {
    return element_slots_;
  }

  auto serialized_slots = lepus::CArray::Create();
  serialized_slots->reserve(element_slots_.GetLength());
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(element_slots_.GetLength());
       ++slot_index) {
    serialized_slots->emplace_back(SerializeElementSlotChildren(
        element_slots_.GetProperty(static_cast<uint32_t>(slot_index))));
  }
  return lepus::Value(std::move(serialized_slots));
}

lepus::Value TemplateElement::SerializeOptions() const {
  if (!options_.IsObject() || options_.GetLength() == 0) {
    return lepus::Value();
  }
  auto serialized_options = lepus::Dictionary::Create();
  serialized_options->reserve(options_.GetLength());
  lepus::Value::ForEachLepusValue(
      options_, [this, &serialized_options](const lepus::Value& key,
                                            const lepus::Value& option_value) {
        if (!key.IsString()) {
          return;
        }
        serialized_options->SetValue(
            key.String(), option_value.IsArrayOrJSArray()
                              ? SerializeTemplateOptionArray(option_value)
                              : option_value);
      });
  return lepus::Value(std::move(serialized_options));
}

lepus::Value TemplateElement::SerializeTemplateOptionArray(
    const lepus::Value& value) const {
  auto serialized_array = lepus::CArray::Create();
  serialized_array->reserve(value.GetLength());
  for (size_t index = 0; index < static_cast<size_t>(value.GetLength());
       ++index) {
    auto option_value = value.GetProperty(static_cast<uint32_t>(index));
    if (!option_value.IsRefCounted()) {
      serialized_array->emplace_back(std::move(option_value));
      continue;
    }

    auto ref_counted = option_value.RefCounted();
    if (ref_counted->GetRefType() != lepus::RefType::kElement) {
      serialized_array->emplace_back(std::move(option_value));
      continue;
    }

    auto element =
        fml::static_ref_ptr_cast<FiberElement>(ref_counted).strongify();
    if (element == nullptr || !element->is_template()) {
      serialized_array->emplace_back(std::move(option_value));
      continue;
    }

    auto template_element = fml::static_ref_ptr_cast<TemplateElement>(element);
    serialized_array->emplace_back(template_element->Serialize());
  }
  return lepus::Value(std::move(serialized_array));
}

lepus::Value TemplateElement::SerializeElementSlotChildren(
    const lepus::Value& slot_children) const {
  if (!slot_children.IsArrayOrJSArray()) {
    return slot_children;
  }

  auto serialized_children = lepus::CArray::Create();
  serialized_children->reserve(slot_children.GetLength());
  for (size_t child_index = 0;
       child_index < static_cast<size_t>(slot_children.GetLength());
       ++child_index) {
    auto serialized_child = SerializeElementSlotChild(
        slot_children.GetProperty(static_cast<uint32_t>(child_index)));
    if (!serialized_child.IsEmpty() && !serialized_child.IsUndefined()) {
      serialized_children->emplace_back(std::move(serialized_child));
    }
  }
  return lepus::Value(std::move(serialized_children));
}

lepus::Value TemplateElement::SerializeElementSlotChild(
    const lepus::Value& child) const {
  if (!child.IsRefCounted()) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots, but got non-refcounted child.");
    return lepus::Value();
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots.");
    return lepus::Value();
  }

  auto element =
      fml::static_ref_ptr_cast<FiberElement>(ref_counted).strongify();
  if (element == nullptr || !element->is_template()) {
    LOGE(
        "SerializeElementTemplate only supports TemplateElement children in "
        "elementSlots.");
    return lepus::Value();
  }
  auto template_element = fml::static_ref_ptr_cast<TemplateElement>(element);
  return template_element->Serialize();
}

fml::RefPtr<FiberElement> TemplateElement::GetRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_GET_ROOT, "template_key",
              template_key_.str(), "bundle_url", bundle_url_.str());
  ResolveGeneratedElements();

  EXEC_EXPR_FOR_INSPECTOR(
      auto* manager = element_manager();
      if (result_ != nullptr && manager != nullptr &&
          manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
        std::function<void(FiberElement*)> prepare_node_f =
            [manager, &prepare_node_f](FiberElement* element) {
              manager->PrepareNodeForInspector(element);
              for (const auto& child : element->children()) {
                prepare_node_f(static_cast<FiberElement*>(child.get()));
              }
            };
        prepare_node_f(result_.get());
      });
  return result_;
}

void TemplateElement::SetAttributeSlot(uint32_t slot_index,
                                       const lepus::Value& value) {
  if (IsTypedTemplate()) {
    if (slot_index == kTypedTemplateRootSlotIndex) {
      SetRootAttributes(value);
    }
    return;
  }
  const bool should_apply_to_result = IsActiveMaterialized();
  if (!should_apply_to_result) {
    pending_operations_.emplace_back(PendingOperation::Type::kSetAttributeSlot,
                                     slot_index, value);
    return;
  }

  auto previous_attribute_slots = CopyArrayHeader(attribute_slots_);
  if (!attribute_slots_.IsArrayOrJSArray()) {
    attribute_slots_ = lepus::Value(lepus::CArray::Create());
  } else {
    EnsureMutableArrayForWrite(&attribute_slots_);
  }
  attribute_slots_.SetProperty(slot_index, value);

  ApplyAttributeSlotToTarget(slot_index, previous_attribute_slots);
}

void TemplateElement::InsertElementSlotChild(
    uint32_t slot_index, const fml::RefPtr<FiberElement>& child,
    const fml::RefPtr<FiberElement>& ref_node) {
  if (child == nullptr || child.get() == ref_node.get()) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootSlotIndex) {
    return;
  }

  if (IsInTemplateTree() && child->is_template()) {
    static_cast<TemplateElement*>(child.get())
        ->MarkInTemplateTreeAndPrepareRecursively();
  }

  const bool should_apply_to_result = IsActiveMaterialized();
  if (!should_apply_to_result) {
    pending_operations_.emplace_back(
        PendingOperation::Type::kInsertElementSlotChild, slot_index, child,
        ref_node);
    return;
  }

  // TODO(songshourui.null): Restore insert-or-move semantics by removing this
  // child from existing element slot records before inserting it here, so
  // Serialize() does not keep stale same-slot or cross-slot entries.
  auto slot_children = GetOrCreateElementSlotChildren(slot_index);
  auto insert_index = FindSlotChildIndex(slot_children, ref_node.get());
  slot_children.Array()->Insert(static_cast<uint32_t>(insert_index),
                                lepus::Value(child));
  element_slots_.SetProperty(slot_index, slot_children);

  if (slot_index < element_slot_targets_.size()) {
    MountElementSlotChild(element_slot_targets_[slot_index], child, ref_node);
  }
}

void TemplateElement::RemoveElementSlotChild(
    uint32_t slot_index, const fml::RefPtr<FiberElement>& child) {
  if (child == nullptr) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootSlotIndex) {
    return;
  }

  const bool should_apply_to_result = IsActiveMaterialized();
  if (!should_apply_to_result) {
    pending_operations_.emplace_back(
        PendingOperation::Type::kRemoveElementSlotChild, slot_index, child);
    return;
  }

  RemoveElementSlotChildFromSlot(slot_index, child.get());
  if (slot_index < element_slot_targets_.size()) {
    UnmountElementSlotChild(element_slot_targets_[slot_index], child);
  }
  if (child->is_template() && child->is_list_item()) {
    static_cast<TemplateElement*>(child.get())
        ->MoveToTemplateTreeCacheIfNeeded();
  }
}

}  // namespace tasm
}  // namespace lynx
