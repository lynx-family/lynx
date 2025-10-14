// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/anticipate_interpolator.h"

namespace clay {

float AnticipateInterpolator::Interpolate(float t) {
  return t * t * ((tension_ + 1) * t - tension_);
}

std::unique_ptr<AnticipateInterpolator> AnticipateInterpolator::Create(
    float tension) {
  return std::make_unique<AnticipateInterpolator>(tension);
}

std::unique_ptr<Interpolator> AnticipateInterpolator::Clone() {
  return AnticipateInterpolator::Create(tension_);
}

}  // namespace clay
