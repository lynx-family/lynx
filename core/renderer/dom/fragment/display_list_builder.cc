// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <cstring>
#include <utility>

#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/starlight/style/borders_data.h"
#include "core/style/transform/matrix44.h"

namespace lynx {
namespace tasm {

DisplayListBuilder::DisplayListBuilder(float dx, float dy)
    : display_list_(dx, dy) {}

DisplayListBuilder::~DisplayListBuilder() = default;

void DisplayListBuilder::Reserve(int32_t capacity) {
  display_list_.Reserve(capacity);
}

DisplayListBuilder& DisplayListBuilder::Begin(int id, float x, float y,
                                              float width, float height) {
  // Use AddOperation directly to avoid temporary vector construction - only
  // store float params
  display_list_.AddOperation(DisplayListOpType::kBegin, id, x, y, width,
                             height);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::End() {
  display_list_.AddOperation(DisplayListOpType::kEnd);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Fill(uint32_t color,
                                             int32_t clip_index) {
  display_list_.AddOperation(DisplayListOpType::kFill,
                             static_cast<int32_t>(color), clip_index);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawView(int view_id) {
  display_list_.AddOperation(DisplayListOpType::kDrawView, view_id);
  display_list_.AddSubLayer(view_id);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Transform(
    const transforms::Matrix44& matrix) {
  SubtreeProperty prop;
  prop.type = DisplayListSubtreePropertyOpType::kTransform;
  // Matrix44 uses column-major storage (fMat[col][row]), which matches
  // what Renderer.java expects
  std::memcpy(prop.data.transform, matrix.Data(), sizeof(float) * 16);
  display_list_.AddSubtreeProperty(prop);
  return *this;
}

DisplayList DisplayListBuilder::Build() { return std::move(display_list_); }

void DisplayListBuilder::Clear() { display_list_ = DisplayList(); }

DisplayListBuilder& DisplayListBuilder::DrawImage(int32_t image_id,
                                                  int32_t box_index) {
  display_list_.AddOperation(DisplayListOpType::kImage, image_id, box_index);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawText(int text_id) {
  display_list_.AddOperation(DisplayListOpType::kText, text_id);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Border(
    int32_t out_index, int32_t inner_index,
    const starlight::BordersData& border) {
  display_list_.AddOperation(DisplayListOpType::kBorder, out_index, inner_index,
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

// Set Clip Rect
DisplayListBuilder& DisplayListBuilder::ClipRect(const RoundedRectangle& rect) {
  if (rect.HasRadius()) {
    display_list_.AddOperation(
        DisplayListOpType::kClipRect, rect.GetX(), rect.GetY(), rect.GetWidth(),
        rect.GetHeight(), rect.GetRadiusXTopLeft(), rect.GetRadiusYTopLeft(),
        rect.GetRadiusXTopRight(), rect.GetRadiusYTopRight(),
        rect.GetRadiusXBottomRight(), rect.GetRadiusYBottomRight(),
        rect.GetRadiusXBottomLeft(), rect.GetRadiusYBottomLeft());
  } else {
    display_list_.AddOperation(DisplayListOpType::kClipRect, rect.GetX(),
                               rect.GetY(), rect.GetWidth(), rect.GetHeight());
  }

  return *this;
}

DisplayListBuilder& DisplayListBuilder::RecordBoxModel(
    const RoundedRectangle& rect, int32_t& index) {
  if (rect.HasRadius()) {
    display_list_.AddOperation(
        DisplayListOpType::kRecordBox, rect.GetX(), rect.GetY(),
        rect.GetWidth(), rect.GetHeight(), rect.GetRadiusXTopLeft(),
        rect.GetRadiusYTopLeft(), rect.GetRadiusXTopRight(),
        rect.GetRadiusYTopRight(), rect.GetRadiusXBottomRight(),
        rect.GetRadiusYBottomRight(), rect.GetRadiusXBottomLeft(),
        rect.GetRadiusYBottomLeft());
  } else {
    display_list_.AddOperation(DisplayListOpType::kRecordBox, rect.GetX(),
                               rect.GetY(), rect.GetWidth(), rect.GetHeight());
  }

  index = current_index_of_box_model++;
  return *this;
}

DisplayListBuilder& DisplayListBuilder::LinearGradient(
    float angle, const base::Vector<uint32_t>& colors,
    const base::Vector<float>& stops, int32_t tiling_index, int32_t clip_index,
    int32_t repeat_x, int32_t repeat_y) {
  display_list_.AddLinearGradient(angle, colors, stops, tiling_index,
                                  clip_index, repeat_x, repeat_y);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::MarkRootNeedClipBounds() {
  display_list_.MarkRootNeedClipBounds();
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Opacity(float alpha) {
  display_list_.AddSubtreeProperty(
      (SubtreeProperty){.type = DisplayListSubtreePropertyOpType::kOpacity,
                        .data.opacity = alpha});
  return *this;
}

}  // namespace tasm
}  // namespace lynx
