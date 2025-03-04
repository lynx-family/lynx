// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_UTILS_TIMING_FUNCTION_H_
#define CORE_ANIMATION_UTILS_TIMING_FUNCTION_H_
#include <memory>

#include "core/animation/utils/cubic_bezier.h"
#include "core/style/animation_data.h"

// See http://www.w3.org/TR/css3-transitions/.
namespace lynx {
namespace animation {

class TimingFunction {
 public:
  virtual ~TimingFunction();

  TimingFunction& operator=(const TimingFunction&) = delete;

  // Note that LINEAR is a nullptr TimingFunction (for now).
  enum class Type { LINEAR, CUBIC_BEZIER, STEPS };

  // Which limit to apply at a discontinuous boundary.
  enum class LimitDirection { LEFT, RIGHT };

  virtual Type GetType() const = 0;
  virtual double GetValue(double t) const = 0;
  virtual double Velocity(double time) const = 0;
  virtual std::unique_ptr<TimingFunction> Clone() const = 0;

  static std::unique_ptr<TimingFunction> MakeTimingFunction(
      const starlight::AnimationData* animation_data);

  static std::unique_ptr<TimingFunction> MakeTimingFunction(
      starlight::TimingFunctionData& timing_function_data);

 protected:
  TimingFunction();
};

class CubicBezierTimingFunction : public TimingFunction {
 public:
  enum class EaseType { EASE, EASE_IN, EASE_OUT, EASE_IN_OUT, CUSTOM };

  static std::unique_ptr<CubicBezierTimingFunction> CreatePreset(
      EaseType ease_type);
  static std::unique_ptr<CubicBezierTimingFunction> Create(double x1, double y1,
                                                           double x2,
                                                           double y2);
  ~CubicBezierTimingFunction() override;

  CubicBezierTimingFunction& operator=(const CubicBezierTimingFunction&) =
      delete;

  // TimingFunction implementation.
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
  static std::unique_ptr<StepsTimingFunction> Create(
      int steps, starlight::StepsType step_position);
  ~StepsTimingFunction() override;

  StepsTimingFunction& operator=(const StepsTimingFunction&) = delete;

  // TimingFunction implementation.
  Type GetType() const override;
  double GetValue(double t) const override;
  std::unique_ptr<TimingFunction> Clone() const override;
  double Velocity(double time) const override;

  int steps() const { return steps_; }
  starlight::StepsType step_position() const { return step_position_; }
  double GetPreciseValue(double t, LimitDirection limit_direction) const;

 private:
  StepsTimingFunction(int steps, starlight::StepsType step_position);

  // The number of jumps is the number of discontinuities in the timing
  // function. There is a subtle distinction between the number of steps and
  // jumps. The number of steps is the number of intervals in the timing
  // function. The number of jumps differs from the number of steps when either
  // both or neither end point has a discontinuity.
  // https://drafts.csswg.org/css-easing-1/#step-easing-functions
  int NumberOfJumps() const;

  float GetStepsStartOffset() const;

  int steps_;
  starlight::StepsType step_position_;
};

class LinearTimingFunction : public TimingFunction {
 public:
  static std::unique_ptr<LinearTimingFunction> Create();
  LinearTimingFunction();
  ~LinearTimingFunction() override;

  // TimingFunction implementation.
  Type GetType() const override;
  double GetValue(double t) const override;
  std::unique_ptr<TimingFunction> Clone() const override;
  double Velocity(double time) const override;

 private:
};

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_UTILS_TIMING_FUNCTION_H_
