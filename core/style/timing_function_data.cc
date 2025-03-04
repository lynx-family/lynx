// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/timing_function_data.h"

namespace lynx {
namespace starlight {

TimingFunctionData::TimingFunctionData()
    : timing_func(TimingFunctionType::kLinear),
      x1(0.0f),
      y1(0.0f),
      x2(0.0f),
      y2(0.0f),
      steps_type(StepsType::kInvalid) {}

void TimingFunctionData::Reset() {
  timing_func = TimingFunctionType::kLinear;
  x1 = 0.0f;
  y1 = 0.0f;
  x2 = 0.0f;
  y2 = 0.0f;
  steps_type = StepsType::kInvalid;
}
}  // namespace starlight
}  // namespace lynx
