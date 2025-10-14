// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_PATH_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_PATH_INTERPOLATOR_H_

#include <memory>
#include <utility>
#include <vector>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

class PathInterpolator : public Interpolator {
 public:
  static std::unique_ptr<PathInterpolator> Create(std::vector<float>&& x,
                                                  std::vector<float>&& y);
  std::unique_ptr<Interpolator> Clone() override;

  PathInterpolator(std::vector<float>&& x, std::vector<float>&& y)
      : x_(std::move(x)), y_(std::move(y)) {}
  float Interpolate(float input) override;

 private:
  std::vector<float> x_;
  std::vector<float> y_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_PATH_INTERPOLATOR_H_
