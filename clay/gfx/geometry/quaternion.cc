// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/quaternion.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "clay/gfx/geometry/float_vector3d.h"

namespace clay {

namespace {

const double kEpsilon = 1e-5;

}  // namespace

Quaternion::Quaternion(const FloatVector3d& axis, double theta) {
  // Rotation angle is the product of |angle| and the magnitude of |axis|.
  double length = axis.Length();
  if (std::abs(length) < kEpsilon) {
    return;
  }

  FloatVector3d normalized = axis;
  normalized.Scale(1.0 / length);

  theta *= 0.5;
  double s = sin(theta);
  x_ = normalized.x() * s;
  y_ = normalized.y() * s;
  z_ = normalized.z() * s;
  w_ = cos(theta);
}

Quaternion::Quaternion(const FloatVector3d& from, const FloatVector3d& to) {
  double dot = DotProduct(from, to);
  double norm = sqrt(from.LengthSquared() * to.LengthSquared());
  double real = norm + dot;
  FloatVector3d axis;
  if (real < kEpsilon * norm) {
    real = 0.0f;
    axis = std::abs(from.x()) > std::abs(from.z())
               ? FloatVector3d{-from.y(), from.x(), 0.0}
               : FloatVector3d{0.0, -from.z(), from.y()};
  } else {
    axis = CrossProduct(from, to);
  }
  x_ = axis.x();
  y_ = axis.y();
  z_ = axis.z();
  w_ = real;
  *this = this->Normalized();
}

Quaternion Quaternion::FromAxisAngle(double x, double y, double z,
                                     double angle) {
  double length = std::sqrt(x * x + y * y + z * z);
  if (std::abs(length) < kEpsilon) {
    return Quaternion(0, 0, 0, 1);
  }

  double scale = std::sin(0.5 * angle) / length;
  return Quaternion(scale * x, scale * y, scale * z, std::cos(0.5 * angle));
}

// Adapted from https://www.euclideanspace.com/maths/algebra/realNormedAlgebra/
// quaternions/slerp/index.htm
Quaternion Quaternion::Slerp(const Quaternion& to, double t) const {
  Quaternion from = *this;

  double cos_half_angle =
      from.x_ * to.x_ + from.y_ * to.y_ + from.z_ * to.z_ + from.w_ * to.w_;
  if (cos_half_angle < 0) {
    // Since the half angle is > 90 degrees, the full rotation angle would
    // exceed 180 degrees. The quaternions (x, y, z, w) and (-x, -y, -z, -w)
    // represent the same rotation. Flipping the orientation of either
    // quaternion ensures that the half angle is less than 90 and that we are
    // taking the shortest path.
    from = from.flip();
    cos_half_angle = -cos_half_angle;
  }

  // Ensure that acos is well behaved at the boundary.
  if (cos_half_angle > 1) {
    cos_half_angle = 1;
  }

  double sin_half_angle = std::sqrt(1.0 - cos_half_angle * cos_half_angle);
  if (sin_half_angle < kEpsilon) {
    // Quaternions share common axis and angle.
    return *this;
  }

  double half_angle = std::acos(cos_half_angle);

  double scaleA = std::sin((1 - t) * half_angle) / sin_half_angle;
  double scaleB = std::sin(t * half_angle) / sin_half_angle;

  return (scaleA * from) + (scaleB * to);
}

Quaternion Quaternion::Lerp(const Quaternion& q, double t) const {
  return (((1.0 - t) * *this) + (t * q)).Normalized();
}

double Quaternion::Length() const {
  return x_ * x_ + y_ * y_ + z_ * z_ + w_ * w_;
}

Quaternion Quaternion::Normalized() const {
  double length = Length();
  if (length < kEpsilon) {
    return *this;
  }
  return *this / sqrt(length);
}

std::string Quaternion::ToString() const {
  // q = (con(abs(v_theta)/2), v_theta/abs(v_theta) * sin(abs(v_theta)/2))
  float abs_theta = acos(w_) * 2;
  float scale = 1. / sin(abs_theta * .5);
  FloatVector3d v(x_, y_, z_);
  v.Scale(scale);

  auto str_printf = [=](char* str_buf, size_t size) {
    return std::snprintf(str_buf, size, "[%f %f %f %f], v:%s, θ:%fπ", x_, y_,
                         z_, w_, v.ToString().c_str(), abs_theta / M_PI);
  };
  int sz = str_printf(nullptr, 0);
  std::vector<char> buf(sz + 1);
  str_printf(&buf[0], buf.size());
  return std::string(buf.data());
}

}  // namespace clay
