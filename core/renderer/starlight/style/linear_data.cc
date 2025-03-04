// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/linear_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"

namespace lynx {
namespace starlight {

LinearData::LinearData()
    : linear_weight_sum_(DefaultLayoutStyle::SL_DEFAULT_LINEAR_WEIGHT_SUM),
      linear_weight_(DefaultLayoutStyle::SL_DEFAULT_LINEAR_WEIGHT),
      linear_orientation_(DefaultLayoutStyle::SL_DEFAULT_LINEAR_ORIENTATION),
      linear_layout_gravity_(
          DefaultLayoutStyle::SL_DEFAULT_LINEAR_LAYOUT_GRAVITY),
      linear_gravity_(DefaultLayoutStyle::SL_DEFAULT_LINEAR_GRAVITY),
      linear_cross_gravity_(
          DefaultLayoutStyle::SL_DEFAULT_LINEAR_CROSS_GRAVITY) {}

LinearData::LinearData(const LinearData& data)
    : linear_weight_sum_(data.linear_weight_sum_),
      linear_weight_(data.linear_weight_),
      linear_orientation_(data.linear_orientation_),
      linear_layout_gravity_(data.linear_layout_gravity_),
      linear_gravity_(data.linear_gravity_),
      linear_cross_gravity_(data.linear_cross_gravity_) {}

void LinearData::Reset() {
  linear_weight_sum_ = DefaultLayoutStyle::SL_DEFAULT_LINEAR_WEIGHT_SUM;
  linear_weight_ = DefaultLayoutStyle::SL_DEFAULT_LINEAR_WEIGHT;
  linear_orientation_ = DefaultLayoutStyle::SL_DEFAULT_LINEAR_ORIENTATION;
  linear_layout_gravity_ = DefaultLayoutStyle::SL_DEFAULT_LINEAR_LAYOUT_GRAVITY;
  linear_gravity_ = DefaultLayoutStyle::SL_DEFAULT_LINEAR_GRAVITY;
  linear_cross_gravity_ = DefaultLayoutStyle::SL_DEFAULT_LINEAR_CROSS_GRAVITY;
}

}  // namespace starlight
}  // namespace lynx
