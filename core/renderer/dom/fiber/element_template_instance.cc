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
                         instance.child_slots_);
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
                         instance.child_slots_);
    SetSerializedOptions(instance, serialized);
    SetSerializedUid(instance, serialized);
    return lepus::Value(std::move(serialized));
  }
};

ElementTemplateInstance::ElementTemplateInstance(
    ElementManager* element_manager)
    : bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {}

ElementTemplateInstance::~ElementTemplateInstance() = default;

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

void ElementTemplateInstance::SetOptions(const lepus::Value& options) {
  options_ = options;
}

void ElementTemplateInstance::SetUid(const lepus::Value& uid) { uid_ = uid; }

lepus::Value ElementTemplateInstance::Serialize() const {
  return ElementTemplateInstanceSerializer().Serialize(*this);
}

}  // namespace tasm
}  // namespace lynx
