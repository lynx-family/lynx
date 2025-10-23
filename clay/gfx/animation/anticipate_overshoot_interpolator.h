// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ANTICIPATE_OVERSHOOT_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_ANTICIPATE_OVERSHOOT_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

class AnticipateOvershootInterpolator : public Interpolator {
 public:
  static std::unique_ptr<AnticipateOvershootInterpolator> Create(float tension);
  std::unique_ptr<Interpolator> Clone() override;

  explicit AnticipateOvershootInterpolator(float tension) : tension_(tension) {}
  float Interpolate(float input) override;

 private:
  const float tension_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ANTICIPATE_OVERSHOOT_INTERPOLATOR_H_
