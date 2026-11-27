// Copyright 2026 The Lynx Authors. All rights reserved.
// Copyright 2014 The Android Open Source Project.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <math.h>

#include "gfx/public/path_parser.h"

bool LBSAppendSvgArc(const LBSSvgArcSink* sink, float start_x, float start_y,
                     float radius_x, float radius_y, float rotation, bool large,
                     bool sweep, float end_x, float end_y) {
  if (!isfinite(start_x) || !isfinite(start_y) || !isfinite(end_x) ||
      !isfinite(end_y) || !isfinite(radius_x) || !isfinite(radius_y) ||
      !isfinite(rotation)) {
    return false;
  }
  if (start_x == end_x && start_y == end_y) return true;
  double rx = fabs(radius_x);
  double ry = fabs(radius_y);
  if (rx == 0 || ry == 0) {
    sink->LineTo(sink->ctx, end_x, end_y);
    return true;
  }

  const double pi = 3.14159265358979323846;
  const double theta = fmod(rotation, 360.) * pi / 180.;
  const double cosine = cos(theta);
  const double sine = sin(theta);
  const double dx = ((double)start_x - end_x) / 2.;
  const double dy = ((double)start_y - end_y) / 2.;
  const double x = cosine * dx + sine * dy;
  const double y = -sine * dx + cosine * dy;
  double nx = x / rx;
  double ny = y / ry;
  double distance = nx * nx + ny * ny;
  if (distance > 1.) {
    // Match the radius correction used with the 45-degree approximation.
    const double factor = sqrt(distance) / 0.999995;
    rx *= factor;
    ry *= factor;
    nx = x / rx;
    ny = y / ry;
    distance = nx * nx + ny * ny;
  }
  const double coefficient =
      (large == sweep ? -1. : 1.) * sqrt(fmax(0., (1. - distance) / distance));
  const double cx = coefficient * rx * ny;
  const double cy = -coefficient * ry * nx;
  const double center_x =
      cosine * cx - sine * cy + ((double)start_x + end_x) / 2.;
  const double center_y =
      sine * cx + cosine * cy + ((double)start_y + end_y) / 2.;
  const double a = rx * cosine;
  const double b = rx * sine;
  const double c = -ry * sine;
  const double d = ry * cosine;
  if (!isfinite(center_x) || !isfinite(center_y) || !isfinite(a) ||
      !isfinite(b) || !isfinite(c) || !isfinite(d)) {
    return false;
  }

  double ux = (x - cx) / rx;
  double uy = (y - cy) / ry;
  const double vx = (-x - cx) / rx;
  const double vy = (-y - cy) / ry;
  // The chord retains small sweeps that disappear when subtracting angles.
  const double minor_angle = 2. * asin(fmin(1., sqrt(distance)));
  const double delta =
      (large ? 2. * pi - minor_angle : minor_angle) * (sweep ? 1. : -1.);
  const int segments = (int)fmax(1., ceil(fabs(delta) / (pi / 4.)));
  const double step = delta / segments;
  const double step_cosine = cos(step);
  const double step_sine = sin(step);
  const double tangent = tan(step / 2.);
  const double alpha =
      step_sine * (sqrt(4. + 3. * tangent * tangent) - 1.) / 3.;

  double px = start_x;
  double py = start_y;
  for (int i = 0; i < segments; ++i) {
    const bool last = i + 1 == segments;
    const double next_x = last ? vx : ux * step_cosine - uy * step_sine;
    const double next_y = last ? vy : ux * step_sine + uy * step_cosine;
    const double qx = last ? end_x : center_x + a * next_x + c * next_y;
    const double qy = last ? end_y : center_y + b * next_x + d * next_y;
    // Use the input endpoint directly to avoid cancellation on shallow arcs.
    sink->CubicTo(sink->ctx, px + alpha * (-a * uy + c * ux),
                  py + alpha * (-b * uy + d * ux),
                  qx - alpha * (-a * next_y + c * next_x),
                  qy - alpha * (-b * next_y + d * next_x), qx, qy);
    px = qx;
    py = qy;
    ux = next_x;
    uy = next_y;
  }
  return true;
}
