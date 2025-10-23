// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_STEPS_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_STEPS_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

class StepsInterpolator : public Interpolator {
 public:
  static std::unique_ptr<StepsInterpolator> Create(int steps,
                                                   ClayStepsType type);
  std::unique_ptr<Interpolator> Clone() override;

  StepsInterpolator(int steps, ClayStepsType type)
      : steps_(steps), type_(type) {}
  float Interpolate(float input) override;

 private:
  const int steps_;
  const ClayStepsType type_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_STEPS_INTERPOLATOR_H_
