// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/display_list.h"

namespace lynx {
namespace tasm {

void DisplayList::Reserve(int32_t capacity) {
  constexpr static const int32_t kPreAllocatedCapacityForOps = 10;
  constexpr static const int32_t kPreAllocatedCapacityForIntData = 20;
  constexpr static const int32_t kPreAllocatedCapacityForFloatData = 20;

  OpData& op_data = *content_data_;
  op_data.ops.reserve(capacity * kPreAllocatedCapacityForOps);
  op_data.int_data.reserve(capacity * kPreAllocatedCapacityForIntData);
  op_data.float_data.reserve(capacity * kPreAllocatedCapacityForFloatData);
}

void DisplayList::Clear() {
  if (content_data_.has_value()) {
    content_data_->ops.clear();
    content_data_->int_data.clear();
    content_data_->float_data.clear();
    content_data_.reset();
  }
  if (subtree_property_data_.has_value()) {
    subtree_property_data_->ops.clear();
    subtree_property_data_->int_data.clear();
    subtree_property_data_->float_data.clear();
    subtree_property_data_.reset();
  }
}

void DisplayList::ClearSubtreeProperties() {
  if (subtree_property_data_.has_value()) {
    subtree_property_data_->ops.clear();
    subtree_property_data_->int_data.clear();
    subtree_property_data_->float_data.clear();
    subtree_property_data_.reset();
  }
}

void DisplayList::AddLinearGradient(float angle,
                                    const base::Vector<uint32_t>& colors,
                                    const base::Vector<float>& stops,
                                    int32_t tiling_index, int32_t clip_index) {
  OpData& op_data = *content_data_;
  op_data.ops.push_back(
      static_cast<int32_t>(DisplayListOpType::kLinearGradient));

  int32_t stop_count = static_cast<int32_t>(colors.size());

  // int_data layout: [int_count, float_count, stop_count, colors...,
  // tiling_index, clip_index]
  // float_data layout: [angle, positions...]

  int32_t int_count = 1 + stop_count + 2;
  int32_t float_count = 1 + stop_count;

  // Pre-calculate and reserve space to avoid multiple reallocations
  op_data.int_data.reserve(op_data.int_data.size() + 2 + int_count);
  op_data.int_data.push_back(int_count);
  op_data.int_data.push_back(float_count);

  op_data.int_data.push_back(stop_count);
  if (stop_count > 0) {
    op_data.int_data.append(colors.data(), colors.size() * sizeof(uint32_t));
  }
  op_data.int_data.push_back(tiling_index);
  op_data.int_data.push_back(clip_index);

  // Pre-calculate and reserve space for float_data
  op_data.float_data.reserve(op_data.float_data.size() + float_count);
  op_data.float_data.push_back(angle);
  if (stop_count > 0) {
    op_data.float_data.append(stops.data(), stops.size() * sizeof(float));
  }
}

}  // namespace tasm
}  // namespace lynx
