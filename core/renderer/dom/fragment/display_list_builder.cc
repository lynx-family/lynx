// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <utility>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/starlight/style/borders_data.h"

namespace lynx {
namespace tasm {

DisplayListBuilder::DisplayListBuilder() = default;

DisplayListBuilder::~DisplayListBuilder() = default;

DisplayListBuilder& DisplayListBuilder::Begin(float x, float y, float width,
                                              float height) {
  // Use AddOperation directly to avoid temporary vector construction - only
  // store float params
  display_list_.AddOperation(DisplayListOpType::kBegin, x, y, width, height);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::End() {
  display_list_.AddOperation(DisplayListOpType::kEnd);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Fill(uint32_t color) {
  display_list_.AddOperation(DisplayListOpType::kFill,
                             static_cast<int32_t>(color));
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawView(int view_id) {
  display_list_.AddOperation(DisplayListOpType::kDrawView, view_id);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Transform(float a, float b, float c,
                                                  float d, float e, float f) {
  // Use AddOperation directly to avoid temporary vector construction
  display_list_.AddOperation(DisplayListSubtreePropertyOpType::kTransform, a, b,
                             c, d, e, f);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Clip(float x, float y, float width,
                                             float height) {
  // Use AddOperation directly to avoid temporary vector construction - only
  // store float params
  display_list_.AddOperation(DisplayListSubtreePropertyOpType::kClip, x, y,
                             width, height);
  return *this;
}

DisplayList DisplayListBuilder::Build() { return std::move(display_list_); }

void DisplayListBuilder::Clear() { display_list_ = DisplayList(); }

DisplayListBuilder& DisplayListBuilder::DrawImage(int image_id) {
  display_list_.AddOperation(DisplayListOpType::kImage, image_id);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawText(int text_id) {
  display_list_.AddOperation(DisplayListOpType::kText, text_id);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Border(
    const starlight::BordersData& border) {
  display_list_.AddOperation(DisplayListOpType::kBorder, border.width_top,
                             border.width_right, border.width_bottom,
                             border.width_left,
                             static_cast<int32_t>(border.color_top),
                             static_cast<int32_t>(border.color_right),
                             static_cast<int32_t>(border.color_bottom),
                             static_cast<int32_t>(border.color_left),
                             static_cast<int32_t>(border.style_top),
                             static_cast<int32_t>(border.style_right),
                             static_cast<int32_t>(border.style_bottom),
                             static_cast<int32_t>(border.style_left));
  return *this;
}

}  // namespace tasm
}  // namespace lynx
