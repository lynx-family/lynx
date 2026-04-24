// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_CUBIC_BEZIER_H_
#define GFX_ANIMATION_CUBIC_BEZIER_H_

namespace lynx {
namespace gfx {

// Cubic bezier implementation for CSS timing functions.
#define CUBIC_BEZIER_SPLINE_SAMPLES 11
class CubicBezier {
 public:
  CubicBezier(double p1x, double p1y, double p2x, double p2y);
  CubicBezier(const CubicBezier& other);
  ~CubicBezier();

  double SampleCurveX(double t) const;
  double SampleCurveY(double t) const;
  double SampleCurveDerivativeX(double t) const;
  double SampleCurveDerivativeY(double t) const;

  static double GetDefaultEpsilon();

  double SolveCurveX(double x, double epsilon) const;
  double Solve(double x) const;
  double SolveWithEpsilon(double x, double epsilon) const;
  double Slope(double x) const;
  double SlopeWithEpsilon(double x, double epsilon) const;

  double GetX1() const { return p1x_; }
  double GetY1() const { return p1y_; }
  double GetX2() const { return p2x_; }
  double GetY2() const { return p2y_; }

  double range_min() const { return range_min_; }
  double range_max() const { return range_max_; }

 private:
  void InitCoefficients(double p1x, double p1y, double p2x, double p2y);
  void InitGradients(double p1x, double p1y, double p2x, double p2y);
  void InitRange(double p1y, double p2y);
  void InitSpline();

  double SolveWithEpsilonInternal(double x, double epsilon) const;
  double SlopeWithEpsilonInternal(double x, double epsilon) const;
  double SolveCurveXInternal(double x, double epsilon) const;

  double p1x_ = 0.0;
  double p1y_ = 0.0;
  double p2x_ = 0.0;
  double p2y_ = 0.0;

  double ax_ = 0.0;
  double bx_ = 0.0;
  double cx_ = 0.0;
  double ay_ = 0.0;
  double by_ = 0.0;
  double cy_ = 0.0;

  double start_gradient_ = 0.0;
  double end_gradient_ = 0.0;

  double range_min_ = 0.0;
  double range_max_ = 1.0;

  double spline_samples_[CUBIC_BEZIER_SPLINE_SAMPLES] = {0};
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_CUBIC_BEZIER_H_
