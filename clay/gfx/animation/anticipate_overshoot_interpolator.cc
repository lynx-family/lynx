// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/anticipate_overshoot_interpolator.h"

namespace clay {

namespace {
float a(float t, float s) { return t * t * ((s + 1) * t - s); }

float o(float t, float s) { return t * t * ((s + 1) * t + s); }
}  // namespace

float AnticipateOvershootInterpolator::Interpolate(float t) {
  if (t < 0.5f) {
    return 0.5f * a(t * 2.0f, tension_);
  } else {
    return 0.5f * (o(t * 2.0f - 2.0f, tension_) + 2.0f);
  }
}

std::unique_ptr<AnticipateOvershootInterpolator>
AnticipateOvershootInterpolator::Create(float tension) {
  return std::make_unique<AnticipateOvershootInterpolator>(tension);
}

std::unique_ptr<Interpolator> AnticipateOvershootInterpolator::Clone() {
  return AnticipateOvershootInterpolator::Create(tension_);
}

}  // namespace clay
