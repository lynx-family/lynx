// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/accelerate_interpolator.h"

#include <math.h>

#include <algorithm>

namespace clay {

float AccelerateInterpolator::Interpolate(float input) {
  if (factor_ == 1.0f) {
    return input * input;
  } else {
    return pow(input, doubleFactor_);
  }
}

std::unique_ptr<AccelerateInterpolator> AccelerateInterpolator::Create(
    float factor) {
  return std::make_unique<AccelerateInterpolator>(factor);
}

std::unique_ptr<Interpolator> AccelerateInterpolator::Clone() {
  return AccelerateInterpolator::Create(factor_);
}

}  // namespace clay
