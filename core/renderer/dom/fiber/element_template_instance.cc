// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <future>
#include <memory>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/tree_resolver.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/template_entry.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/tasm_constants.h"

namespace lynx {
namespace tasm {
namespace {

static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateKey[] = "templateKey";
static constexpr const char kTemplateTypedTag[] = "tag";
static constexpr const char kTemplateAttributes[] = "attributes";
static constexpr const char kTemplateBundleUrl[] = "bundleUrl";
static constexpr const char kTemplateAttributeSlots[] = "attributeSlots";
static constexpr const char kTemplateChildSlots[] = "childSlots";
static constexpr const char kTemplateOptions[] = "options";
static constexpr const char kTemplateUid[] = "uid";
static constexpr const char kTemplateRootAttributeSpread[] = "rootAttributes";
static constexpr const char kDefaultPageComponentId[] = "0";
static constexpr int32_t kDefaultPageCSSId = 0;
static constexpr uint32_t kTypedTemplateRootAttributeSlotIndex = 0;
static constexpr uint32_t kTypedTemplateRootChildSlotIndex = 0;

fml::RefPtr<ElementTemplateInstance> ResolveElementTemplateInstanceValue(
    const lepus::Value& value) {
  if (!value.IsRefCounted()) {
    return nullptr;
  }
  auto ref = value.RefCounted();
  if (ref->GetRefType() != lepus::RefType::kElementTemplate) {
    return nullptr;
  }
  return fml::static_ref_ptr_cast<ElementTemplateInstance>(ref).strongify();
}

size_t FindSlotChildIndex(const lepus::Value& slot_children,
                          const lepus::Value& ref_node) {
  if (!ref_node.IsRefCounted()) {
    return static_cast<size_t>(slot_children.GetLength());
  }
  for (size_t i = 0; i < static_cast<size_t>(slot_children.GetLength()); ++i) {
    if (slot_children.GetProperty(static_cast<uint32_t>(i)).IsEqual(ref_node)) {
      return i;
    }
  }
  return static_cast<size_t>(slot_children.GetLength());
}

lepus::Value CopyTemplateValueForStorage(const lepus::Value& value) {
  return value.IsCallable() ? value : lepus::Value::Clone(value);
}

lepus::Value CopyTemplateObjectForStorage(const lepus::Value& value) {
  auto object = lepus::Dictionary::Create();
  lepus::Value::ForEachLepusValue(
      value, [&object](const lepus::Value& key, const lepus::Value& item) {
        if (key.IsString()) {
          object->SetValue(key.String(), CopyTemplateValueForStorage(item));
        }
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
  attribute_slots->emplace_back(root_attributes.IsObject() ? root_attributes
                                                           : lepus::Value());
  return lepus::Value(std::move(attribute_slots));
}

const SharedTemplateAttributes& RootSpreadTemplateAttributes() {
  static const auto attributes =
      std::make_shared<const TemplateAttributes>(TemplateAttributes{
          Attribute{ATTRIBUTE_BINDING_TYPE_SPREAD,
                    BASE_STATIC_STRING(kTemplateRootAttributeSpread),
                    lepus::Value(), kTypedTemplateRootAttributeSlotIndex}});
  return attributes;
}

template <typename Apply>
void ApplyRootTemplateAttributes(Element* root, Apply&& apply) {
  if (root == nullptr) {
    return;
  }
  auto compiled_attributes = root->template_attributes();
  root->SetTemplateAttributes(RootSpreadTemplateAttributes());
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

void ApplyRootTemplateNonEventAttributes(
    Element* root, const lepus::Value& previous_root_attributes,
    const lepus::Value& root_attributes) {
  if (!previous_root_attributes.IsObject() && !root_attributes.IsObject()) {
    return;
  }
  ApplyRootTemplateAttributes(
      root, [root, &previous_root_attributes, &root_attributes]() {
        if (previous_root_attributes.IsObject()) {
          TreeResolver::ApplyTemplateNonEventAttributesToElement(
              root, CreateRootAttributeSlots(previous_root_attributes),
              CreateRootAttributeSlots(root_attributes));
          return;
        }
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

GeneratedElementsResult GeneratePreparedElementsResult(
    TemplateEntry* entry, const base::String& template_key,
    const lepus::Value& attribute_slots, uint32_t attribute_slots_generation,
    const lepus::Value& root_attributes, uint32_t root_attributes_generation) {
  GeneratedElementsResult generated;
  if (entry != nullptr) {
    auto& info = entry->GetElementTemplateInfo(template_key.str());
    generated = TreeResolver::GenerateElementsFromTemplateInfo(info);
  }
  ApplyInitialNonEventAttributeSlots(generated.attribute_slot_targets_,
                                     attribute_slots);
  ApplyRootTemplateNonEventAttributes(generated.result_.get(), root_attributes);
  generated.prepared_attribute_slots_ = attribute_slots;
  generated.attribute_slots_generation_ = attribute_slots_generation;
  generated.prepared_root_attributes_ = root_attributes;
  generated.root_attributes_generation_ = root_attributes_generation;
  return generated;
}

}  // namespace

class ElementTemplateInstanceSerializer {
 public:
  lepus::Value Serialize(const ElementTemplateInstance& instance) {
    return instance.IsTypedTemplate() ? SerializeTypedTemplate(instance)
                                      : SerializeCompiledTemplate(instance);
  }

 private:
  lepus::Value SerializeChildSlots(const ElementTemplateInstance& instance) {
    if (!instance.child_slots_.IsArrayOrJSArray()) {
      return instance.child_slots_;
    }
    auto serialized_slots = lepus::CArray::Create();
    const int slot_count = instance.child_slots_.GetLength();
    serialized_slots->reserve(static_cast<size_t>(slot_count));
    for (int slot_index = 0; slot_index < slot_count; ++slot_index) {
      auto slot_children =
          instance.child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
      if (!slot_children.IsArrayOrJSArray()) {
        serialized_slots->emplace_back(std::move(slot_children));
        continue;
      }
      auto serialized_children = lepus::CArray::Create();
      const int child_count = slot_children.GetLength();
      serialized_children->reserve(static_cast<size_t>(child_count));
      for (int child_index = 0; child_index < child_count; ++child_index) {
        auto child = ResolveElementTemplateInstanceValue(
            slot_children.GetProperty(static_cast<uint32_t>(child_index)));
        if (child != nullptr) {
          serialized_children->emplace_back(Serialize(*child));
        }
      }
      serialized_slots->emplace_back(std::move(serialized_children));
    }
    return lepus::Value(std::move(serialized_slots));
  }

  void SetSerializedOptions(const ElementTemplateInstance& instance,
                            const lepus::DictionaryPtr& serialized) {
    if (instance.options_.IsObject() && instance.options_.GetLength() > 0) {
      serialized->SetValue(BASE_STATIC_STRING(kTemplateOptions),
                           instance.options_);
    }
  }

  void SetSerializedUid(const ElementTemplateInstance& instance,
                        const lepus::DictionaryPtr& serialized) {
    serialized->SetValue(BASE_STATIC_STRING(kTemplateUid), instance.uid_);
  }

  lepus::Value SerializeTypedTemplate(const ElementTemplateInstance& instance) {
    auto serialized = lepus::Dictionary::Create();
    serialized->SetValue(BASE_STATIC_STRING(kTemplateTypedTag),
                         instance.typed_tag_);
    if (instance.root_attributes_.IsObject() &&
        instance.root_attributes_.GetLength() > 0) {
      serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributes),
                           instance.root_attributes_);
    }
    serialized->SetValue(BASE_STATIC_STRING(kTemplateChildSlots),
                         SerializeChildSlots(instance));
    SetSerializedOptions(instance, serialized);
    SetSerializedUid(instance, serialized);
    return lepus::Value(std::move(serialized));
  }

  lepus::Value SerializeCompiledTemplate(
      const ElementTemplateInstance& instance) {
    auto serialized = lepus::Dictionary::Create();
    serialized->SetValue(BASE_STATIC_STRING(kTemplateKey),
                         instance.template_key_);
    serialized->SetValue(BASE_STATIC_STRING(kTemplateBundleUrl),
                         instance.bundle_url_);
    serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributeSlots),
                         instance.attribute_slots_);
    serialized->SetValue(BASE_STATIC_STRING(kTemplateChildSlots),
                         SerializeChildSlots(instance));
    SetSerializedOptions(instance, serialized);
    SetSerializedUid(instance, serialized);
    return lepus::Value(std::move(serialized));
  }
};

ElementTemplateInstance::ElementTemplateInstance(
    ElementManager* element_manager)
    : element_manager_(element_manager),
      bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {}

ElementTemplateInstance::~ElementTemplateInstance() {
  ClearLogicalChildParentLinks();
}

void ElementTemplateInstance::SetTypedTag(const base::String& typed_tag) {
  typed_tag_ = typed_tag;
}

void ElementTemplateInstance::SetRootAttributes(
    const lepus::Value& attributes) {
  if (!attributes.IsObject() && !attributes.IsNil() &&
      !attributes.IsUndefined()) {
    return;
  }
  auto next_root_attributes = attributes.IsObject()
                                  ? CopyTemplateObjectForStorage(attributes)
                                  : lepus::Value();
  auto previous_root_attributes = std::move(root_attributes_);
  root_attributes_ = std::move(next_root_attributes);
  ++root_attributes_generation_;
  if (IsMaterialized()) {
    ApplyRootTemplateAttributes(result_.get(), previous_root_attributes,
                                root_attributes_);
  }
}

void ElementTemplateInstance::SetAttributeSlots(
    const lepus::Value& attribute_slots) {
  attribute_slots_ = CopyAttributeSlotsForStorage(attribute_slots);
  ++attribute_slots_generation_;
}

void ElementTemplateInstance::InitializeChildSlots(
    const lepus::Value& child_slots) {
  if (!child_slots.IsArrayOrJSArray()) {
    child_slots_ = child_slots;
    return;
  }

  // Rebuild both array levels so later logical mutations do not alias the
  // caller's arrays. Sequential inserts also establish single-parent ownership.
  const size_t slot_count = static_cast<size_t>(child_slots.GetLength());
  auto stored_slots = lepus::CArray::Create();
  stored_slots->reserve(slot_count);
  child_slots_ = lepus::Value(stored_slots);
  for (size_t slot_index = 0; slot_index < slot_count; ++slot_index) {
    auto slot_children =
        child_slots.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      stored_slots->emplace_back(std::move(slot_children));
      continue;
    }
    auto stored_children = lepus::CArray::Create();
    stored_slots->emplace_back(stored_children);
    const size_t child_count = static_cast<size_t>(slot_children.GetLength());
    for (size_t child_index = 0; child_index < child_count; ++child_index) {
      auto child_value =
          slot_children.GetProperty(static_cast<uint32_t>(child_index));
      if (child_value.IsNil() || child_value.IsUndefined() ||
          child_value.IsEmpty()) {
        stored_children->emplace_back(std::move(child_value));
        continue;
      }
      InsertNodeIntoChildSlot(static_cast<uint32_t>(slot_index), child_value,
                              lepus::Value());
    }
  }
}

void ElementTemplateInstance::SetOptions(const lepus::Value& options) {
  options_ = options;
}

void ElementTemplateInstance::SetUid(const lepus::Value& uid) { uid_ = uid; }

void ElementTemplateInstance::RequestMaterializationRecursively() {
  if (materialization_requested_) {
    return;
  }
  materialization_requested_ = true;
  EnsureCreateElementTreeTaskScheduled();
  if (!child_slots_.IsArrayOrJSArray()) {
    return;
  }

  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(child_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }
    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveElementTemplateInstanceValue(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child != nullptr) {
        child->RequestMaterializationRecursively();
      }
    }
  }
}

void ElementTemplateInstance::EnsureCreateElementTreeTaskScheduled() {
  if (IsTypedTemplate() || result_ != nullptr ||
      create_element_tree_task_ != nullptr) {
    return;
  }
  if (entry_ == nullptr && tasm_ != nullptr) {
    entry_ = tasm_->FindEntry(bundle_url_.str()).get();
  }

  create_element_tree_task_ = CreateElementTreeTask(entry_);
  element_manager_->EnqueuePostMTSRenderTask(
      base::closure([task = create_element_tree_task_]() { task->Run(); }));
}

base::OnceTaskRefptr<GeneratedElementsResult>
ElementTemplateInstance::CreateElementTreeTask(TemplateEntry* entry) {
  std::promise<GeneratedElementsResult> promise;
  auto future = promise.get_future();
  auto template_key = template_key_;
  auto attribute_slots = attribute_slots_;
  auto attribute_slots_generation = attribute_slots_generation_;
  auto root_attributes = root_attributes_;
  auto root_attributes_generation = root_attributes_generation_;
  return fml::MakeRefCounted<base::OnceTask<GeneratedElementsResult>>(
      [entry, template_key = std::move(template_key),
       attribute_slots = std::move(attribute_slots), attribute_slots_generation,
       root_attributes = std::move(root_attributes), root_attributes_generation,
       promise = std::move(promise)]() mutable {
        promise.set_value(GeneratePreparedElementsResult(
            entry, template_key, attribute_slots, attribute_slots_generation,
            root_attributes, root_attributes_generation));
      },
      std::move(future));
}

void ElementTemplateInstance::MaterializeRoot() {
  if (IsMaterialized()) {
    return;
  }

  if (IsTypedTemplate()) {
    InitTypedRoot();
    if (result_ == nullptr) {
      return;
    }
    ApplyRootTemplateAttributes(result_.get(), lepus::Value(),
                                root_attributes_);
    return;
  }

  if (create_element_tree_task_ == nullptr) {
    EnsureCreateElementTreeTaskScheduled();
    if (create_element_tree_task_ == nullptr) {
      return;
    }
  }

  create_element_tree_task_->Run();
  auto generated = create_element_tree_task_->GetFuture().get();
  create_element_tree_task_ = nullptr;
  if (generated.result_ == nullptr) {
    return;
  }
  result_ = std::move(generated.result_);
  attribute_slot_targets_ = std::move(generated.attribute_slot_targets_);
  event_attribute_slot_targets_ =
      std::move(generated.event_attribute_slot_targets_);
  static_event_targets_ = std::move(generated.static_event_targets_);
  element_slot_targets_ = std::move(generated.element_slot_targets_);

  InitGeneratedElementTree(generated.prepared_attribute_slots_,
                           generated.attribute_slots_generation_,
                           generated.prepared_root_attributes_,
                           generated.root_attributes_generation_);
}

void ElementTemplateInstance::InitGeneratedElementTree(
    const lepus::Value& prepared_attribute_slots,
    uint32_t prepared_attribute_slots_generation,
    const lepus::Value& prepared_root_attributes,
    uint32_t prepared_root_attributes_generation) {
  if (result_ == nullptr || entry_ == nullptr) {
    return;
  }
  auto* root = element_manager_->root();
  TreeResolver::InitElementTree(result_, root != nullptr ? root->impl_id() : -1,
                                element_manager_,
                                entry_->GetStyleSheetManager());
  ApplyStaticEventAttributes(static_event_targets_);
  if (prepared_attribute_slots_generation != attribute_slots_generation_) {
    ApplyInitialAttributeSlots(
        attribute_slot_targets_, attribute_slots_,
        [&prepared_attribute_slots](Element* element,
                                    const lepus::Value& slots) {
          TreeResolver::ApplyTemplateNonEventAttributesToElement(
              element, prepared_attribute_slots, slots);
        });
  }
  ApplyInitialEventAttributeSlots(event_attribute_slot_targets_,
                                  attribute_slots_);
  if (prepared_root_attributes_generation != root_attributes_generation_) {
    ApplyRootTemplateNonEventAttributes(result_.get(), prepared_root_attributes,
                                        root_attributes_);
  }
  ApplyRootTemplateEventAttributes(result_.get(), root_attributes_);
}

void ElementTemplateInstance::InitTypedRoot() {
  if (!IsTypedTemplate() || result_ != nullptr) {
    return;
  }

  result_ = CreateTypedRootElement(element_manager_, tasm_, typed_tag_);
  if (result_ == nullptr) {
    return;
  }
  result_->MarkTemplateElement();
  auto* root = element_manager_->root();
  if (root != nullptr) {
    result_->SetParentComponentUniqueIdRecursively(root->impl_id());
  }

  element_slot_targets_.clear();
  element_slot_targets_.push_back(ElementSlotMountPoint{result_, nullptr});
}

fml::RefPtr<Element> ElementTemplateInstance::GetRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_ELEMENT_GET_ROOT, "template_key",
              template_key_.str(), "bundle_url", bundle_url_.str());
  RequestMaterializationRecursively();
  MaterializeRoot();
  return result_;
}

fml::RefPtr<Element> ElementTemplateInstance::PeekMaterializedRoot() const {
  return result_;
}

void ElementTemplateInstance::ApplyAttributeSlotToTarget(
    uint32_t slot_index, const lepus::Value& previous_attribute_slots) {
  if (slot_index >= attribute_slot_targets_.size()) {
    return;
  }
  const auto& target = attribute_slot_targets_[slot_index];
  if (target == nullptr) {
    return;
  }
  TreeResolver::ApplyTemplateAttributesToElement(
      target.get(), previous_attribute_slots, attribute_slots_);
}

lepus::Value ElementTemplateInstance::GetOrCreateMutableChildSlot(
    uint32_t slot_index) {
  if (!child_slots_.IsArray()) {
    child_slots_ = lepus::Value(lepus::CArray::Create());
  }
  auto slot_children = child_slots_.GetProperty(slot_index);
  if (!slot_children.IsArray()) {
    slot_children = lepus::Value(lepus::CArray::Create());
    child_slots_.SetProperty(slot_index, slot_children);
  }
  return slot_children;
}

bool ElementTemplateInstance::EraseChildFromSlotStorage(
    uint32_t slot_index, const lepus::Value& child) {
  if (!child_slots_.IsArray() ||
      slot_index >= static_cast<uint32_t>(child_slots_.GetLength())) {
    return false;
  }
  auto slot_children = child_slots_.GetProperty(slot_index);
  if (!slot_children.IsArray()) {
    return false;
  }
  auto array = slot_children.Array();
  for (size_t i = 0; i < array->size(); ++i) {
    if (array->get(i).IsEqual(child)) {
      array->Erase(static_cast<uint32_t>(i));
      return true;
    }
  }
  return false;
}

void ElementTemplateInstance::ClearLogicalChildParentLinks() {
  if (!child_slots_.IsArrayOrJSArray()) {
    return;
  }
  for (size_t slot_index = 0;
       slot_index < static_cast<size_t>(child_slots_.GetLength());
       ++slot_index) {
    auto slot_children =
        child_slots_.GetProperty(static_cast<uint32_t>(slot_index));
    if (!slot_children.IsArrayOrJSArray()) {
      continue;
    }
    for (size_t child_index = 0;
         child_index < static_cast<size_t>(slot_children.GetLength());
         ++child_index) {
      auto child = ResolveElementTemplateInstanceValue(
          slot_children.GetProperty(static_cast<uint32_t>(child_index)));
      if (child != nullptr && child->logical_parent_ == this) {
        child->logical_parent_ = nullptr;
        child->logical_parent_slot_index_ = 0;
      }
    }
  }
}

lepus::Value ElementTemplateInstance::Serialize() const {
  return ElementTemplateInstanceSerializer().Serialize(*this);
}

void ElementTemplateInstance::SetAttributeSlot(uint32_t slot_index,
                                               const lepus::Value& value) {
  if (IsTypedTemplate()) {
    if (slot_index == kTypedTemplateRootAttributeSlotIndex) {
      SetRootAttributes(value);
    }
    return;
  }

  auto previous_attribute_slots = std::move(attribute_slots_);
  auto next_slots = lepus::CArray::Create();
  if (previous_attribute_slots.IsArrayOrJSArray()) {
    next_slots->reserve(previous_attribute_slots.GetLength());
    for (size_t index = 0;
         index < static_cast<size_t>(previous_attribute_slots.GetLength());
         ++index) {
      next_slots->emplace_back(
          previous_attribute_slots.GetProperty(static_cast<uint32_t>(index)));
    }
  }
  attribute_slots_ = lepus::Value(std::move(next_slots));
  attribute_slots_.SetProperty(slot_index, CopyTemplateValueForStorage(value));
  ++attribute_slots_generation_;
  if (IsMaterialized()) {
    ApplyAttributeSlotToTarget(slot_index, previous_attribute_slots);
  }
}

void ElementTemplateInstance::InsertNodeIntoChildSlot(
    uint32_t slot_index, const lepus::Value& child,
    const lepus::Value& ref_node) {
  auto child_instance = ResolveElementTemplateInstanceValue(child);
  if (child_instance == nullptr || child.IsEqual(ref_node)) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootChildSlotIndex) {
    return;
  }

  auto* previous_parent = child_instance->logical_parent_;
  auto previous_slot_index = child_instance->logical_parent_slot_index_;
  if (previous_parent != nullptr) {
    previous_parent->EraseChildFromSlotStorage(previous_slot_index, child);
  }

  auto slot_children = GetOrCreateMutableChildSlot(slot_index);
  auto insert_index = FindSlotChildIndex(slot_children, ref_node);
  slot_children.Array()->Insert(static_cast<uint32_t>(insert_index), child);
  child_instance->logical_parent_ = this;
  child_instance->logical_parent_slot_index_ = slot_index;
}

void ElementTemplateInstance::RemoveNodeFromChildSlot(
    uint32_t slot_index, const lepus::Value& child) {
  auto child_instance = ResolveElementTemplateInstanceValue(child);
  if (child_instance == nullptr) {
    return;
  }
  if (IsTypedTemplate() && slot_index != kTypedTemplateRootChildSlotIndex) {
    return;
  }
  if (!EraseChildFromSlotStorage(slot_index, child)) {
    return;
  }
  child_instance->logical_parent_ = nullptr;
  child_instance->logical_parent_slot_index_ = 0;
}

}  // namespace tasm
}  // namespace lynx
