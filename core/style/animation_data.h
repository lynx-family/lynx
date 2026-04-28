// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_ANIMATION_DATA_H_
#define CORE_STYLE_ANIMATION_DATA_H_

#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/style/timing_function_data.h"

namespace lynx {
namespace starlight {
struct AnimationData {
  AnimationData();
  ~AnimationData() = default;
  base::String name;
  long duration;
  long delay;
  TimingFunctionData timing_func;
  int iteration_count;
  AnimationFillModeType fill_mode;
  AnimationDirectionType direction;
  AnimationPlayStateType play_state;

  bool operator==(const AnimationData& rhs) const {
    return name == rhs.name && timing_func == rhs.timing_func &&
           iteration_count == rhs.iteration_count &&
           fill_mode == rhs.fill_mode && duration == rhs.duration &&
           delay == rhs.delay && direction == rhs.direction &&
           play_state == rhs.play_state;
  }

  bool operator!=(const AnimationData& rhs) const { return !operator==(rhs); }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_ANIMATION_DATA_H_
