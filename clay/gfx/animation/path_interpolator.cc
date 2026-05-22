// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/path_interpolator.h"

namespace clay {

std::unique_ptr<PathInterpolator> PathInterpolator::Create(
    std::vector<float>&& x, std::vector<float>&& y) {
  return std::make_unique<PathInterpolator>(std::move(x), std::move(y));
}

std::unique_ptr<Interpolator> PathInterpolator::Clone() {
  std::vector<float> x = x_;
  std::vector<float> y = y_;
  return PathInterpolator::Create(std::move(x), std::move(y));
}

float PathInterpolator::Interpolate(float t) {
  if (t <= 0) {
    return 0;
  } else if (t >= 1) {
    return 1;
  }
  // Do a binary search for the correct x to interpolate between.
  size_t start_index = 0;
  size_t end_index = x_.size() - 1;

  while (end_index > start_index + 1) {
    int mid_index = (start_index + end_index) / 2;
    if (t < x_[mid_index]) {
      end_index = mid_index;
    } else {
      start_index = mid_index;
    }
  }

  float x_range = x_[end_index] - x_[start_index];
  if (x_range == 0) {
    return y_[start_index];
  }

  float t_in_range = t - x_[start_index];
  float fraction = t_in_range / x_range;

  float start_y = y_[start_index];
  float end_y = y_[end_index];
  return start_y + (fraction * (end_y - start_y));
}

}  // namespace clay
