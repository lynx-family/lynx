// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_TRANSITION_DATA_H_
#define CORE_STYLE_TRANSITION_DATA_H_

#include <tuple>

#include "core/renderer/starlight/style/css_type.h"
#include "core/style/timing_function_data.h"

namespace lynx {
namespace starlight {
struct TransitionData {
  TransitionData();
  ~TransitionData() = default;

  long duration;
  long delay;
  AnimationPropertyType property;
  TimingFunctionData timing_func;
  bool operator==(const TransitionData& rhs) const {
    return std::tie(duration, delay, property, timing_func) ==
           std::tie(rhs.duration, rhs.delay, rhs.property, rhs.timing_func);
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_TRANSITION_DATA_H_
