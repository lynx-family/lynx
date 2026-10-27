// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_DECELERATE_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_DECELERATE_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

class DecelerateInterpolator : public Interpolator {
 public:
  static std::unique_ptr<DecelerateInterpolator> Create(float factor);
  std::unique_ptr<Interpolator> Clone() override;

  explicit DecelerateInterpolator(float factor) : factor_(factor) {}
  float Interpolate(float input) override;

 private:
  const float factor_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_DECELERATE_INTERPOLATOR_H_
