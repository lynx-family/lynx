// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

#include <cstdint>
#include <cstring>

namespace lynx {
namespace tasm {

namespace {

constexpr int32_t kBeginIdOffset = 4;
constexpr int32_t kBeginTypeOffset = 8;
constexpr int32_t kBeginXOffset = 12;
constexpr int32_t kBeginYOffset = 16;
constexpr int32_t kBeginWOffset = 20;
constexpr int32_t kBeginHOffset = 24;
constexpr int32_t kFillColorOffset = 4;
constexpr int32_t kFillClipIndexOffset = 8;
constexpr int32_t kDrawViewIdOffset = 4;
constexpr int32_t kTextIdOffset = 4;
constexpr int32_t kTextBoxIndexOffset = 8;
constexpr int32_t kImageIdOffset = 4;
constexpr int32_t kImageBoxIndexOffset = 8;
constexpr int32_t kBorderOutIndexOffset = 4;
constexpr int32_t kBorderInnerIndexOffset = 8;
constexpr int32_t kBorderColorsOffset = 12;
constexpr int32_t kBorderStylesOffset = 28;
constexpr int32_t kRecordBoxXOffset = 4;
constexpr int32_t kRecordBoxYOffset = 8;
constexpr int32_t kRecordBoxWOffset = 12;
constexpr int32_t kRecordBoxHOffset = 16;
constexpr int32_t kRecordBoxRadiiOffset = 20;
constexpr int32_t kRecordBoxHasRadiiOffset = 52;
constexpr int32_t kClipRectXOffset = 4;
constexpr int32_t kClipRectYOffset = 8;
constexpr int32_t kClipRectWOffset = 12;
constexpr int32_t kClipRectHOffset = 16;
constexpr int32_t kClipRectRadiiOffset = 20;
constexpr int32_t kClipRectHasRadiiOffset = 52;
constexpr int32_t kGradientColorCountOffsetOffset = 4;
constexpr int32_t kGradientColorCountOffset = 8;
constexpr int32_t kGradientStopCountOffsetOffset = 12;
constexpr int32_t kGradientStopCountOffset = 16;
constexpr int32_t kGradientTilingIndexOffset = 20;
constexpr int32_t kGradientClipIndexOffset = 24;
constexpr int32_t kGradientRepeatXOffset = 28;
constexpr int32_t kGradientRepeatYOffset = 32;
constexpr int32_t kGradientAngleOffset = 36;
constexpr int32_t kBoxShadowShadowBoxIndexOffset = 4;
constexpr int32_t kBoxShadowClipBoxIndexOffset = 8;
constexpr int32_t kBoxShadowColorOffset = 12;
constexpr int32_t kBoxShadowBlurRadiusOffset = 16;
constexpr int32_t kBoxShadowClipModeOffset = 20;

template <typename T>
void WriteField(base::Vector<uint8_t>& buffer, size_t offset, T value) {
  static_assert(sizeof(T) == sizeof(int32_t),
                "DisplayList serialized fields are 32-bit values");
  std::memcpy(buffer.data() + offset, &value, sizeof(T));
}

}  // namespace

void DisplayList::Reserve(int32_t capacity) {
  constexpr static const int32_t kPreAllocatedCapacityForItems = 10;
  constexpr static const int32_t kPreAllocatedCapacityForData = 64;

  content_items_->reserve(capacity * kPreAllocatedCapacityForItems);
  serialized_content_items_->reserve(kDisplayListItemBufferHeaderSize +
                                     capacity * kSerializedDisplayListItemSize);
  content_data_->reserve(capacity * kPreAllocatedCapacityForData);
}

void DisplayList::Clear() {
  if (content_items_.has_value()) {
    content_items_->clear();
  }
  if (serialized_content_items_.has_value()) {
    serialized_content_items_->clear();
    serialized_content_items_.reset();
  }
  if (content_data_.has_value()) {
    content_data_->clear();
  }
  ClearSubtreeProperties();
}

void DisplayList::ClearSubtreeProperties() {
  if (subtree_properties_.has_value()) {
    subtree_properties_->clear();
    subtree_properties_.reset();
  }
}

void DisplayList::AppendItem(const DisplayListItem& item) {
  content_items_->push_back(item);
  SerializeItem(item, *serialized_content_items_);
}

void DisplayList::SerializeItem(const DisplayListItem& item,
                                base::Vector<uint8_t>& buffer) {
  if (buffer.empty()) {
    buffer.resize<true>(kDisplayListItemBufferHeaderSize);
    WriteField(buffer, 0, kSerializedDisplayListItemSize);
  }

  const size_t item_offset = buffer.size();
  buffer.resize<true>(item_offset + kSerializedDisplayListItemSize);
  WriteField(buffer, item_offset, static_cast<int32_t>(item.type));

  switch (item.type) {
    case DisplayListOpType::kBegin:
      WriteField(buffer, item_offset + kBeginIdOffset, item.payload.begin.id);
      WriteField(buffer, item_offset + kBeginTypeOffset,
                 item.payload.begin.type);
      WriteField(buffer, item_offset + kBeginXOffset, item.payload.begin.x);
      WriteField(buffer, item_offset + kBeginYOffset, item.payload.begin.y);
      WriteField(buffer, item_offset + kBeginWOffset, item.payload.begin.w);
      WriteField(buffer, item_offset + kBeginHOffset, item.payload.begin.h);
      break;
    case DisplayListOpType::kFill:
      WriteField(buffer, item_offset + kFillColorOffset,
                 item.payload.fill.color);
      WriteField(buffer, item_offset + kFillClipIndexOffset,
                 item.payload.fill.clip_index);
      break;
    case DisplayListOpType::kDrawView:
      WriteField(buffer, item_offset + kDrawViewIdOffset,
                 item.payload.draw_view.view_id);
      break;
    case DisplayListOpType::kText:
      WriteField(buffer, item_offset + kTextIdOffset,
                 item.payload.text.text_id);
      WriteField(buffer, item_offset + kTextBoxIndexOffset,
                 item.payload.text.box_index);
      break;
    case DisplayListOpType::kImage:
      WriteField(buffer, item_offset + kImageIdOffset,
                 item.payload.image.image_id);
      WriteField(buffer, item_offset + kImageBoxIndexOffset,
                 item.payload.image.box_index);
      break;
    case DisplayListOpType::kBorder:
      WriteField(buffer, item_offset + kBorderOutIndexOffset,
                 item.payload.border.out_index);
      WriteField(buffer, item_offset + kBorderInnerIndexOffset,
                 item.payload.border.inner_index);
      for (int i = 0; i < 4; ++i) {
        WriteField(buffer, item_offset + kBorderColorsOffset + i * 4,
                   item.payload.border.colors[i]);
        WriteField(buffer, item_offset + kBorderStylesOffset + i * 4,
                   item.payload.border.styles[i]);
      }
      break;
    case DisplayListOpType::kClipRect:
      WriteField(buffer, item_offset + kClipRectXOffset,
                 item.payload.clip_rect.x);
      WriteField(buffer, item_offset + kClipRectYOffset,
                 item.payload.clip_rect.y);
      WriteField(buffer, item_offset + kClipRectWOffset,
                 item.payload.clip_rect.w);
      WriteField(buffer, item_offset + kClipRectHOffset,
                 item.payload.clip_rect.h);
      for (int i = 0; i < 8; ++i) {
        WriteField(buffer, item_offset + kClipRectRadiiOffset + i * 4,
                   item.payload.clip_rect.radii[i]);
      }
      WriteField(buffer, item_offset + kClipRectHasRadiiOffset,
                 item.payload.clip_rect.has_radii);
      break;
    case DisplayListOpType::kRecordBox:
      WriteField(buffer, item_offset + kRecordBoxXOffset,
                 item.payload.record_box.x);
      WriteField(buffer, item_offset + kRecordBoxYOffset,
                 item.payload.record_box.y);
      WriteField(buffer, item_offset + kRecordBoxWOffset,
                 item.payload.record_box.w);
      WriteField(buffer, item_offset + kRecordBoxHOffset,
                 item.payload.record_box.h);
      for (int i = 0; i < 8; ++i) {
        WriteField(buffer, item_offset + kRecordBoxRadiiOffset + i * 4,
                   item.payload.record_box.radii[i]);
      }
      WriteField(buffer, item_offset + kRecordBoxHasRadiiOffset,
                 item.payload.record_box.has_radii);
      break;
    case DisplayListOpType::kLinearGradient:
      WriteField(buffer, item_offset + kGradientColorCountOffsetOffset,
                 item.payload.linear_gradient.color_count_offset);
      WriteField(buffer, item_offset + kGradientColorCountOffset,
                 item.payload.linear_gradient.color_count);
      WriteField(buffer, item_offset + kGradientStopCountOffsetOffset,
                 item.payload.linear_gradient.stop_count_offset);
      WriteField(buffer, item_offset + kGradientStopCountOffset,
                 item.payload.linear_gradient.stop_count);
      WriteField(buffer, item_offset + kGradientTilingIndexOffset,
                 item.payload.linear_gradient.tiling_index);
      WriteField(buffer, item_offset + kGradientClipIndexOffset,
                 item.payload.linear_gradient.clip_index);
      WriteField(buffer, item_offset + kGradientRepeatXOffset,
                 item.payload.linear_gradient.repeat_x);
      WriteField(buffer, item_offset + kGradientRepeatYOffset,
                 item.payload.linear_gradient.repeat_y);
      WriteField(buffer, item_offset + kGradientAngleOffset,
                 item.payload.linear_gradient.angle);
      break;
    case DisplayListOpType::kBoxShadow:
      WriteField(buffer, item_offset + kBoxShadowShadowBoxIndexOffset,
                 item.payload.box_shadow.shadow_box_index);
      WriteField(buffer, item_offset + kBoxShadowClipBoxIndexOffset,
                 item.payload.box_shadow.clip_box_index);
      WriteField(buffer, item_offset + kBoxShadowColorOffset,
                 item.payload.box_shadow.color);
      WriteField(buffer, item_offset + kBoxShadowBlurRadiusOffset,
                 item.payload.box_shadow.blur_radius);
      WriteField(buffer, item_offset + kBoxShadowClipModeOffset,
                 item.payload.box_shadow.clip_mode);
      break;
    case DisplayListOpType::kEnd:
    case DisplayListOpType::kCustom:
      break;
  }
}

void DisplayList::AddLinearGradient(float angle,
                                    const base::Vector<uint32_t>& colors,
                                    const base::Vector<float>& stops,
                                    int32_t tiling_index, int32_t clip_index,
                                    int32_t repeat_x, int32_t repeat_y) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kLinearGradient;

  uint32_t color_count = static_cast<uint32_t>(colors.size());
  uint32_t stop_count = static_cast<uint32_t>(stops.size());

  // Append colors and stops to the trailing data region
  uint32_t color_offset = 0;
  uint32_t stop_offset = 0;

  if (color_count > 0) {
    color_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(colors.data()),
                          colors.size() * sizeof(uint32_t));
  }

  if (stop_count > 0) {
    stop_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(stops.data()),
                          stops.size() * sizeof(float));
  }

  item.payload.linear_gradient.color_count_offset = color_offset;
  item.payload.linear_gradient.color_count = color_count;
  item.payload.linear_gradient.stop_count_offset = stop_offset;
  item.payload.linear_gradient.stop_count = stop_count;
  item.payload.linear_gradient.tiling_index = tiling_index;
  item.payload.linear_gradient.clip_index = clip_index;
  item.payload.linear_gradient.repeat_x = repeat_x;
  item.payload.linear_gradient.repeat_y = repeat_y;
  item.payload.linear_gradient.angle = angle;

  AppendItem(item);
}

}  // namespace tasm
}  // namespace lynx
