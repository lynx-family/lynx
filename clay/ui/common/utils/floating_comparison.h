// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_UTILS_FLOATING_COMPARISON_H_
#define CLAY_UI_COMMON_UTILS_FLOATING_COMPARISON_H_

#include <algorithm>

namespace clay {

static const float kEpsilon = 1e-6f;

inline bool RoughlyEqual(float v1, float v2) {
  return fabsf(v1 - v2) < kEpsilon;
}

inline bool RoughlyNotEqual(float v1, float v2) {
  return !RoughlyEqual(v1, v2);
}

inline bool RoughlyEqualToZero(float value) { return RoughlyEqual(value, 0.f); }

inline bool RoughlyNotZero(float value) { return !RoughlyEqualToZero(value); }

}  // namespace clay

#endif  // CLAY_UI_COMMON_UTILS_FLOATING_COMPARISON_H_
