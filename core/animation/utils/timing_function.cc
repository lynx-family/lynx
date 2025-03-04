// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/utils/timing_function.h"

#include <cmath>

namespace lynx {
namespace animation {

TimingFunction::TimingFunction() = default;

TimingFunction::~TimingFunction() = default;

std::unique_ptr<TimingFunction> TimingFunction::MakeTimingFunction(
    const starlight::AnimationData* animation_data) {
  auto timing_function_data = animation_data->timing_func;
  return MakeTimingFunction(timing_function_data);
}

std::unique_ptr<TimingFunction> TimingFunction::MakeTimingFunction(
    starlight::TimingFunctionData& timing_function_data) {
  std::unique_ptr<TimingFunction> timing;
  auto type = timing_function_data.timing_func;
  if (type == starlight::TimingFunctionType::kLinear) {
    timing = std::make_unique<LinearTimingFunction>();
  } else if (type == starlight::TimingFunctionType::kEaseIn) {
    timing = CubicBezierTimingFunction::CreatePreset(
        CubicBezierTimingFunction::EaseType::EASE_IN);
  } else if (type == starlight::TimingFunctionType::kEaseOut) {
    timing = CubicBezierTimingFunction::CreatePreset(
        CubicBezierTimingFunction::EaseType::EASE_OUT);
  } else if (type == starlight::TimingFunctionType::kEaseInEaseOut) {
    timing = CubicBezierTimingFunction::CreatePreset(
        CubicBezierTimingFunction::EaseType::EASE_IN_OUT);
  } else if (type == starlight::TimingFunctionType::kCubicBezier) {
    timing = CubicBezierTimingFunction::Create(
        timing_function_data.x1, timing_function_data.y1,
        timing_function_data.x2, timing_function_data.y2);
  } else if (type == starlight::TimingFunctionType::kSteps) {
    timing = StepsTimingFunction::Create(timing_function_data.x1,
                                         timing_function_data.steps_type);
  } else {
    timing = std::make_unique<LinearTimingFunction>();
  }

  return timing;
}

std::unique_ptr<CubicBezierTimingFunction>
CubicBezierTimingFunction::CreatePreset(EaseType ease_type) {
  // These numbers come from
  // http://www.w3.org/TR/css3-transitions/#transition-timing-function_tag.
  switch (ease_type) {
    case EaseType::EASE:
      return std::unique_ptr<CubicBezierTimingFunction>(
          new CubicBezierTimingFunction(ease_type, 0.25, 0.1, 0.25, 1.0));
    case EaseType::EASE_IN:
      return std::unique_ptr<CubicBezierTimingFunction>(
          new CubicBezierTimingFunction(ease_type, 0.42, 0.0, 1.0, 1.0));
    case EaseType::EASE_OUT:
      return std::unique_ptr<CubicBezierTimingFunction>(
          new CubicBezierTimingFunction(ease_type, 0.0, 0.0, 0.58, 1.0));
    case EaseType::EASE_IN_OUT:
      return std::unique_ptr<CubicBezierTimingFunction>(
          new CubicBezierTimingFunction(ease_type, 0.42, 0.0, 0.58, 1));
    default:
      //      NOTREACHED();
      return nullptr;
  }
}
std::unique_ptr<CubicBezierTimingFunction> CubicBezierTimingFunction::Create(
    double x1, double y1, double x2, double y2) {
  return std::unique_ptr<CubicBezierTimingFunction>(
      new CubicBezierTimingFunction(EaseType::CUSTOM, x1, y1, x2, y2));
}

CubicBezierTimingFunction::CubicBezierTimingFunction(EaseType ease_type,
                                                     double x1, double y1,
                                                     double x2, double y2)
    : bezier_(x1, y1, x2, y2), ease_type_(ease_type) {}

CubicBezierTimingFunction::~CubicBezierTimingFunction() = default;

TimingFunction::Type CubicBezierTimingFunction::GetType() const {
  return Type::CUBIC_BEZIER;
}

double CubicBezierTimingFunction::GetValue(double x) const {
  return bezier_.Solve(x);
}

double CubicBezierTimingFunction::Velocity(double x) const {
  return bezier_.Slope(x);
}

std::unique_ptr<TimingFunction> CubicBezierTimingFunction::Clone() const {
  return std::unique_ptr<TimingFunction>(new CubicBezierTimingFunction(*this));
}

std::unique_ptr<StepsTimingFunction> StepsTimingFunction::Create(
    int steps, starlight::StepsType step_position) {
  return std::unique_ptr<StepsTimingFunction>(
      new StepsTimingFunction(steps, step_position));
}

StepsTimingFunction::StepsTimingFunction(int steps,
                                         starlight::StepsType step_position)
    : steps_(steps), step_position_(step_position) {}

StepsTimingFunction::~StepsTimingFunction() = default;

TimingFunction::Type StepsTimingFunction::GetType() const {
  return Type::STEPS;
}

double StepsTimingFunction::GetValue(double t) const {
  return GetPreciseValue(t, TimingFunction::LimitDirection::RIGHT);
}

std::unique_ptr<TimingFunction> StepsTimingFunction::Clone() const {
  return std::unique_ptr<TimingFunction>(new StepsTimingFunction(*this));
}

double StepsTimingFunction::Velocity(double x) const { return 0; }

double StepsTimingFunction::GetPreciseValue(double t,
                                            LimitDirection direction) const {
  const double steps = static_cast<double>(steps_);
  double current_step = std::floor((steps * t) + GetStepsStartOffset());
  // Adjust step if using a left limit at a discontinuous step boundary.
  if (direction == LimitDirection::LEFT &&
      steps * t - std::floor(steps * t) == 0) {
    current_step -= 1;
  }
  // Jumps may differ from steps based on the number of end-point
  // discontinuities, which may be 0, 1 or 2.
  int jumps = NumberOfJumps();
  if (t >= 0 && current_step < 0) current_step = 0;
  if (t <= 1 && current_step > jumps) current_step = jumps;
  return current_step / jumps;
}

int StepsTimingFunction::NumberOfJumps() const {
  switch (step_position_) {
    case starlight::StepsType::kEnd:
    case starlight::StepsType::kStart:
      return steps_;

    case starlight::StepsType::kJumpBoth:
      return steps_ + 1;

    case starlight::StepsType::kJumpNone:
      // assert(steps_ > 1);
      return steps_ - 1;

    default:
      return steps_;
  }
}

float StepsTimingFunction::GetStepsStartOffset() const {
  switch (step_position_) {
    case starlight::StepsType::kJumpBoth:
    case starlight::StepsType::kStart:
      return 1;

    case starlight::StepsType::kEnd:
    case starlight::StepsType::kJumpNone:
      return 0;

    default:
      return 1;
  }
}

std::unique_ptr<LinearTimingFunction> LinearTimingFunction::Create() {
  return std::make_unique<LinearTimingFunction>();
}

LinearTimingFunction::LinearTimingFunction() = default;

LinearTimingFunction::~LinearTimingFunction() = default;

TimingFunction::Type LinearTimingFunction::GetType() const {
  return Type::LINEAR;
}

std::unique_ptr<TimingFunction> LinearTimingFunction::Clone() const {
  return std::make_unique<LinearTimingFunction>(*this);
}

double LinearTimingFunction::Velocity(double x) const { return 0; }

double LinearTimingFunction::GetValue(double t) const { return t; }

}  // namespace animation
}  // namespace lynx
