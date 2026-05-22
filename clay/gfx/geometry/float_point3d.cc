// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_point3d.h"

#include <vector>

namespace clay {

std::string FloatPoint3d::ToString() const {
  auto str_printf = [=](char* str_buf, size_t size) {
    return std::snprintf(str_buf, size, "%f,%f,%f", x_, y_, z_);
  };
  int sz = str_printf(nullptr, 0);
  std::vector<char> buf(sz + 1);
  str_printf(&buf[0], buf.size());
  return std::string(buf.data());
}

FloatPoint3d operator+(const FloatPoint3d& lhs, const FloatVector3d& rhs) {
  float x = lhs.x() + rhs.x();
  float y = lhs.y() + rhs.y();
  float z = lhs.z() + rhs.z();
  return FloatPoint3d(x, y, z);
}

// Subtract a vector from a point, producing a new point offset by the vector's
// inverse.
FloatPoint3d operator-(const FloatPoint3d& lhs, const FloatVector3d& rhs) {
  float x = lhs.x() - rhs.x();
  float y = lhs.y() - rhs.y();
  float z = lhs.z() - rhs.z();
  return FloatPoint3d(x, y, z);
}

// Subtract one point from another, producing a vector that represents the
// distances between the two points along each axis.
FloatVector3d operator-(const FloatPoint3d& lhs, const FloatPoint3d& rhs) {
  float x = lhs.x() - rhs.x();
  float y = lhs.y() - rhs.y();
  float z = lhs.z() - rhs.z();
  return FloatVector3d(x, y, z);
}

}  // namespace clay
