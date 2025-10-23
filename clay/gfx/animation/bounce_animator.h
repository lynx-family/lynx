// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_BOUNCE_ANIMATOR_H_
#define CLAY_GFX_ANIMATION_BOUNCE_ANIMATOR_H_

#include "clay/gfx/animation/dynamic_animator.h"

namespace clay {

// ref:
// https://github.com/super-ultra/ScrollMechanics/blob/master/ScrollMechanics/Sources/SpringTimingParameters.swift
class BounceAnimator : public DynamicAnimator {
 public:
  void SetTargetValue(float value) { final_value_ = value; }
  void Init();

 protected:
  bool UpdateValueAndVelocity(int64_t delta_time) override;

  float GetAcceleration(float value, float velocity) override { return 0; }

  bool IsAtEquilibrium(float value, float velocity) override;

  void SetValueThreshold(float threshold) override {}
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_BOUNCE_ANIMATOR_H_
