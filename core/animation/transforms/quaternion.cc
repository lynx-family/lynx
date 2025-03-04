// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifdef OS_WIN
#define _USE_MATH_DEFINES
#endif

#include "core/animation/transforms/quaternion.h"

#include <cmath>

namespace lynx {
namespace transforms {

namespace {

const double kEpsilon = 1e-5;
const double kThreshold = 0.5f - kEpsilon;

}  // namespace

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
  if (cos_half_angle > 1) cos_half_angle = 1;

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

Euler Quaternion::ConvertToEuler() const {  // Z-Y-X Euler angles
  Euler euler;
  double test = w_ * y_ - x_ * z_;
  if (test < -kThreshold || test > kThreshold) {
    double sign = test > 0 ? 1.0f : -1.0f;
    euler.x = 0;
    euler.y = sign * (M_PI / 2.0f);
    euler.z = -2.0f * sign * atan2(x_, w_);
  } else {
    euler.x =
        atan2(2 * (y_ * z_ + w_ * x_), 1.0f - 2 * x_ * x_ - 2.0f * y_ * y_);
    euler.y = asin(-2 * (x_ * z_ - w_ * y_));
    euler.z =
        atan2(2 * (x_ * y_ + w_ * z_), 1.0f - 2 * y_ * y_ - 2.0f * z_ * z_);
  }
  return euler;
}

}  // namespace transforms
}  // namespace lynx
