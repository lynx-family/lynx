// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/decelerate_interpolator.h"

#include <math.h>

namespace clay {

float DecelerateInterpolator::Interpolate(float input) {
  float result;
  if (factor_ == 1.0f) {
    result = 1.0f - (1.0f - input) * (1.0f - input);
  } else {
    result = 1.0f - pow((1.0f - input), 2 * factor_);
  }
  return result;
}

std::unique_ptr<DecelerateInterpolator> DecelerateInterpolator::Create(
    float factor) {
  return std::make_unique<DecelerateInterpolator>(factor);
}

std::unique_ptr<Interpolator> DecelerateInterpolator::Clone() {
  return DecelerateInterpolator::Create(factor_);
}

}  // namespace clay
