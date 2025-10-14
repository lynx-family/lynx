// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_LUT_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_LUT_INTERPOLATOR_H_

#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

class LUTInterpolator : public Interpolator {
 public:
  static std::unique_ptr<LUTInterpolator> Create(
      std::unique_ptr<float[]> values, size_t size);
  std::unique_ptr<Interpolator> Clone() override;

  LUTInterpolator(std::unique_ptr<float[]> values, size_t size);
  ~LUTInterpolator();

  float Interpolate(float input) override;

 private:
  std::unique_ptr<float[]> values_;
  size_t size_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_LUT_INTERPOLATOR_H_
