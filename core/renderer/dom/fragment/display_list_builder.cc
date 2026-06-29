// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list_builder.h"

#include <cstdint>
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

DisplayListBuilder& DisplayListBuilder::Begin(int id, PlatformRendererType type,
                                              float x, float y, float width,
                                              float height) {
  display_list_.AddOperation(DisplayListOpType::kBegin, id,
                             static_cast<int32_t>(type), x, y, width, height);
  DisplayListItem item{};
  item.type = DisplayListOpType::kBegin;
  item.payload.begin.id = id;
  item.payload.begin.type = static_cast<int32_t>(type);
  item.payload.begin.x = x;
  item.payload.begin.y = y;
  item.payload.begin.w = width;
  item.payload.begin.h = height;
  display_list_.AppendItem(item);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::End() {
  display_list_.AddOperation(DisplayListOpType::kEnd);
  DisplayListItem item{};
  item.type = DisplayListOpType::kEnd;
  display_list_.AppendItem(item);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::Fill(uint32_t color,
                                             int32_t clip_index) {
  display_list_.AddOperation(DisplayListOpType::kFill,
                             static_cast<int32_t>(color), clip_index);
  DisplayListItem item{};
  item.type = DisplayListOpType::kFill;
  item.payload.fill.color = color;
  item.payload.fill.clip_index = clip_index;
  display_list_.AppendItem(item);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawView(int view_id) {
  display_list_.AddOperation(DisplayListOpType::kDrawView, view_id);
  DisplayListItem item{};
  item.type = DisplayListOpType::kDrawView;
  item.payload.draw_view.view_id = view_id;
  display_list_.AppendItem(item);
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
  DisplayListItem item{};
  item.type = DisplayListOpType::kImage;
  item.payload.image.image_id = image_id;
  item.payload.image.box_index = box_index;
  display_list_.AppendItem(item);
  return *this;
}

DisplayListBuilder& DisplayListBuilder::DrawText(int text_id,
                                                 int32_t box_index) {
  display_list_.AddOperation(DisplayListOpType::kText, text_id, box_index);
  DisplayListItem item{};
  item.type = DisplayListOpType::kText;
  item.payload.text.text_id = text_id;
  item.payload.text.box_index = box_index;
  display_list_.AppendItem(item);
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
  DisplayListItem item{};
  item.type = DisplayListOpType::kBorder;
  item.payload.border.out_index = out_index;
  item.payload.border.inner_index = inner_index;
  item.payload.border.colors[0] = static_cast<uint32_t>(border.color_top);
  item.payload.border.colors[1] = static_cast<uint32_t>(border.color_right);
  item.payload.border.colors[2] = static_cast<uint32_t>(border.color_bottom);
  item.payload.border.colors[3] = static_cast<uint32_t>(border.color_left);
  item.payload.border.styles[0] = static_cast<int32_t>(border.style_top);
  item.payload.border.styles[1] = static_cast<int32_t>(border.style_right);
  item.payload.border.styles[2] = static_cast<int32_t>(border.style_bottom);
  item.payload.border.styles[3] = static_cast<int32_t>(border.style_left);
  display_list_.AppendItem(item);
  return *this;
}

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
  DisplayListItem item{};
  item.type = DisplayListOpType::kClipRect;
  item.payload.clip_rect.x = rect.GetX();
  item.payload.clip_rect.y = rect.GetY();
  item.payload.clip_rect.w = rect.GetWidth();
  item.payload.clip_rect.h = rect.GetHeight();
  if (rect.HasRadius()) {
    item.payload.clip_rect.has_radii = 1;
    item.payload.clip_rect.radii[0] = rect.GetRadiusXTopLeft();
    item.payload.clip_rect.radii[1] = rect.GetRadiusYTopLeft();
    item.payload.clip_rect.radii[2] = rect.GetRadiusXTopRight();
    item.payload.clip_rect.radii[3] = rect.GetRadiusYTopRight();
    item.payload.clip_rect.radii[4] = rect.GetRadiusXBottomRight();
    item.payload.clip_rect.radii[5] = rect.GetRadiusYBottomRight();
    item.payload.clip_rect.radii[6] = rect.GetRadiusXBottomLeft();
    item.payload.clip_rect.radii[7] = rect.GetRadiusYBottomLeft();
  } else {
    item.payload.clip_rect.has_radii = 0;
  }
  display_list_.AppendItem(item);
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
  DisplayListItem item{};
  item.type = DisplayListOpType::kRecordBox;
  item.payload.record_box.x = rect.GetX();
  item.payload.record_box.y = rect.GetY();
  item.payload.record_box.w = rect.GetWidth();
  item.payload.record_box.h = rect.GetHeight();
  if (rect.HasRadius()) {
    item.payload.record_box.has_radii = 1;
    item.payload.record_box.radii[0] = rect.GetRadiusXTopLeft();
    item.payload.record_box.radii[1] = rect.GetRadiusYTopLeft();
    item.payload.record_box.radii[2] = rect.GetRadiusXTopRight();
    item.payload.record_box.radii[3] = rect.GetRadiusYTopRight();
    item.payload.record_box.radii[4] = rect.GetRadiusXBottomRight();
    item.payload.record_box.radii[5] = rect.GetRadiusYBottomRight();
    item.payload.record_box.radii[6] = rect.GetRadiusXBottomLeft();
    item.payload.record_box.radii[7] = rect.GetRadiusYBottomLeft();
  } else {
    item.payload.record_box.has_radii = 0;
  }
  display_list_.AppendItem(item);
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

DisplayListBuilder& DisplayListBuilder::BoxShadow(int32_t shadow_box_index,
                                                  int32_t clip_box_index,
                                                  uint32_t color,
                                                  float blur_radius,
                                                  BoxShadowClipMode clip_mode) {
  display_list_.AddOperation(DisplayListOpType::kBoxShadow, shadow_box_index,
                             clip_box_index, static_cast<int32_t>(color),
                             blur_radius, static_cast<int32_t>(clip_mode));
  DisplayListItem item{};
  item.type = DisplayListOpType::kBoxShadow;
  item.payload.box_shadow.shadow_box_index = shadow_box_index;
  item.payload.box_shadow.clip_box_index = clip_box_index;
  item.payload.box_shadow.color = color;
  item.payload.box_shadow.blur_radius = blur_radius;
  item.payload.box_shadow.clip_mode = static_cast<int32_t>(clip_mode);
  display_list_.AppendItem(item);
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
