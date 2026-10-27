// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_TIMING_FUNCTION_DATA_H_
#define CLAY_GFX_ANIMATION_TIMING_FUNCTION_DATA_H_

#include <tuple>

#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

struct TimingFunctionData {
  constexpr static int kIndexType = 0;
  constexpr static int kIndexX1 = 1;
  constexpr static int kIndexY1 = 2;
  constexpr static int kIndexX2 = 3;
  constexpr static int kIndexY2 = 4;
  constexpr static int kIndexStepsType = 2;

  ClayTimingFunctionType timing_func;
  float x1;
  float y1;
  float x2;
  float y2;
  ClayStepsType steps_type;

  TimingFunctionData();
  ~TimingFunctionData() = default;
  void Reset();
  bool operator==(const TimingFunctionData& rhs) const {
    return std::tie(timing_func, x1, y1, x2, y2, steps_type) ==
           std::tie(rhs.timing_func, rhs.x1, rhs.y1, rhs.x2, rhs.y2,
                    rhs.steps_type);
  }
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_TIMING_FUNCTION_DATA_H_
