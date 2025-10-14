// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/cycle_interpolator.h"

#include <math.h>

#include <algorithm>

namespace clay {

float CycleInterpolator::Interpolate(float input) {
  return sinf(2 * cycles_ * M_PI * input);
}

std::unique_ptr<CycleInterpolator> CycleInterpolator::Create(float cycles) {
  return std::make_unique<CycleInterpolator>(cycles);
}

std::unique_ptr<Interpolator> CycleInterpolator::Clone() {
  return CycleInterpolator::Create(cycles_);
}

}  // namespace clay
