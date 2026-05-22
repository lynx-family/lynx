// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/bounce_interpolator.h"

namespace clay {

namespace {
float bounce(float t) { return t * t * 8.0f; }
}  // namespace

float BounceInterpolator::Interpolate(float t) {
  t *= 1.1226f;
  if (t < 0.3535f) {
    return bounce(t);
  } else if (t < 0.7408f) {
    return bounce(t - 0.54719f) + 0.7f;
  } else if (t < 0.9644f) {
    return bounce(t - 0.8526f) + 0.9f;
  } else {
    return bounce(t - 1.0435f) + 0.95f;
  }
}

std::unique_ptr<BounceInterpolator> BounceInterpolator::Create() {
  return std::make_unique<BounceInterpolator>();
}

std::unique_ptr<Interpolator> BounceInterpolator::Clone() {
  return BounceInterpolator::Create();
}

}  // namespace clay
