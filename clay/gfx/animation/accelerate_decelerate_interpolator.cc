// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/accelerate_decelerate_interpolator.h"

#include <math.h>

#include <algorithm>

namespace clay {

float AccelerateDecelerateInterpolator::Interpolate(float input) {
  return static_cast<float>(cosf((input + 1) * M_PI) / 2.0f) + 0.5f;
}

std::unique_ptr<AccelerateDecelerateInterpolator>
AccelerateDecelerateInterpolator::Create() {
  return std::make_unique<AccelerateDecelerateInterpolator>();
}

std::unique_ptr<Interpolator> AccelerateDecelerateInterpolator::Clone() {
  return AccelerateDecelerateInterpolator::Create();
}

}  // namespace clay
