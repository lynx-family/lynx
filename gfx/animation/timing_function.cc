// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/timing_function.h"

#include <cmath>

namespace lynx {
namespace gfx {

std::unique_ptr<CubicBezierTimingFunction>
CubicBezierTimingFunction::CreatePreset(EaseType ease_type) {
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
          new CubicBezierTimingFunction(ease_type, 0.42, 0.0, 0.58, 1.0));
    default:
      return nullptr;
  }
}

std::unique_ptr<CubicBezierTimingFunction> CubicBezierTimingFunction::Create(
    double x1, double y1, double x2, double y2) {
  return std::unique_ptr<CubicBezierTimingFunction>(
      new CubicBezierTimingFunction(EaseType::CUSTOM, x1, y1, x2, y2));
}

std::unique_ptr<CubicBezierTimingFunction>
CubicBezierTimingFunction::CreateSquareBezier(double x, double y) {
  constexpr double kScaleFactor = 2.0 / 3.0;
  return CubicBezierTimingFunction::Create(x * kScaleFactor, y * kScaleFactor,
                                           1.0 + (x - 1.0) * kScaleFactor,
                                           1.0 + (y - 1.0) * kScaleFactor);
}

CubicBezierTimingFunction::CubicBezierTimingFunction(EaseType ease_type,
                                                     double x1, double y1,
                                                     double x2, double y2)
    : bezier_(x1, y1, x2, y2), ease_type_(ease_type) {}

CubicBezierTimingFunction::~CubicBezierTimingFunction() = default;

TimingFunction::Type CubicBezierTimingFunction::GetType() const {
  return Type::CUBIC_BEZIER;
}

double CubicBezierTimingFunction::GetValue(double time) const {
  return bezier_.Solve(time);
}

double CubicBezierTimingFunction::Velocity(double time) const {
  return bezier_.Slope(time);
}

std::unique_ptr<TimingFunction> CubicBezierTimingFunction::Clone() const {
  return std::unique_ptr<TimingFunction>(new CubicBezierTimingFunction(*this));
}

std::unique_ptr<StepsTimingFunction> StepsTimingFunction::Create(
    int steps, StepsType step_position) {
  return std::unique_ptr<StepsTimingFunction>(
      new StepsTimingFunction(steps, step_position));
}

StepsTimingFunction::StepsTimingFunction(int steps, StepsType step_position)
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

double StepsTimingFunction::Velocity(double /*time*/) const { return 0; }

double StepsTimingFunction::GetPreciseValue(double t,
                                            LimitDirection direction) const {
  const double steps = static_cast<double>(steps_);
  double current_step = std::floor((steps * t) + GetStepsStartOffset());
  if (direction == LimitDirection::LEFT &&
      steps * t - std::floor(steps * t) == 0) {
    current_step -= 1;
  }
  const int jumps = NumberOfJumps();
  if (t >= 0 && current_step < 0) {
    current_step = 0;
  }
  if (t <= 1 && current_step > jumps) {
    current_step = jumps;
  }
  return jumps == 0 ? 0.0 : (current_step / jumps);
}

int StepsTimingFunction::NumberOfJumps() const {
  switch (step_position_) {
    case StepsType::kEnd:
    case StepsType::kStart:
      return steps_;
    case StepsType::kJumpBoth:
      return steps_ + 1;
    case StepsType::kJumpNone:
      return steps_ - 1;
    default:
      return steps_;
  }
}

float StepsTimingFunction::GetStepsStartOffset() const {
  switch (step_position_) {
    case StepsType::kJumpBoth:
    case StepsType::kStart:
      return 1;
    case StepsType::kEnd:
    case StepsType::kJumpNone:
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

double LinearTimingFunction::GetValue(double t) const { return t; }

std::unique_ptr<TimingFunction> LinearTimingFunction::Clone() const {
  return std::make_unique<LinearTimingFunction>(*this);
}

double LinearTimingFunction::Velocity(double /*time*/) const { return 0; }

std::unique_ptr<TimingFunction> CreateTimingFunction(
    const TimingFunctionData& timing_function_data) {
  using TFType = TimingFunctionType;
  switch (timing_function_data.timing_func) {
    case TFType::kLinear:
      return LinearTimingFunction::Create();
    case TFType::kEaseIn:
      return CubicBezierTimingFunction::CreatePreset(
          CubicBezierTimingFunction::EaseType::EASE_IN);
    case TFType::kEaseOut:
      return CubicBezierTimingFunction::CreatePreset(
          CubicBezierTimingFunction::EaseType::EASE_OUT);
    case TFType::kEaseInEaseOut:
      return CubicBezierTimingFunction::CreatePreset(
          CubicBezierTimingFunction::EaseType::EASE_IN_OUT);
    case TFType::kCubicBezier:
      return CubicBezierTimingFunction::Create(
          timing_function_data.x1, timing_function_data.y1,
          timing_function_data.x2, timing_function_data.y2);
    case TFType::kSquareBezier:
      return CubicBezierTimingFunction::CreateSquareBezier(
          timing_function_data.x1, timing_function_data.y1);
    case TFType::kSteps:
      return StepsTimingFunction::Create(
          static_cast<int>(timing_function_data.x1),
          timing_function_data.steps_type);
    default:
      return LinearTimingFunction::Create();
  }
}

}  // namespace gfx
}  // namespace lynx
