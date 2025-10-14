// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_vector3d.h"

#include <cmath>
#include <cstdio>
#include <vector>

#include "clay/gfx/geometry/math_util.h"

namespace {
const double kEpsilon = 1.0e-6;
}

namespace clay {

std::string FloatVector3d::ToString() const {
  const char* fmt = "[%f %f %f]";
  int sz = std::snprintf(nullptr, 0, fmt, x_, y_, z_);
  std::vector<char> buf(sz + 1);
  std::snprintf(&buf[0], buf.size(), fmt, x_, y_, z_);
  return std::string(buf.data());
}

bool FloatVector3d::IsZero() const { return x_ == 0 && y_ == 0 && z_ == 0; }

void FloatVector3d::Add(const FloatVector3d& other) {
  x_ += other.x_;
  y_ += other.y_;
  z_ += other.z_;
}

void FloatVector3d::Subtract(const FloatVector3d& other) {
  x_ -= other.x_;
  y_ -= other.y_;
  z_ -= other.z_;
}

double FloatVector3d::LengthSquared() const {
  return static_cast<double>(x_) * x_ + static_cast<double>(y_) * y_ +
         static_cast<double>(z_) * z_;
}

float FloatVector3d::Length() const {
  return static_cast<float>(std::sqrt(LengthSquared()));
}

void FloatVector3d::Scale(float x_scale, float y_scale, float z_scale) {
  x_ *= x_scale;
  y_ *= y_scale;
  z_ *= z_scale;
}

void FloatVector3d::Cross(const FloatVector3d& other) {
  double dx = x_;
  double dy = y_;
  double dz = z_;
  float x = static_cast<float>(dy * other.z() - dz * other.y());
  float y = static_cast<float>(dz * other.x() - dx * other.z());
  float z = static_cast<float>(dx * other.y() - dy * other.x());
  x_ = x;
  y_ = y;
  z_ = z;
}

bool FloatVector3d::GetNormalized(FloatVector3d* out) const {
  double length_squared = LengthSquared();
  *out = *this;
  if (length_squared < kEpsilon * kEpsilon) {
    return false;
  }
  out->Scale(1 / sqrt(length_squared));
  return true;
}

float DotProduct(const FloatVector3d& lhs, const FloatVector3d& rhs) {
  return lhs.x() * rhs.x() + lhs.y() * rhs.y() + lhs.z() * rhs.z();
}

FloatVector3d ScaleVector3d(const FloatVector3d& v, float x_scale,
                            float y_scale, float z_scale) {
  FloatVector3d scaled_v(v);
  scaled_v.Scale(x_scale, y_scale, z_scale);
  return scaled_v;
}

float AngleBetweenVectorsInDegrees(const FloatVector3d& base,
                                   const FloatVector3d& other) {
  // Clamp the resulting value to prevent potential NANs from floating point
  // precision issues.
  return RadToDeg(std::acos(
      fmax(fmin(DotProduct(base, other) / base.Length() / other.Length(), 1.f),
           -1.f)));
}

float ClockwiseAngleBetweenVectorsInDegrees(const FloatVector3d& base,
                                            const FloatVector3d& other,
                                            const FloatVector3d& normal) {
  float angle = AngleBetweenVectorsInDegrees(base, other);
  FloatVector3d cross(base);
  cross.Cross(other);

  // If the dot product of this cross product is normal, it means that the
  // shortest angle between |base| and |other| was counterclockwise with respect
  // to the surface represented by |normal| and this angle must be reversed.
  if (DotProduct(cross, normal) > 0.0f) {
    angle = 360.0f - angle;
  }
  return angle;
}

}  // namespace clay
