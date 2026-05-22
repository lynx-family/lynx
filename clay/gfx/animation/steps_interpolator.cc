// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/steps_interpolator.h"

#include "clay/fml/logging.h"

namespace clay {

std::unique_ptr<StepsInterpolator> StepsInterpolator::Create(
    int steps, ClayStepsType type) {
  return std::make_unique<StepsInterpolator>(steps, type);
}

std::unique_ptr<Interpolator> StepsInterpolator::Clone() {
  return StepsInterpolator::Create(steps_, type_);
}

float StepsInterpolator::Interpolate(float input) {
  int state;
  switch (type_) {
    case ClayStepsType::kStart:
      state = static_cast<int>(input * steps_) + 1;
      if (state > steps_) {
        state = steps_;
      }
      return (static_cast<float>(state)) / steps_;
    case ClayStepsType::kEnd:
      state = static_cast<int>(input * steps_);
      if (state == steps_) {
        state -= 1;
      }
      return (static_cast<float>(state)) / steps_;
    case ClayStepsType::kJumpBoth:
      state = static_cast<int>(input * steps_) + 1;
      if (state > steps_) {
        state = steps_;
      }
      return (static_cast<float>(state)) / (steps_ + 1);
    case ClayStepsType::kJumpNone:
      state = static_cast<int>(input * steps_);
      if (state == steps_) {
        state -= 1;
      }
      return (static_cast<float>(state)) / (steps_ - 1);
    default:
      FML_UNREACHABLE();
      return 0;
  }
}

}  // namespace clay
