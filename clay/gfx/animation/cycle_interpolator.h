// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_CYCLE_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_CYCLE_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

class CycleInterpolator : public Interpolator {
 public:
  static std::unique_ptr<CycleInterpolator> Create(float cycles);
  std::unique_ptr<Interpolator> Clone() override;

  explicit CycleInterpolator(float cycles) : cycles_(cycles) {}
  float Interpolate(float input) override;

 private:
  const float cycles_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_CYCLE_INTERPOLATOR_H_
