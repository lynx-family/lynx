// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_TIMELINE_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_TIMELINE_H_

#include <memory>

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "core/animation/basic_animation/animation_effect_timing.h"

namespace lynx {
namespace animation {
namespace basic {
class AnimationTimeLine {
 public:
  virtual ~AnimationTimeLine() = default;
  virtual const fml::TimePoint& current_time() = 0;

 private:
  fml::TimePoint clock_time{fml::TimePoint()};
};
}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_TIMELINE_H_
