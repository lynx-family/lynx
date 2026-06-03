// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_TIMING_FUNCTION_H_
#define GFX_ANIMATION_TIMING_FUNCTION_H_

#include <cstdint>
#include <memory>

#include "gfx/animation/animation_types.h"
#include "gfx/animation/cubic_bezier.h"

namespace lynx {
namespace gfx {

class TimingFunction {
 public:
  enum class Type { LINEAR, CUBIC_BEZIER, STEPS };
  enum class LimitDirection { LEFT, RIGHT };

  virtual ~TimingFunction() = default;

  TimingFunction& operator=(const TimingFunction&) = delete;

  virtual Type GetType() const = 0;
  virtual double GetValue(double t) const = 0;
  virtual double Velocity(double time) const = 0;
  virtual std::unique_ptr<TimingFunction> Clone() const = 0;

 protected:
  TimingFunction() = default;
};

class CubicBezierTimingFunction : public TimingFunction {
 public:
  enum class EaseType { EASE, EASE_IN, EASE_OUT, EASE_IN_OUT, CUSTOM };

  static std::unique_ptr<CubicBezierTimingFunction> CreatePreset(
      EaseType ease_type);
  static std::unique_ptr<CubicBezierTimingFunction> Create(double x1, double y1,
                                                           double x2,
                                                           double y2);
  static std::unique_ptr<CubicBezierTimingFunction> CreateSquareBezier(
      double x, double y);

  ~CubicBezierTimingFunction() override;

  CubicBezierTimingFunction& operator=(const CubicBezierTimingFunction&) =
      delete;

  Type GetType() const override;
  double GetValue(double time) const override;
  double Velocity(double time) const override;
  std::unique_ptr<TimingFunction> Clone() const override;

  EaseType ease_type() const { return ease_type_; }
  const CubicBezier& bezier() const { return bezier_; }

 private:
  CubicBezierTimingFunction(EaseType ease_type, double x1, double y1, double x2,
                            double y2);

  CubicBezier bezier_;
  EaseType ease_type_;
};

class StepsTimingFunction : public TimingFunction {
 public:
  static std::unique_ptr<StepsTimingFunction> Create(int steps,
                                                     StepsType step_position);
  ~StepsTimingFunction() override;

  StepsTimingFunction& operator=(const StepsTimingFunction&) = delete;

  Type GetType() const override;
  double GetValue(double t) const override;
  std::unique_ptr<TimingFunction> Clone() const override;
  double Velocity(double time) const override;

  int steps() const { return steps_; }
  StepsType step_position() const { return step_position_; }
  double GetPreciseValue(double t, LimitDirection limit_direction) const;

 private:
  StepsTimingFunction(int steps, StepsType step_position);

  int NumberOfJumps() const;
  float GetStepsStartOffset() const;

  int steps_;
  StepsType step_position_;
};

class LinearTimingFunction : public TimingFunction {
 public:
  static std::unique_ptr<LinearTimingFunction> Create();
  LinearTimingFunction();
  ~LinearTimingFunction() override;

  Type GetType() const override;
  double GetValue(double t) const override;
  std::unique_ptr<TimingFunction> Clone() const override;
  double Velocity(double time) const override;
};

std::unique_ptr<TimingFunction> CreateTimingFunction(
    const TimingFunctionData& timing_function_data);

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_TIMING_FUNCTION_H_
