// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

#include <cstdint>

namespace lynx {
namespace tasm {

void DisplayList::Reserve(int32_t capacity) {
  constexpr static const int32_t kPreAllocatedCapacityForItems = 10;
  constexpr static const int32_t kPreAllocatedCapacityForData = 64;

  content_items_->reserve(capacity * kPreAllocatedCapacityForItems);
  content_data_->reserve(capacity * kPreAllocatedCapacityForData);
}

void DisplayList::Clear() {
  if (content_items_.has_value()) {
    content_items_->clear();
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

void DisplayList::AddRadialGradient(float center_x, float center_y,
                                    float radius_x, float radius_y,
                                    const base::Vector<uint32_t>& colors,
                                    const base::Vector<float>& stops,
                                    int32_t tiling_index, int32_t clip_index,
                                    int32_t repeat_x, int32_t repeat_y) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kRadialGradient;

  const uint32_t color_count = static_cast<uint32_t>(colors.size());
  const uint32_t stop_count = static_cast<uint32_t>(stops.size());
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

  auto& payload = item.payload.radial_gradient;
  payload.color_count_offset = color_offset;
  payload.color_count = color_count;
  payload.stop_count_offset = stop_offset;
  payload.stop_count = stop_count;
  payload.tiling_index = tiling_index;
  payload.clip_index = clip_index;
  payload.repeat_x = repeat_x;
  payload.repeat_y = repeat_y;
  payload.center_x = center_x;
  payload.center_y = center_y;
  payload.radius_x = radius_x;
  payload.radius_y = radius_y;
  AppendItem(item);
}

void DisplayList::AddBackgroundImage(const fml::RefPtr<PaintImage>& image,
                                     int32_t tiling_index, int32_t clip_index,
                                     int32_t repeat_x, int32_t repeat_y) {
  DisplayListItem item{};
  item.type = DisplayListOpType::kBackgroundImage;
  item.payload.background_image.image_id = image->image_key_;
  item.payload.background_image.tiling_index = tiling_index;
  item.payload.background_image.clip_index = clip_index;
  item.payload.background_image.repeat_x = repeat_x;
  item.payload.background_image.repeat_y = repeat_y;
  AppendItem(item);
  Images().emplace_back(image);
}

}  // namespace tasm
}  // namespace lynx
