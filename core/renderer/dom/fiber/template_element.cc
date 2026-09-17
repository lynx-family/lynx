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

static constexpr const char kTemplateTag[] = "template";
static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateRootAttributeSpread[] = "rootAttributes";
static constexpr const char kDefaultPageComponentId[] = "0";
static constexpr int32_t kDefaultPageCSSId = 0;
static constexpr uint32_t kTypedTemplateRootSlotIndex = 0;

fml::RefPtr<Element> ResolveInitialElementSlotChild(const lepus::Value& child) {
  if (!child.IsRefCounted()) {
    return nullptr;
  }

  auto ref_counted = child.RefCounted();
  if (ref_counted->GetRefType() != lepus::RefType::kElement) {
    return nullptr;
  }

  return fml::static_ref_ptr_cast<Element>(ref_counted);
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
void ApplyRootTemplateAttributes(Element* root, Apply&& apply) {
  if (root == nullptr) {
    return;
  }
  auto compiled_attributes = root->template_attributes();
  root->SetTemplateAttributes(CreateRootSpreadTemplateAttributes());
  apply();
  root->SetTemplateAttributes(compiled_attributes);
}

void ApplyRootTemplateAttributes(Element* root,
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

void ApplyRootTemplateNonEventAttributes(Element* root,
                                         const lepus::Value& root_attributes) {
  if (!root_attributes.IsObject()) {
    return;
  }
  ApplyRootTemplateAttributes(root, [root, &root_attributes]() {
    TreeResolver::ApplyTemplateNonEventAttributesToElement(
        root, CreateRootAttributeSlots(root_attributes));
  });
}

void ApplyRootTemplateEventAttributes(Element* root,
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

fml::RefPtr<Element> CreateTypedRootElement(ElementManager* manager,
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
    const base::Vector<fml::RefPtr<Element>>& targets,
    const lepus::Value& attribute_slots, Apply apply) {
  Element* previous_element = nullptr;
  for (const auto& target : targets) {
    auto* element = target.get();
    if (element == nullptr || element == previous_element) {
      continue;
    }
    apply(element, attribute_slots);
    previous_element = element;
  }
}

void ApplyInitialNonEventAttributeSlots(
    const base::Vector<fml::RefPtr<Element>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](Element* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateNonEventAttributesToElement(element, slots);
      });
}

void ApplyInitialEventAttributeSlots(
    const base::Vector<fml::RefPtr<Element>>& targets,
    const lepus::Value& attribute_slots) {
  ApplyInitialAttributeSlots(
      targets, attribute_slots,
      [](Element* element, const lepus::Value& slots) {
        TreeResolver::ApplyTemplateEventAttributesToElement(element, slots);
      });
}

void ApplyStaticEventAttributes(
    const base::Vector<fml::RefPtr<Element>>& targets) {
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
    : Element(element_manager, BASE_STATIC_STRING(kTemplateTag)),
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

void TemplateElement::PrepareAsyncCreateElementTree() {
  if (IsTypedTemplate()) {
    return;
  }
  if (result_ != nullptr || async_create_task_ != nullptr) {
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
  if (IsActiveMaterialized()) {
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
  if (IsInTemplateTree()) {
    return;
  }
  template_tree_state_ = TemplateElementTreeState::kInTemplateTree;
  PrepareAsyncCreateElementTree();
}

void TemplateElement::MarkInTemplateTreeAndPrepareRecursively() {
  if (IsInTemplateTree()) {
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
    const fml::RefPtr<Element>& child) {
  if (mount_point.parent_ == nullptr || child == nullptr) {
    return;
  }
  if (mount_point.ref_node_ != nullptr) {
    mount_point.parent_->InsertNodeBefore(child, mount_point.ref_node_);
  } else {
    mount_point.parent_->InsertNode(child);
  }
}

fml::RefPtr<Element> TemplateElement::GetRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_GET_ROOT, "template_key",
              template_key_.str(), "bundle_url", bundle_url_.str());
  ResolveGeneratedElements();

  EXEC_EXPR_FOR_INSPECTOR(
      auto* manager = element_manager();
      if (result_ != nullptr && manager != nullptr &&
          manager->GetDevToolFlag() && manager->IsDomTreeEnabled()) {
        std::function<void(Element*)> prepare_node_f =
            [manager, &prepare_node_f](Element* element) {
              manager->PrepareNodeForInspector(element);
              for (const auto& child : element->children()) {
                prepare_node_f(child.get());
              }
            };
        prepare_node_f(result_.get());
      });
  return result_;
}

}  // namespace tasm
}  // namespace lynx
