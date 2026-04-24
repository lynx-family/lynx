// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/cubic_bezier.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace lynx {
namespace gfx {

namespace {
constexpr double kBezierEpsilon = 1e-7;
constexpr int kMaxNewtonIterations = 4;

inline double Clamp01(double v) { return std::max(0.0, std::min(1.0, v)); }
}  // namespace

CubicBezier::CubicBezier(double p1x, double p1y, double p2x, double p2y)
    : p1x_(p1x), p1y_(p1y), p2x_(p2x), p2y_(p2y) {
  InitCoefficients(p1x, p1y, p2x, p2y);
  InitGradients(p1x, p1y, p2x, p2y);
  InitRange(p1y, p2y);
  InitSpline();
}

CubicBezier::CubicBezier(const CubicBezier& other) = default;

CubicBezier::~CubicBezier() = default;

void CubicBezier::InitCoefficients(double p1x, double p1y, double p2x,
                                   double p2y) {
  cx_ = 3.0 * p1x;
  bx_ = 3.0 * (p2x - p1x) - cx_;
  ax_ = 1.0 - cx_ - bx_;

  cy_ = 3.0 * p1y;
  by_ = 3.0 * (p2y - p1y) - cy_;
  ay_ = 1.0 - cy_ - by_;
}

double CubicBezier::SampleCurveX(double t) const {
  return ((ax_ * t + bx_) * t + cx_) * t;
}

double CubicBezier::SampleCurveY(double t) const {
  return ((ay_ * t + by_) * t + cy_) * t;
}

double CubicBezier::SampleCurveDerivativeX(double t) const {
  return (3.0 * ax_ * t + 2.0 * bx_) * t + cx_;
}

double CubicBezier::SampleCurveDerivativeY(double t) const {
  return (3.0 * ay_ * t + 2.0 * by_) * t + cy_;
}

double CubicBezier::GetDefaultEpsilon() { return kBezierEpsilon; }

void CubicBezier::InitGradients(double p1x, double p1y, double p2x,
                                double p2y) {
  if (p1x > 0) {
    start_gradient_ = p1y / p1x;
  } else if (p1y == 0 && p2x > 0) {
    start_gradient_ = p2y / p2x;
  } else {
    start_gradient_ = 0;
  }

  if (p2x < 1) {
    end_gradient_ = (p2y - 1) / (p2x - 1);
  } else if (p2y == 1 && p1x < 1) {
    end_gradient_ = (p1y - 1) / (p1x - 1);
  } else {
    end_gradient_ = 0;
  }
}

void CubicBezier::InitRange(double p1y, double p2y) {
  range_min_ = 0.0;
  range_max_ = 1.0;

  const double a = 3.0 * ay_;
  const double b = 2.0 * by_;
  const double c = cy_;
  if (std::fabs(a) < std::numeric_limits<double>::epsilon()) {
    if (std::fabs(b) < std::numeric_limits<double>::epsilon()) {
      return;
    }
    const double t = -c / b;
    if (t > 0 && t < 1) {
      const double y = SampleCurveY(t);
      range_min_ = std::min(range_min_, y);
      range_max_ = std::max(range_max_, y);
    }
    return;
  }

  const double discriminant = b * b - 4.0 * a * c;
  if (discriminant < 0) {
    return;
  }

  const double sqrt_discriminant = std::sqrt(discriminant);
  const double t1 = (-b + sqrt_discriminant) / (2.0 * a);
  const double t2 = (-b - sqrt_discriminant) / (2.0 * a);
  if (t1 > 0 && t1 < 1) {
    const double y = SampleCurveY(t1);
    range_min_ = std::min(range_min_, y);
    range_max_ = std::max(range_max_, y);
  }
  if (t2 > 0 && t2 < 1) {
    const double y = SampleCurveY(t2);
    range_min_ = std::min(range_min_, y);
    range_max_ = std::max(range_max_, y);
  }

  range_min_ = std::min(range_min_, std::min(p1y, p2y));
  range_max_ = std::max(range_max_, std::max(p1y, p2y));
}

void CubicBezier::InitSpline() {
  for (int i = 0; i < CUBIC_BEZIER_SPLINE_SAMPLES; ++i) {
    const double t = static_cast<double>(i) /
                     static_cast<double>(CUBIC_BEZIER_SPLINE_SAMPLES - 1);
    spline_samples_[i] = SampleCurveX(t);
  }
}

double CubicBezier::SolveCurveXInternal(double x, double epsilon) const {
  double t0 = 0.0;
  double t1 = 1.0;
  double t2 = x;

  int sample = 1;
  for (; sample < CUBIC_BEZIER_SPLINE_SAMPLES - 1; ++sample) {
    if (x <= spline_samples_[sample]) {
      break;
    }
  }

  const double sample_step = 1.0 / (CUBIC_BEZIER_SPLINE_SAMPLES - 1);
  t0 = (sample - 1) * sample_step;
  t1 = sample * sample_step;

  const double x0 = spline_samples_[sample - 1];
  const double x1 = spline_samples_[sample];
  if (std::fabs(x1 - x0) > std::numeric_limits<double>::epsilon()) {
    t2 = t0 + (x - x0) * (t1 - t0) / (x1 - x0);
  } else {
    t2 = t0;
  }

  for (int i = 0; i < kMaxNewtonIterations; ++i) {
    const double x2 = SampleCurveX(t2) - x;
    if (std::fabs(x2) < epsilon) {
      return t2;
    }
    const double d2 = SampleCurveDerivativeX(t2);
    if (std::fabs(d2) < std::numeric_limits<double>::epsilon()) {
      break;
    }
    t2 = t2 - x2 / d2;
  }

  while (t0 < t1) {
    const double x2 = SampleCurveX(t2);
    if (std::fabs(x2 - x) < epsilon) {
      return t2;
    }
    if (x > x2) {
      t0 = t2;
    } else {
      t1 = t2;
    }
    t2 = (t1 - t0) * 0.5 + t0;
  }

  return t2;
}

double CubicBezier::SolveWithEpsilonInternal(double x, double epsilon) const {
  if (x < 0.0) {
    return 0.0 + start_gradient_ * x;
  }
  if (x > 1.0) {
    return 1.0 + end_gradient_ * (x - 1.0);
  }
  const double t = SolveCurveXInternal(x, epsilon);
  return SampleCurveY(t);
}

double CubicBezier::SlopeWithEpsilonInternal(double x, double epsilon) const {
  x = Clamp01(x);
  const double t = SolveCurveXInternal(x, epsilon);
  const double dx = SampleCurveDerivativeX(t);
  if (std::fabs(dx) < std::numeric_limits<double>::epsilon()) {
    return 0.0;
  }
  return SampleCurveDerivativeY(t) / dx;
}

double CubicBezier::SolveCurveX(double x, double epsilon) const {
  return SolveCurveXInternal(x, epsilon);
}

double CubicBezier::Solve(double x) const {
  return SolveWithEpsilonInternal(x, kBezierEpsilon);
}

double CubicBezier::SolveWithEpsilon(double x, double epsilon) const {
  return SolveWithEpsilonInternal(x, epsilon);
}

double CubicBezier::Slope(double x) const {
  return SlopeWithEpsilonInternal(x, kBezierEpsilon);
}

double CubicBezier::SlopeWithEpsilon(double x, double epsilon) const {
  return SlopeWithEpsilonInternal(x, epsilon);
}

}  // namespace gfx
}  // namespace lynx
