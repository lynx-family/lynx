// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_ACCELERATE_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_ACCELERATE_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

class AccelerateInterpolator : public Interpolator {
 public:
  static std::unique_ptr<AccelerateInterpolator> Create(float factor);
  std::unique_ptr<Interpolator> Clone() override;

  explicit AccelerateInterpolator(float factor)
      : factor_(factor), doubleFactor_(factor * 2) {}
  float Interpolate(float input) override;

 private:
  const float factor_;
  const float doubleFactor_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_ACCELERATE_INTERPOLATOR_H_
