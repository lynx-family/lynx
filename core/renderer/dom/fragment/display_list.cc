// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace lynx {
namespace tasm {

void DisplayList::Reserve(int32_t capacity) {
  constexpr static const int32_t kPreAllocatedCapacityForItems = 10;
  constexpr static const int32_t kPreAllocatedCapacityForData = 64;
  constexpr static const int32_t kPreAllocatedCapacityForOps = 10;
  constexpr static const int32_t kPreAllocatedCapacityForIntData = 20;
  constexpr static const int32_t kPreAllocatedCapacityForFloatData = 20;

  content_items_->reserve(capacity * kPreAllocatedCapacityForItems);
  content_data_->reserve(capacity * kPreAllocatedCapacityForData);

  OpData& legacy_op_data = *legacy_content_data_;
  legacy_op_data.ops.reserve(capacity * kPreAllocatedCapacityForOps);
  legacy_op_data.int_data.reserve(capacity * kPreAllocatedCapacityForIntData);
  legacy_op_data.float_data.reserve(capacity *
                                    kPreAllocatedCapacityForFloatData);
}

void DisplayList::Clear() {
  if (content_items_.has_value()) {
    content_items_->clear();
    content_items_.reset();
  }
  if (content_data_.has_value()) {
    content_data_->clear();
    content_data_.reset();
  }
  if (legacy_content_data_.has_value()) {
    legacy_content_data_->ops.clear();
    legacy_content_data_->int_data.clear();
    legacy_content_data_->float_data.clear();
    legacy_content_data_.reset();
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
  OpData& legacy_op_data = *legacy_content_data_;
  legacy_op_data.ops.push_back(
      static_cast<int32_t>(DisplayListOpType::kLinearGradient));

  int32_t color_count = static_cast<int32_t>(colors.size());
  int32_t stop_count = static_cast<int32_t>(stops.size());
  int32_t int_count = 1 + color_count + 1 + 4;
  int32_t float_count = 1 + stop_count;

  legacy_op_data.int_data.reserve(legacy_op_data.int_data.size() + 2 +
                                  int_count);
  legacy_op_data.int_data.push_back(int_count);
  legacy_op_data.int_data.push_back(float_count);
  legacy_op_data.int_data.push_back(color_count);
  if (color_count > 0) {
    legacy_op_data.int_data.append(colors.data(),
                                   colors.size() * sizeof(uint32_t));
  }
  legacy_op_data.int_data.push_back(stop_count);
  legacy_op_data.int_data.push_back(tiling_index);
  legacy_op_data.int_data.push_back(clip_index);
  legacy_op_data.int_data.push_back(repeat_x);
  legacy_op_data.int_data.push_back(repeat_y);

  legacy_op_data.float_data.reserve(legacy_op_data.float_data.size() +
                                    float_count);
  legacy_op_data.float_data.push_back(angle);
  if (stop_count > 0) {
    legacy_op_data.float_data.append(stops.data(),
                                     stops.size() * sizeof(float));
  }

  DisplayListItem item{};
  item.type = DisplayListOpType::kLinearGradient;

  uint32_t item_color_count = static_cast<uint32_t>(colors.size());
  uint32_t item_stop_count = static_cast<uint32_t>(stops.size());

  // Append colors and stops to the trailing data region
  uint32_t color_offset = 0;
  uint32_t stop_offset = 0;

  if (item_color_count > 0) {
    color_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(colors.data()),
                          colors.size() * sizeof(uint32_t));
  }

  if (item_stop_count > 0) {
    stop_offset = static_cast<uint32_t>(content_data_->size());
    content_data_->append(reinterpret_cast<const uint8_t*>(stops.data()),
                          stops.size() * sizeof(float));
  }

  item.payload.linear_gradient.color_count_offset = color_offset;
  item.payload.linear_gradient.color_count = item_color_count;
  item.payload.linear_gradient.stop_count_offset = stop_offset;
  item.payload.linear_gradient.stop_count = item_stop_count;
  item.payload.linear_gradient.tiling_index = tiling_index;
  item.payload.linear_gradient.clip_index = clip_index;
  item.payload.linear_gradient.repeat_x = repeat_x;
  item.payload.linear_gradient.repeat_y = repeat_y;
  item.payload.linear_gradient.angle = angle;

  content_items_->push_back(item);
}

}  // namespace tasm
}  // namespace lynx
