// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/element_template_instance.h"

#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"

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
    : bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {}

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
  root_attributes_ = attributes.IsObject()
                         ? CopyTemplateObjectForStorage(attributes)
                         : lepus::Value();
  ++root_attributes_generation_;
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
