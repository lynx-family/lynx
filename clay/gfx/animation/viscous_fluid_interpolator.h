// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_VISCOUS_FLUID_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_VISCOUS_FLUID_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

// Copy from Android. Used by ScrollView.
class ViscousFluidInterpolator : public Interpolator {
 public:
  float Interpolate(float input) override;
  std::unique_ptr<Interpolator> Clone() override;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_VISCOUS_FLUID_INTERPOLATOR_H_
