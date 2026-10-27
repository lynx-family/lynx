// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_FLOAT_VECTOR3D_H_
#define CLAY_GFX_GEOMETRY_FLOAT_VECTOR3D_H_

#include <string>

#include "clay/gfx/geometry/float_vector2d.h"

namespace clay {

class FloatVector3d {
 public:
  constexpr FloatVector3d() : x_(0), y_(0), z_(0) {}
  constexpr FloatVector3d(float x, float y, float z) : x_(x), y_(y), z_(z) {}

  constexpr explicit FloatVector3d(const FloatVector2d& other)
      : x_(other.x()), y_(other.y()), z_(0) {}

  constexpr float x() const { return x_; }
  void SetX(float x) { x_ = x; }

  constexpr float y() const { return y_; }
  void SetY(float y) { y_ = y; }

  constexpr float z() const { return z_; }
  void SetZ(float z) { z_ = z; }

  // True if all components of the vector are 0.
  bool IsZero() const;

  // Add the components of the |other| vector to the current vector.
  void Add(const FloatVector3d& other);
  // Subtract the components of the |other| vector from the current vector.
  void Subtract(const FloatVector3d& other);

  void operator+=(const FloatVector3d& other) { Add(other); }
  void operator-=(const FloatVector3d& other) { Subtract(other); }

  void SetToMin(const FloatVector3d& other) {
    x_ = x_ <= other.x_ ? x_ : other.x_;
    y_ = y_ <= other.y_ ? y_ : other.y_;
    z_ = z_ <= other.z_ ? z_ : other.z_;
  }

  void SetToMax(const FloatVector3d& other) {
    x_ = x_ >= other.x_ ? x_ : other.x_;
    y_ = y_ >= other.y_ ? y_ : other.y_;
    z_ = z_ >= other.z_ ? z_ : other.z_;
  }

  // Gives the square of the diagonal length of the vector.
  double LengthSquared() const;
  // Gives the diagonal length of the vector.
  float Length() const;

  // Scale all components of the vector by |scale|.
  void Scale(float scale) { Scale(scale, scale, scale); }
  // Scale the each component of the vector by the given scale factors.
  void Scale(float x_scale, float y_scale, float z_scale);

  // Take the cross product of this vector with |other| and become the result.
  void Cross(const FloatVector3d& other);

  // |out| is assigned a unit-length vector in the direction of |this| iff
  // this function returns true. It can return false if |this| is too short.
  bool GetNormalized(FloatVector3d* out) const;

  std::string ToString() const;

 private:
  float x_;
  float y_;
  float z_;
};

inline bool operator==(const FloatVector3d& lhs, const FloatVector3d& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.z() == rhs.z();
}

inline bool operator!=(const FloatVector3d& lhs, const FloatVector3d& rhs) {
  return !(lhs == rhs);
}

inline FloatVector3d operator-(const FloatVector3d& v) {
  return FloatVector3d(-v.x(), -v.y(), -v.z());
}

inline FloatVector3d operator+(const FloatVector3d& lhs,
                               const FloatVector3d& rhs) {
  FloatVector3d result = lhs;
  result.Add(rhs);
  return result;
}

inline FloatVector3d operator-(const FloatVector3d& lhs,
                               const FloatVector3d& rhs) {
  FloatVector3d result = lhs;
  result.Add(-rhs);
  return result;
}

// Return the cross product of two vectors.
inline FloatVector3d CrossProduct(const FloatVector3d& lhs,
                                  const FloatVector3d& rhs) {
  FloatVector3d result = lhs;
  result.Cross(rhs);
  return result;
}

// Return the dot product of two vectors.
float DotProduct(const FloatVector3d& lhs, const FloatVector3d& rhs);

// Return a vector that is |v| scaled by the given scale factors along each
// axis.
FloatVector3d ScaleVector3d(const FloatVector3d& v, float x_scale,
                            float y_scale, float z_scale);

// Return a vector that is |v| scaled by the components of |s|
inline FloatVector3d ScaleVector3d(const FloatVector3d& v,
                                   const FloatVector3d& s) {
  return ScaleVector3d(v, s.x(), s.y(), s.z());
}

// Return a vector that is |v| scaled by the given scale factor.
inline FloatVector3d ScaleVector3d(const FloatVector3d& v, float scale) {
  return ScaleVector3d(v, scale, scale, scale);
}

// Returns the angle between |base| and |other| in degrees.
float AngleBetweenVectorsInDegrees(const FloatVector3d& base,
                                   const FloatVector3d& other);

// Returns the clockwise angle between |base| and |other| where |normal| is the
// normal of the virtual surface to measure clockwise according to.
float ClockwiseAngleBetweenVectorsInDegrees(const FloatVector3d& base,
                                            const FloatVector3d& other,
                                            const FloatVector3d& normal);

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_VECTOR3D_H_
