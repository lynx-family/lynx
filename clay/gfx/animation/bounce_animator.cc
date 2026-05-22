// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/bounce_animator.h"

#include "base/include/fml/time/time_point.h"
#include "clay/fml/logging.h"

namespace clay {

void BounceAnimator::Init() {
  start_time_ = fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF();
}

bool BounceAnimator::UpdateValueAndVelocity(int64_t delta_time) {
  // // FIXME: sometimes `delta_time` is negative, need to fix it.
  // if (delta_time <= 0) {
  //   return false;
  // }

  FML_DCHECK(last_frame_time_ - start_time_ >= 0);
  float t = (last_frame_time_ - start_time_) / 1000.f;
  float c1 = start_value_ - final_value_;
  float c2 = velocity_ + 10 * c1;
  value_ = final_value_ + exp(-10 * t) * (c1 + c2 * t);

  if (IsAtEquilibrium(value_, velocity_)) {
    value_ = final_value_;
    return true;
  }
  return false;
}

bool BounceAnimator::IsAtEquilibrium(float value, float velocity) {
  // A bounce animation may have the same start and end value. In this case,
  // checking the distance to the final value is not enough, which may cause the
  // animation to be finished at the beginning. So we also check the elapsed
  // time.
  return std::abs(value - final_value_) <= 1 &&
         (last_frame_time_ - start_time_) >= 100;
}

}  // namespace clay
