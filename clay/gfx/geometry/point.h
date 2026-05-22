// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_POINT_H_
#define CLAY_GFX_GEOMETRY_POINT_H_

#include <math.h>

#include <string>

#include "clay/gfx/geometry/size.h"

namespace clay {

class Point {
 public:
  Point() : x_(0), y_(0) {}
  Point(int x, int y) : x_(x), y_(y) {}

  int x() const { return x_; }
  int y() const { return y_; }

  void SetX(int x) { x_ = x; }
  void SetY(int y) { y_ = y; }

  void MoveBy(const Point& offset) { Move(offset.x(), offset.y()); }
  void Move(int dx, int dy) {
    x_ += dx;
    y_ += dy;
  }
  void Scale(float sx, float sy) {
    x_ = lroundf(static_cast<float>(x_ * sx));
    y_ = lroundf(static_cast<float>(y_ * sy));
  }

  Point ExpandedTo(const Point& other) const {
    return Point(x_ > other.x_ ? x_ : other.x_, y_ > other.y_ ? y_ : other.y_);
  }

  Point ShrunkTo(const Point& other) const {
    return Point(x_ < other.x_ ? x_ : other.x_, y_ < other.y_ ? y_ : other.y_);
  }

  void ClampNegativeToZero() { *this = ExpandedTo(Point()); }

  std::string ToString() const {
    return "Point{" + std::to_string(x_) + "," + std::to_string(y_) + "}";
  }

 private:
  int x_, y_;
};

inline Point operator+(const Point& a, const Point& b) {
  return Point(a.x() + b.x(), a.y() + b.y());
}

inline Point operator-(const Point& point) {
  return Point(-point.x(), -point.y());
}

inline Size operator-(const Point& a, const Point& b) {
  return Size(a.x() - b.x(), a.y() - b.y());
}

inline bool operator==(const Point& a, const Point& b) {
  return a.x() == b.x() && a.y() == b.y();
}

inline bool operator!=(const Point& a, const Point& b) {
  return a.x() != b.x() || a.y() != b.y();
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_POINT_H_
