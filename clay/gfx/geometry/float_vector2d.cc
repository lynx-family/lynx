// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_vector2d.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace clay {

std::string FloatVector2d::ToString() const {
  const char* fmt = "[%f %f]";
  int sz = std::snprintf(nullptr, 0, fmt, x_, y_);
  std::vector<char> buf(sz + 1);
  std::snprintf(&buf[0], buf.size(), fmt, x_, y_);
  return std::string(buf.data());
}

bool FloatVector2d::IsZero() const { return x_ == 0 && y_ == 0; }

void FloatVector2d::Add(const FloatVector2d& other) {
  x_ += other.x_;
  y_ += other.y_;
}

void FloatVector2d::Subtract(const FloatVector2d& other) {
  x_ -= other.x_;
  y_ -= other.y_;
}

double FloatVector2d::LengthSquared() const {
  return static_cast<double>(x_) * x_ + static_cast<double>(y_) * y_;
}

float FloatVector2d::Length() const {
  return static_cast<float>(std::sqrt(LengthSquared()));
}

void FloatVector2d::Scale(float x_scale, float y_scale) {
  x_ *= x_scale;
  y_ *= y_scale;
}

double CrossProduct(const FloatVector2d& lhs, const FloatVector2d& rhs) {
  return static_cast<double>(lhs.x()) * rhs.y() -
         static_cast<double>(lhs.y()) * rhs.x();
}

double DotProduct(const FloatVector2d& lhs, const FloatVector2d& rhs) {
  return static_cast<double>(lhs.x()) * rhs.x() +
         static_cast<double>(lhs.y()) * rhs.y();
}

FloatVector2d ScaleVector2d(const FloatVector2d& v, float x_scale,
                            float y_scale) {
  FloatVector2d scaled_v(v);
  scaled_v.Scale(x_scale, y_scale);
  return scaled_v;
}

}  // namespace clay
