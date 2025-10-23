// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/interpolator.h"

#include "clay/gfx/animation/accelerate_decelerate_interpolator.h"
#include "clay/gfx/animation/accelerate_interpolator.h"
#include "clay/gfx/animation/cubic_bezier_interpolator.h"
#include "clay/gfx/animation/decelerate_interpolator.h"
#include "clay/gfx/animation/steps_interpolator.h"
#include "clay/gfx/animation/timing_function_data.h"

namespace clay {

std::unique_ptr<Interpolator> Interpolator::CreateDefaultInterpolator() {
  return LinearInterpolator::Create();
}

std::unique_ptr<Interpolator> Interpolator::Create(
    const TimingFunctionData& data) {
  switch (data.timing_func) {
    case ClayTimingFunctionType::kLinear:
      return LinearInterpolator::Create();
    case ClayTimingFunctionType::kEaseIn:
      return AccelerateInterpolator::Create(1.0f);
    case ClayTimingFunctionType::kEaseOut:
      return DecelerateInterpolator::Create(1.0f);
    case ClayTimingFunctionType::kEaseInEaseOut:
      return AccelerateDecelerateInterpolator::Create();
    case ClayTimingFunctionType::kSquareBezier:
      return CubicBezierInterpolator::Create(data.x1, data.y1);
    case ClayTimingFunctionType::kCubicBezier:
      return CubicBezierInterpolator::Create(data.x1, data.y1, data.x2,
                                             data.y2);
    case ClayTimingFunctionType::kSteps:
      return StepsInterpolator::Create(data.x1, data.steps_type);
    default:
      return nullptr;
  }
}

std::unique_ptr<LinearInterpolator> LinearInterpolator::Create() {
  return std::make_unique<LinearInterpolator>();
}

std::unique_ptr<Interpolator> LinearInterpolator::Clone() {
  return LinearInterpolator::Create();
}

}  // namespace clay
