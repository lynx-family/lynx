// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_FLOAT_POINT3D_H_
#define CLAY_GFX_GEOMETRY_FLOAT_POINT3D_H_

#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_vector3d.h"

namespace clay {

// A point has an x, y and z coordinate.
class FloatPoint3d {
 public:
  constexpr FloatPoint3d() : x_(0), y_(0), z_(0) {}
  constexpr FloatPoint3d(float x, float y, float z) : x_(x), y_(y), z_(z) {}

  constexpr explicit FloatPoint3d(const FloatPoint& point)
      : x_(point.x()), y_(point.y()), z_(0) {}

  void Scale(float scale) { Scale(scale, scale, scale); }

  void Scale(float x_scale, float y_scale, float z_scale) {
    SetPoint(x() * x_scale, y() * y_scale, z() * z_scale);
  }

  constexpr float x() const { return x_; }
  constexpr float y() const { return y_; }
  constexpr float z() const { return z_; }

  void SetX(float x) { x_ = x; }
  void SetY(float y) { y_ = y; }
  void SetZ(float z) { z_ = z; }

  void SetPoint(float x, float y, float z) {
    x_ = x;
    y_ = y;
    z_ = z;
  }

  // Offset the point by the given vector.
  void operator+=(const FloatVector3d& v) {
    x_ += v.x();
    y_ += v.y();
    z_ += v.z();
  }

  // Offset the point by the given vector's inverse.
  void operator-=(const FloatVector3d& v) {
    x_ -= v.x();
    y_ -= v.y();
    z_ -= v.z();
  }

  // Returns the squared euclidean distance between two points.
  float SquaredDistanceTo(const FloatPoint3d& other) const {
    float dx = x_ - other.x_;
    float dy = y_ - other.y_;
    float dz = z_ - other.z_;
    return dx * dx + dy * dy + dz * dz;
  }

  FloatPoint AsPointF() const { return FloatPoint(x_, y_); }

  // Returns a string representation of 3d point.
  std::string ToString() const;

 private:
  float x_;
  float y_;
  float z_;

  // copy/assign are allowed.
};

inline bool operator==(const FloatPoint3d& lhs, const FloatPoint3d& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y() && lhs.z() == rhs.z();
}

inline bool operator!=(const FloatPoint3d& lhs, const FloatPoint3d& rhs) {
  return !(lhs == rhs);
}

// Add a vector to a point, producing a new point offset by the vector.
FloatPoint3d operator+(const FloatPoint3d& lhs, const FloatVector3d& rhs);

// Subtract a vector from a point, producing a new point offset by the vector's
// inverse.
FloatPoint3d operator-(const FloatPoint3d& lhs, const FloatVector3d& rhs);

// Subtract one point from another, producing a vector that represents the
// distances between the two points along each axis.
FloatVector3d operator-(const FloatPoint3d& lhs, const FloatPoint3d& rhs);

inline FloatPoint3d PointAtOffsetFromOrigin(const FloatVector3d& offset) {
  return FloatPoint3d(offset.x(), offset.y(), offset.z());
}

inline FloatPoint3d ScalePoint(const FloatPoint3d& p, float x_scale,
                               float y_scale, float z_scale) {
  return FloatPoint3d(p.x() * x_scale, p.y() * y_scale, p.z() * z_scale);
}

inline FloatPoint3d ScalePoint(const FloatPoint3d& p, const FloatVector3d& v) {
  return FloatPoint3d(p.x() * v.x(), p.y() * v.y(), p.z() * v.z());
}

inline FloatPoint3d ScalePoint(const FloatPoint3d& p, float scale) {
  return ScalePoint(p, scale, scale, scale);
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_POINT3D_H_
