// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/overshoot_interpolator.h"

namespace clay {

float OvershootInterpolator::Interpolate(float t) {
  t -= 1.0f;
  return t * t * ((tension_ + 1) * t + tension_) + 1.0f;
}

std::unique_ptr<OvershootInterpolator> OvershootInterpolator::Create(
    float tension) {
  return std::make_unique<OvershootInterpolator>(tension);
}

std::unique_ptr<Interpolator> OvershootInterpolator::Clone() {
  return OvershootInterpolator::Create(tension_);
}

}  // namespace clay
