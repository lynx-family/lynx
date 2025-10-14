// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/cubic_bezier_interpolator.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"

namespace clay {

static const double kBezierEpsilon = 1e-7;

const int kMaxNewtonIterations = 4;

void CubicBezierInterpolator::InitSpline() {
  double delta_t = 1.0 / (CUBIC_BEZIER_SPLINE_SAMPLES - 1);
  for (int i = 0; i < CUBIC_BEZIER_SPLINE_SAMPLES; i++) {
    spline_samples_[i] = SampleCurveX(i * delta_t);
  }
}

double CubicBezierInterpolator::ToFinite(double value) const {
  if (std::isinf(value)) {
    if (value > 0) return std::numeric_limits<double>::max();
    return std::numeric_limits<double>::lowest();
  }
  return value;
}

void CubicBezierInterpolator::InitCoefficients(
    double p1x, double p1y, double p2x,
    double p2y) {  // Calculate the polynomial coefficients, implicit first and
                   // last control
  // points are (0,0) and (1,1).
  cx_ = 3.0 * p1x;
  bx_ = 3.0 * (p2x - p1x) - cx_;
  ax_ = 1.0 - cx_ - bx_;

  cy_ = ToFinite(3.0 * p1y);
  by_ = ToFinite(3.0 * (p2y - p1y) - cy_);
  ay_ = ToFinite(1.0 - cy_ - by_);
}

void CubicBezierInterpolator::InitGradients(double p1x, double p1y, double p2x,
                                            double p2y) {
  // End-point gradients are used to calculate timing function results
  // outside the range [0, 1].
  //
  // There are four possibilities for the gradient at each end:
  // (1) the closest control point is not horizontally coincident with regard to
  //     (0, 0) or (1, 1). In this case the line between the end point and
  //     the control point is tangent to the bezier at the end point.
  // (2) the closest control point is coincident with the end point. In
  //     this case the line between the end point and the far control
  //     point is tangent to the bezier at the end point.
  // (3) both internal control points are coincident with an endpoint. There
  //     are two special case that fall into this category:
  //     CubicBezier(0, 0, 0, 0) and CubicBezier(1, 1, 1, 1). Both are
  //     equivalent to linear.
  // (4) the closest control point is horizontally coincident with the end
  //     point, but vertically distinct. In this case the gradient at the
  //     end point is Infinite. However, this causes issues when
  //     interpolating. As a result, we break down to a simple case of
  //     0 gradient under these conditions.

  if (p1x > 0)
    start_gradient_ = p1y / p1x;
  else if (!p1y && p2x > 0)
    start_gradient_ = p2y / p2x;
  else if (!p1y && !p2y)
    start_gradient_ = 1;
  else
    start_gradient_ = 0;

  if (p2x < 1)
    end_gradient_ = (p2y - 1) / (p2x - 1);
  else if (p2y == 1 && p1x < 1)
    end_gradient_ = (p1y - 1) / (p1x - 1);
  else if (p2y == 1 && p1y == 1)
    end_gradient_ = 1;
  else
    end_gradient_ = 0;
}

CubicBezierInterpolator::CubicBezierInterpolator(float a, float b, float c,
                                                 float d)
    : a_(a), b_(b), c_(c), d_(d) {
  InitCoefficients(a, b, c, d);
  InitGradients(a, b, c, d);
  InitSpline();
}

std::unique_ptr<CubicBezierInterpolator> CubicBezierInterpolator::CreatePreset(
    Type type) {
  switch (type) {
    case Ease:
      return CubicBezierInterpolator::Create(0.25f, 0.1f, 0.25f, 1.0f);
    case EaseIn:
      return CubicBezierInterpolator::Create(0.42f, 0.0f, 1.0f, 1.0f);
    case EaseOut:
      return CubicBezierInterpolator::Create(0.0f, 0.0f, 0.58f, 1.0f);
    case EaseInOut:
      return CubicBezierInterpolator::Create(0.42f, 0.0f, 0.58f, 1.0f);
    default:
      FML_UNREACHABLE();
  }
  return nullptr;
}

std::unique_ptr<CubicBezierInterpolator> CubicBezierInterpolator::Create(
    float a, float b, float c, float d) {
  return std::make_unique<CubicBezierInterpolator>(a, b, c, d);
}

std::unique_ptr<CubicBezierInterpolator> CubicBezierInterpolator::Create(
    float x, float y) {
  // A cubic Bézier can be made identical to a quadratic one by
  // copying the end points, and placing its 2 middle control 2/3 along line
  // segments from the end points to the quadratic curve's middle control point.
  constexpr float kScaleFactor = 2.0f / 3.0f;
  FloatPoint p1(x, y), p2(1.0f, 1.0f);
  FloatPoint q1 = p1;
  q1.Scale(kScaleFactor, kScaleFactor);
  FloatPoint v2 = p1 - p2;
  v2.Scale(kScaleFactor, kScaleFactor);
  FloatPoint q2 = p2 + v2;
  return std::make_unique<CubicBezierInterpolator>(q1.x(), q1.y(), q2.x(),
                                                   q2.y());
}

std::unique_ptr<Interpolator> CubicBezierInterpolator::Clone() {
  return CubicBezierInterpolator::Create(a_, b_, c_, d_);
}

float CubicBezierInterpolator::Interpolate(float t) {
  if (t < 0.0) return ToFinite(0.0 + start_gradient_ * t);
  if (t > 1.0) return ToFinite(1.0 + end_gradient_ * (t - 1.0));
  return SampleCurveY(SolveCurveX(t, kBezierEpsilon));
}

double CubicBezierInterpolator::SolveCurveX(double x, double epsilon) const {
  double t0;
  double t1;
  double t2 = x;
  double x2;
  double d2;
  int i;

  // Linear interpolation of spline curve for initial guess.
  double delta_t = 1.0 / (CUBIC_BEZIER_SPLINE_SAMPLES - 1);
  for (i = 1; i < CUBIC_BEZIER_SPLINE_SAMPLES; i++) {
    if (x <= spline_samples_[i]) {
      t1 = delta_t * i;
      t0 = t1 - delta_t;
      t2 = t0 + (t1 - t0) * (x - spline_samples_[i - 1]) /
                    (spline_samples_[i] - spline_samples_[i - 1]);
      break;
    }
  }

  // Perform a few iterations of Newton's method -- normally very fast.
  // See https://en.wikipedia.org/wiki/Newton%27s_method.
  double newton_epsilon = std::min(kBezierEpsilon, epsilon);
  for (i = 0; i < kMaxNewtonIterations; i++) {
    x2 = SampleCurveX(t2) - x;
    if (fabs(x2) < newton_epsilon) return t2;
    d2 = SampleCurveDerivativeX(t2);
    if (fabs(d2) < kBezierEpsilon) break;
    t2 = t2 - x2 / d2;
  }
  if (fabs(x2) < epsilon) return t2;

  // Fall back to the bisection method for reliability.
  while (t0 < t1) {
    x2 = SampleCurveX(t2);
    if (fabs(x2 - x) < epsilon) return t2;
    if (x > x2)
      t0 = t2;
    else
      t1 = t2;
    t2 = (t1 + t0) * .5;
  }

  // Failure.
  return t2;
}

double CubicBezierInterpolator::Slop(double x) const {
  x = std::clamp(x, 0.0, 1.0);
  double t = SolveCurveX(x, kBezierEpsilon);
  double dx = SampleCurveDerivativeX(t);
  double dy = SampleCurveDerivativeY(t);
  // TODO(crbug.com/40207101): We should clamp NaN to a proper value.
  // Please see the issue for detail.
  if (!dx && !dy) return 0;
  return ToFinite(dy / dx);
}

double CubicBezierInterpolator::Velocity(double time) const {
  return Slop(time);
}

}  // namespace clay
