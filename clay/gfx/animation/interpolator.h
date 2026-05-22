// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_INTERPOLATOR_H_

#include <memory>

namespace clay {

struct TimingFunctionData;

class Interpolator {
 public:
  virtual ~Interpolator() = default;

  virtual float Interpolate(float input) = 0;

  virtual std::unique_ptr<Interpolator> Clone() = 0;

  static std::unique_ptr<Interpolator> Create(const TimingFunctionData& data);

  static std::unique_ptr<Interpolator> CreateDefaultInterpolator();
  virtual double Velocity(double time) const { return 0; }

 protected:
  Interpolator() = default;
};

class LinearInterpolator : public Interpolator {
 public:
  static std::unique_ptr<LinearInterpolator> Create();
  std::unique_ptr<Interpolator> Clone() override;

  float Interpolate(float input) override { return input; }
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_INTERPOLATOR_H_
