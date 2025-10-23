// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/timing_function_data.h"

namespace clay {

TimingFunctionData::TimingFunctionData()
    : timing_func(ClayTimingFunctionType::kLinear),
      x1(0.0f),
      y1(0.0f),
      x2(0.0f),
      y2(0.0f),
      steps_type(ClayStepsType::kInvalid) {}

void TimingFunctionData::Reset() {
  timing_func = ClayTimingFunctionType::kLinear;
  x1 = 0.0f;
  y1 = 0.0f;
  x2 = 0.0f;
  y2 = 0.0f;
  steps_type = ClayStepsType::kInvalid;
}

}  // namespace clay
