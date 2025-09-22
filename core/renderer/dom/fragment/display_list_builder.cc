// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <utility>

namespace lynx {
namespace tasm {

DisplayListBuilder::DisplayListBuilder() = default;

DisplayListBuilder::~DisplayListBuilder() = default;

DisplayListBuilder& DisplayListBuilder::Begin(float x, float y, float width,
                                              float height) {
  // Use AddOperation directly to avoid temporary vector construction - only
  // store float params
  display_list_.AddOperation<DisplayListOpCategory::kContent>(
      DisplayListOpType::kBegin, x, y, width, height);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::End() {
  display_list_.AddOperation<DisplayListOpCategory::kContent>(
      DisplayListOpType::kEnd);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Fill(uint32_t color) {
  display_list_.AddOperation<DisplayListOpCategory::kContent>(
      DisplayListOpType::kFill, static_cast<int32_t>(color));
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawView(int view_id) {
  display_list_.AddOperation<DisplayListOpCategory::kContent>(
      DisplayListOpType::kDrawView, view_id);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Transform(float a, float b, float c,
                                                  float d, float e, float f) {
  // Use AddOperation directly to avoid temporary vector construction
  display_list_.AddOperation<DisplayListOpCategory::kSubtreeProperty>(
      DisplayListSubtreePropertyOpType::kTransform, a, b, c, d, e, f);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Clip(float x, float y, float width,
                                             float height) {
  // Use AddOperation directly to avoid temporary vector construction - only
  // store float params
  display_list_.AddOperation<DisplayListOpCategory::kSubtreeProperty>(
      DisplayListSubtreePropertyOpType::kClip, x, y, width, height);
  return *this;
}

DisplayList DisplayListBuilder::Build() { return std::move(display_list_); }

void DisplayListBuilder::Clear() { display_list_.Clear(); }

}  // namespace tasm
}  // namespace lynx
