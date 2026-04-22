// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/template_element.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/value/base_value.h"

namespace lynx {
namespace tasm {
namespace {

// These keys define the serialized Element Template payload consumed by
// FiberSerializeElementTemplate / FiberCreateElementTemplate.
static constexpr const char kTemplateTag[] = "template";
static constexpr const char kDefaultTemplateBundleUrl[] = "__Card__";
static constexpr const char kTemplateKey[] = "templateKey";
static constexpr const char kTemplateBundleUrl[] = "bundleUrl";
static constexpr const char kTemplateAttributeSlots[] = "attributeSlots";
static constexpr const char kTemplateElementSlots[] = "elementSlots";
static constexpr const char kTemplateUid[] = "uid";

}  // namespace

TemplateElement::TemplateElement(ElementManager* element_manager)
    : FiberElement(element_manager, BASE_STATIC_STRING(kTemplateTag)),
      bundle_url_(BASE_STATIC_STRING(kDefaultTemplateBundleUrl)) {
  MarkTemplateElement();
}

TemplateElement::~TemplateElement() = default;

void TemplateElement::PrepareAsyncCreateElementTree() {
  // The async materialization flow is connected in the final integration
  // commit.
}

fml::RefPtr<FiberElement> TemplateElement::GetRoot() {
  // The materialized root is produced once the final integration commit wires
  // this shell to TreeResolver and ElementManager.
  return nullptr;
}

lepus::Value TemplateElement::Serialize() const {
  auto serialized = lepus::Dictionary::Create();
  serialized->SetValue(BASE_STATIC_STRING(kTemplateKey), template_key_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateBundleUrl), bundle_url_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateAttributeSlots),
                       attribute_slots_);
  serialized->SetValue(BASE_STATIC_STRING(kTemplateElementSlots),
                       SerializeElementSlots());
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

}  // namespace tasm
}  // namespace lynx
