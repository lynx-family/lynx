// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TRANSFORMS_QUATERNION_H_
#define CORE_ANIMATION_TRANSFORMS_QUATERNION_H_

namespace lynx {
namespace transforms {

struct Euler {
  Euler(float x, float y, float z) : x(x), y(y), z(z) {}
  Euler() = default;
  float x{0.0f};
  float y{0.0f};
  float z{0.0f};
};

class Quaternion {
 public:
  constexpr Quaternion() = default;
  constexpr Quaternion(double x, double y, double z, double w)
      : x_(x), y_(y), z_(z), w_(w) {}

  constexpr double x() const { return x_; }
  void set_x(double x) { x_ = x; }

  constexpr double y() const { return y_; }
  void set_y(double y) { y_ = y; }

  constexpr double z() const { return z_; }
  void set_z(double z) { z_ = z; }

  constexpr double w() const { return w_; }
  void set_w(double w) { w_ = w; }

  Quaternion operator+(const Quaternion& q) const {
    return {q.x_ + x_, q.y_ + y_, q.z_ + z_, q.w_ + w_};
  }

  Quaternion flip() const { return {-x_, -y_, -z_, -w_}; }

  // Blends with the given quaternion, |q|, via spherical linear interpolation.
  // Values of |t| in the range [0, 1] will interpolate between |this| and |q|,
  // and values outside that range will extrapolate beyond in either direction.
  Quaternion Slerp(const Quaternion& q, double t) const;

  Euler ConvertToEuler() const;

 private:
  double x_ = 0.0;
  double y_ = 0.0;
  double z_ = 0.0;
  double w_ = 1.0;
};

// |s| is an arbitrary, real constant.
inline Quaternion operator*(double s, const Quaternion& q) {
  return Quaternion(q.x() * s, q.y() * s, q.z() * s, q.w() * s);
}

}  // namespace transforms
}  // namespace lynx

#endif  // CORE_ANIMATION_TRANSFORMS_QUATERNION_H_
