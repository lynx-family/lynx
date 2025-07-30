// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/simple_styling/style_object.h"

#include <mutex>
#include <utility>

#include "core/renderer/simple_styling/simple_style_node.h"

namespace lynx::style {

void StyleObject::BindToElement(SimpleStyleNode* element) {}

void StyleObject::UnbindFromElement(SimpleStyleNode* element) {}

void StyleObject::ResetStylesInElement(SimpleStyleNode* element) const {
  for (const auto& pair : style_map_) {
    element->ResetSimpleStyle(pair.first);
  }
}

void StyleObject::FromBinary() {
  std::call_once(decode_flag_, [this]() { DecodeImmediately(); });
}

lepus::RefType StyleObject::GetRefType() const {
  return lepus::RefType::kStyleObject;
}

void StyleObject::DecodeImmediately() {
  // No decode needed for a predecoded StyleObject.
  if (!creator_) {
    return;
  }

  if (const auto decoder = creator_(data_, length_, string_list_); decoder) {
    decoder->DecodeStyleObject(style_map_, range_);
  }
}

void DynamicStyleObject::UpdateStyleMap(const tasm::StyleMap& style_map) {
  style_map_.merge(style_map);

  for (auto element : elements_) {
    element->UpdateSimpleStyles(style_map);
  }
}

void DynamicStyleObject::BindToElement(SimpleStyleNode* element) {
  elements_.emplace_back(element);
}

void DynamicStyleObject::UnbindFromElement(SimpleStyleNode* element) {
  if (auto it = std::find(elements_.begin(), elements_.end(), element);
      it != elements_.end()) {
    elements_.erase(it);
  }
}

void DynamicStyleObject::Reset() {
  for (auto* element : elements_) {
    ResetStylesInElement(element);
  }
}

std::unique_ptr<StyleObject*, StyleObjectArrayDeleter> CreateStyleObjectArray(
    int capacity) {
  auto* array =
      static_cast<StyleObject**>(malloc(capacity * sizeof(StyleObject*)));
  return std::unique_ptr<StyleObject*, StyleObjectArrayDeleter>(array);
}

}  // namespace lynx::style
