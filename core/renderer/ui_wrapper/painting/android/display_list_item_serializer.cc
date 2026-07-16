// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/display_list_item_serializer.h"

#include <cstring>

#include "base/include/compiler_specific.h"

namespace lynx::tasm {

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
constexpr int32_t kBackgroundImageIdOffset = 4;
constexpr int32_t kBackgroundImageTilingIndexOffset = 8;
constexpr int32_t kBackgroundImageClipIndexOffset = 12;
constexpr int32_t kBackgroundImageRepeatXOffset = 16;
constexpr int32_t kBackgroundImageRepeatYOffset = 20;
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
ALWAYS_INLINE void WriteField(uint8_t* data, size_t offset, T value) {
  static_assert(sizeof(T) == sizeof(int32_t),
                "DisplayList serialized fields are 32-bit values");
  std::memcpy(data + offset, &value, sizeof(T));
}

template <typename T, size_t N>
ALWAYS_INLINE void WriteArray(uint8_t* data, size_t offset,
                              const T (&values)[N]) {
  static_assert(sizeof(T) == sizeof(int32_t),
                "DisplayList serialized fields are 32-bit values");
  std::memcpy(data + offset, values, sizeof(values));
}

void SerializeDisplayListItemForAndroid(const DisplayListItem& item,
                                        uint8_t* item_data) {
  WriteField(item_data, 0, static_cast<int32_t>(item.type));

  switch (item.type) {
    case DisplayListOpType::kBegin:
      WriteField(item_data, kBeginIdOffset, item.payload.begin.id);
      WriteField(item_data, kBeginTypeOffset, item.payload.begin.type);
      WriteField(item_data, kBeginXOffset, item.payload.begin.x);
      WriteField(item_data, kBeginYOffset, item.payload.begin.y);
      WriteField(item_data, kBeginWOffset, item.payload.begin.w);
      WriteField(item_data, kBeginHOffset, item.payload.begin.h);
      break;
    case DisplayListOpType::kFill:
      WriteField(item_data, kFillColorOffset, item.payload.fill.color);
      WriteField(item_data, kFillClipIndexOffset, item.payload.fill.clip_index);
      break;
    case DisplayListOpType::kDrawView:
      WriteField(item_data, kDrawViewIdOffset, item.payload.draw_view.view_id);
      break;
    case DisplayListOpType::kText:
      WriteField(item_data, kTextIdOffset, item.payload.text.text_id);
      WriteField(item_data, kTextBoxIndexOffset, item.payload.text.box_index);
      break;
    case DisplayListOpType::kImage:
      WriteField(item_data, kImageIdOffset, item.payload.image.image_id);
      WriteField(item_data, kImageBoxIndexOffset, item.payload.image.box_index);
      break;
    case DisplayListOpType::kBackgroundImage:
      WriteField(item_data, kBackgroundImageIdOffset,
                 item.payload.background_image.image_id);
      WriteField(item_data, kBackgroundImageTilingIndexOffset,
                 item.payload.background_image.tiling_index);
      WriteField(item_data, kBackgroundImageClipIndexOffset,
                 item.payload.background_image.clip_index);
      WriteField(item_data, kBackgroundImageRepeatXOffset,
                 item.payload.background_image.repeat_x);
      WriteField(item_data, kBackgroundImageRepeatYOffset,
                 item.payload.background_image.repeat_y);
      break;
    case DisplayListOpType::kBorder:
      WriteField(item_data, kBorderOutIndexOffset,
                 item.payload.border.out_index);
      WriteField(item_data, kBorderInnerIndexOffset,
                 item.payload.border.inner_index);
      WriteArray(item_data, kBorderColorsOffset, item.payload.border.colors);
      WriteArray(item_data, kBorderStylesOffset, item.payload.border.styles);
      break;
    case DisplayListOpType::kClipRect:
      WriteField(item_data, kClipRectXOffset, item.payload.clip_rect.x);
      WriteField(item_data, kClipRectYOffset, item.payload.clip_rect.y);
      WriteField(item_data, kClipRectWOffset, item.payload.clip_rect.w);
      WriteField(item_data, kClipRectHOffset, item.payload.clip_rect.h);
      WriteArray(item_data, kClipRectRadiiOffset, item.payload.clip_rect.radii);
      WriteField(item_data, kClipRectHasRadiiOffset,
                 item.payload.clip_rect.has_radii);
      break;
    case DisplayListOpType::kRecordBox:
      WriteField(item_data, kRecordBoxXOffset, item.payload.record_box.x);
      WriteField(item_data, kRecordBoxYOffset, item.payload.record_box.y);
      WriteField(item_data, kRecordBoxWOffset, item.payload.record_box.w);
      WriteField(item_data, kRecordBoxHOffset, item.payload.record_box.h);
      WriteArray(item_data, kRecordBoxRadiiOffset,
                 item.payload.record_box.radii);
      WriteField(item_data, kRecordBoxHasRadiiOffset,
                 item.payload.record_box.has_radii);
      break;
    case DisplayListOpType::kLinearGradient:
      WriteField(item_data, kGradientColorCountOffsetOffset,
                 item.payload.linear_gradient.color_count_offset);
      WriteField(item_data, kGradientColorCountOffset,
                 item.payload.linear_gradient.color_count);
      WriteField(item_data, kGradientStopCountOffsetOffset,
                 item.payload.linear_gradient.stop_count_offset);
      WriteField(item_data, kGradientStopCountOffset,
                 item.payload.linear_gradient.stop_count);
      WriteField(item_data, kGradientTilingIndexOffset,
                 item.payload.linear_gradient.tiling_index);
      WriteField(item_data, kGradientClipIndexOffset,
                 item.payload.linear_gradient.clip_index);
      WriteField(item_data, kGradientRepeatXOffset,
                 item.payload.linear_gradient.repeat_x);
      WriteField(item_data, kGradientRepeatYOffset,
                 item.payload.linear_gradient.repeat_y);
      WriteField(item_data, kGradientAngleOffset,
                 item.payload.linear_gradient.angle);
      break;
    case DisplayListOpType::kBoxShadow:
      WriteField(item_data, kBoxShadowShadowBoxIndexOffset,
                 item.payload.box_shadow.shadow_box_index);
      WriteField(item_data, kBoxShadowClipBoxIndexOffset,
                 item.payload.box_shadow.clip_box_index);
      WriteField(item_data, kBoxShadowColorOffset,
                 item.payload.box_shadow.color);
      WriteField(item_data, kBoxShadowBlurRadiusOffset,
                 item.payload.box_shadow.blur_radius);
      WriteField(item_data, kBoxShadowClipModeOffset,
                 item.payload.box_shadow.clip_mode);
      break;
    case DisplayListOpType::kEnd:
    case DisplayListOpType::kCustom:
      break;
  }
}

}  // namespace

void SerializeDisplayListItemsForAndroid(const DisplayListItem* items,
                                         size_t item_count,
                                         base::Vector<uint8_t>& buffer) {
  if (items == nullptr || item_count == 0) {
    buffer.clear();
    return;
  }

  const size_t item_size =
      static_cast<size_t>(kSerializedAndroidDisplayListItemSize);
  const size_t buffer_size =
      kAndroidDisplayListItemBufferHeaderSize + item_count * item_size;
  buffer.resize<false>(buffer_size);

  uint8_t* data = buffer.data();
  WriteField(data, 0, kSerializedAndroidDisplayListItemSize);

  uint8_t* item_data = data + kAndroidDisplayListItemBufferHeaderSize;
  for (size_t i = 0; i < item_count; ++i, item_data += item_size) {
    SerializeDisplayListItemForAndroid(items[i], item_data);
  }
}

}  // namespace lynx::tasm
