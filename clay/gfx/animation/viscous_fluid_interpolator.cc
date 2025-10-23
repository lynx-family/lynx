// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/viscous_fluid_interpolator.h"

#include <cmath>
#include <memory>
#include <utility>

namespace clay {
namespace {

constexpr float kViscousFluidScale = 8.0f;

float ViscousFluid(float x) {
  x *= kViscousFluidScale;
  if (x < 1.0f) {
    x -= (1.0f - std::exp(-x));
  } else {
    float start = 0.36787944117f;  // 1/e == exp(-1)
    x = 1.0f - std::exp(1.0f - x);
    x = start + x * (1.0f - start);
  }
  return x;
}

static const float kViscousFluidNormalize = 1.0f / ViscousFluid(1.0f);
static const float kViscousFluidOffset =
    1.0f - kViscousFluidNormalize * ViscousFluid(1.0f);

}  // namespace

std::unique_ptr<Interpolator> ViscousFluidInterpolator::Clone() {
  return std::make_unique<ViscousFluidInterpolator>();
}

float ViscousFluidInterpolator::Interpolate(float input) {
  const float interpolated = kViscousFluidNormalize * ViscousFluid(input);
  if (interpolated > 0) {
    return interpolated + kViscousFluidOffset;
  }
  return interpolated;
}

}  // namespace clay
