// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_FLOAT_POINT_H_
#define CLAY_GFX_GEOMETRY_FLOAT_POINT_H_

#include <math.h>

#include <string>

#include "clay/gfx/geometry/float_size.h"
#include "clay/gfx/geometry/size.h"

namespace clay {
class Point;
class Size;

class FloatPoint {
 public:
  FloatPoint() : x_(0.0f), y_(0.0f) {}
  FloatPoint(float x, float y) : x_(x), y_(y) {}
  explicit FloatPoint(const Size& size) : x_(size.width()), y_(size.height()) {}
  explicit FloatPoint(const Point& location);

  float x() const { return x_; }
  float y() const { return y_; }
  float distance() const { return hypotf(x(), y()); }

  void SetX(float x) { x_ = x; }
  void SetY(float y) { y_ = y; }

  void MoveBy(const FloatSize& float_size) {
    Move(float_size.width(), float_size.height());
  }
  void MoveBy(const FloatPoint& offset) { Move(offset.x(), offset.y()); }
  void Move(int dx, float dy) {
    x_ += dx;
    y_ += dy;
  }
  void Scale(float sx, float sy) {
    x_ = x_ * sx;
    y_ = y_ * sy;
  }

  bool IsOrigin() const { return (x_ == 0 && y_ == 0); }

  FloatPoint ExpandedTo(const FloatPoint& other) const {
    return FloatPoint(x_ > other.x_ ? x_ : other.x_,
                      y_ > other.y_ ? y_ : other.y_);
  }

  FloatPoint ShrunkTo(const FloatPoint& other) const {
    return FloatPoint(x_ < other.x_ ? x_ : other.x_,
                      y_ < other.y_ ? y_ : other.y_);
  }

  std::string ToString() const {
    return "FloatPoint{" + std::to_string(x_) + "," + std::to_string(y_) + "}";
  }

  static FloatPoint Lerp(FloatPoint a, FloatPoint b, float t) {
    return {a.x_ + (b.x_ - a.x_) * t, a.y_ + (b.y_ - a.y_) * t};
  }

 private:
  float x_, y_;
};

inline FloatPoint& operator+=(FloatPoint& a, const FloatPoint& b) {
  a.Move(b.x(), b.y());
  return a;
}

inline FloatPoint& operator+=(FloatPoint& a, const FloatSize& b) {
  a.Move(b.width(), b.height());
  return a;
}

inline FloatPoint& operator-=(FloatPoint& a, const FloatPoint& b) {
  a.Move(-b.x(), -b.y());
  return a;
}

inline FloatPoint& operator-=(FloatPoint& a, const FloatSize& b) {
  a.Move(-b.width(), -b.height());
  return a;
}

inline FloatPoint operator+(const FloatPoint& a, const FloatPoint& b) {
  return FloatPoint(a.x() + b.x(), a.y() + b.y());
}

inline FloatPoint operator-(const FloatPoint& a, const FloatPoint& b) {
  return FloatPoint(a.x() - b.x(), a.y() - b.y());
}

inline FloatPoint operator*(const FloatPoint& a, float factor) {
  return FloatPoint(a.x() * factor, a.y() * factor);
}

inline bool operator==(const FloatPoint& a, const FloatPoint& b) {
  return a.x() == b.x() && a.y() == b.y();
}

inline bool operator!=(const FloatPoint& a, const FloatPoint& b) {
  return a.x() != b.x() || a.y() != b.y();
}

inline bool FindIntersection(const FloatPoint& p1, const FloatPoint& p2,
                             const FloatPoint& d1, const FloatPoint& d2,
                             FloatPoint* intersection) {
  float px_length = p2.x() - p1.x();
  float py_length = p2.y() - p1.y();

  float dx_length = d2.x() - d1.x();
  float dy_length = d2.y() - d1.y();

  float denom = px_length * dy_length - py_length * dx_length;
  if (!denom) {
    return false;
  }

  float param =
      ((d1.x() - p1.x()) * dy_length - (d1.y() - p1.y()) * dx_length) / denom;

  intersection->SetX(p1.x() + param * px_length);
  intersection->SetY(p1.y() + param * py_length);
  return true;
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_POINT_H_
