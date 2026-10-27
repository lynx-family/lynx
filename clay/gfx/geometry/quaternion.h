// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_QUATERNION_H_
#define CLAY_GFX_GEOMETRY_QUATERNION_H_

#include <string>

namespace clay {

class FloatVector3d;

class Quaternion {
 public:
  constexpr Quaternion() = default;
  constexpr Quaternion(double x, double y, double z, double w)
      : x_(x), y_(y), z_(z), w_(w) {}
  Quaternion(const FloatVector3d& axis, double angle);

  // Constructs a quaternion representing a rotation between |from| and |to|.
  Quaternion(const FloatVector3d& from, const FloatVector3d& to);

  static Quaternion FromAxisAngle(double x, double y, double z, double angle);

  constexpr double x() const { return x_; }
  void SetX(double x) { x_ = x; }

  constexpr double y() const { return y_; }
  void SetY(double y) { y_ = y; }

  constexpr double z() const { return z_; }
  void SetZ(double z) { z_ = z; }

  constexpr double w() const { return w_; }
  void set_w(double w) { w_ = w; }

  Quaternion operator+(const Quaternion& q) const {
    return {q.x_ + x_, q.y_ + y_, q.z_ + z_, q.w_ + w_};
  }

  Quaternion operator*(const Quaternion& q) const {
    return {w_ * q.x_ + x_ * q.w_ + y_ * q.z_ - z_ * q.y_,
            w_ * q.y_ - x_ * q.z_ + y_ * q.w_ + z_ * q.x_,
            w_ * q.z_ + x_ * q.y_ - y_ * q.x_ + z_ * q.w_,
            w_ * q.w_ - x_ * q.x_ - y_ * q.y_ - z_ * q.z_};
  }

  Quaternion inverse() const { return {-x_, -y_, -z_, w_}; }

  Quaternion flip() const { return {-x_, -y_, -z_, -w_}; }

  // Blends with the given quaternion, |q|, via spherical linear interpolation.
  // Values of |t| in the range [0, 1] will interpolate between |this| and |q|,
  // and values outside that range will extrapolate beyond in either direction.
  Quaternion Slerp(const Quaternion& q, double t) const;

  // Blends with the given quaternion, |q|, via linear interpolation. This is
  // rarely what you want. Use only if you know what you're doing.
  // Values of |t| in the range [0, 1] will interpolate between |this| and |q|,
  // and values outside that range will extrapolate beyond in either direction.
  Quaternion Lerp(const Quaternion& q, double t) const;

  double Length() const;

  Quaternion Normalized() const;

  std::string ToString() const;

 private:
  double x_ = 0.0;
  double y_ = 0.0;
  double z_ = 0.0;
  double w_ = 1.0;
};

// |s| is an arbitrary, real constant.
inline Quaternion operator*(const Quaternion& q, double s) {
  return Quaternion(q.x() * s, q.y() * s, q.z() * s, q.w() * s);
}

// |s| is an arbitrary, real constant.
inline Quaternion operator*(double s, const Quaternion& q) {
  return Quaternion(q.x() * s, q.y() * s, q.z() * s, q.w() * s);
}

// |s| is an arbitrary, real constant.
inline Quaternion operator/(const Quaternion& q, double s) {
  double inv = 1.0 / s;
  return q * inv;
}

// Returns true if the x, y, z, w values of |lhs| and |rhs| are equal. Note that
// two quaternions can represent the same orientation with different values.
// This operator will return false in that scenario.
inline bool operator==(const Quaternion& lhs, const Quaternion& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.z() == rhs.z() &&
         lhs.w() == rhs.w();
}

inline bool operator!=(const Quaternion& lhs, const Quaternion& rhs) {
  return !(lhs == rhs);
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_QUATERNION_H_
