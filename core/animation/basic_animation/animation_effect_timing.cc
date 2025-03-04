// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/basic_animation/animation_effect_timing.h"

namespace lynx {
namespace animation {

namespace basic {

void AnimationEffectTiming::UpdateTiming(
    std::unique_ptr<OptionalAnimationEffectTiming> timing) {
  if (timing->delay_.has_value()) {
    delay_ = *timing->delay_;
  }
  if (timing->fill_.has_value()) {
    fill_ = *timing->fill_;
  }
  if (timing->iterations_.has_value()) {
    iterations_ = *timing->iterations_;
  }
  if (timing->duration_.has_value()) {
    duration_ = *timing->duration_;
  }
  if (timing->direction_.has_value()) {
    direction_ = *timing->direction_;
  }
  if (timing->easing_) {
    easing_ = std::move(timing->easing_);
  }
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
