// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_CUBIC_BEZIER_INTERPOLATOR_H_
#define CLAY_GFX_ANIMATION_CUBIC_BEZIER_INTERPOLATOR_H_

#include <algorithm>
#include <memory>

#include "clay/gfx/animation/interpolator.h"

namespace clay {

#define CUBIC_BEZIER_SPLINE_SAMPLES 11

class CubicBezierInterpolator : public Interpolator {
 public:
  enum Type { Ease, EaseIn, EaseOut, EaseInOut };

  static std::unique_ptr<CubicBezierInterpolator> CreatePreset(Type type);
  static std::unique_ptr<CubicBezierInterpolator> Create(float a, float b,
                                                         float c, float d);
  // Create a quadratic Bézier curve using only one control point.
  static std::unique_ptr<CubicBezierInterpolator> Create(float x, float y);
  std::unique_ptr<Interpolator> Clone() override;

  CubicBezierInterpolator(float a, float b, float c, float d);

  void InitSpline();

  float Interpolate(float input) override;

  double ToFinite(double value) const;

  double Velocity(double time) const override;

  double SampleCurveX(double t) const {
    return ((ax_ * t + bx_) * t + cx_) * t;
  }
  double SampleCurveY(double t) const {
    return ToFinite(((ay_ * t + by_) * t + cy_) * t);
  }

  double SolveCurveX(double x, double epsilon) const;

  double SampleCurveDerivativeX(double t) const {
    return (3.0 * ax_ * t + 2.0 * bx_) * t + cx_;
  }

  double SampleCurveDerivativeY(double t) const {
    return ToFinite(
        ToFinite(ToFinite(3.0 * ay_) * t + ToFinite(2.0 * by_)) * t + cy_);
  }

  double Slop(double x) const;

 private:
  void InitCoefficients(double p1x, double p1y, double p2x, double p2y);

  void InitGradients(double p1x, double p1y, double p2x, double p2y);

  /// The x coordinate of the first control point.
  ///
  /// The line through the point (0, 0) and the first control point is tangent
  /// to the curve at the point (0, 0).
  const float a_;

  /// The y coordinate of the first control point.
  ///
  /// The line through the point (0, 0) and the first control point is tangent
  /// to the curve at the point (0, 0).
  const float b_;

  /// The x coordinate of the second control point.
  ///
  /// The line through the point (1, 1) and the second control point is
  /// tangent to the curve at the point (1, 1).
  const float c_;

  /// The y coordinate of the second control point.
  ///
  /// The line through the point (1, 1) and the second control point is
  /// tangent to the curve at the point (1, 1).
  const float d_;

  double ax_;
  double bx_;
  double cx_;

  double ay_;
  double by_;
  double cy_;

  double start_gradient_;
  double end_gradient_;

  double spline_samples_[CUBIC_BEZIER_SPLINE_SAMPLES];
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_CUBIC_BEZIER_INTERPOLATOR_H_
