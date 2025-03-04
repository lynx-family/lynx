// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_GEOMETRY_POINT_H_
#define BASE_INCLUDE_GEOMETRY_POINT_H_

namespace lynx {
namespace base {
namespace geometry {

template <typename T>
class Point {
 public:
  Point() : x_(0), y_(0) {}
  Point(T x, T y) : x_(x), y_(y) {}

  T X() const { return x_; }
  T Y() const { return y_; }

  void SetX(T x) { x_ = x; }
  void SetY(T y) { y_ = y; }

  void MoveBy(const Point<T>& offset) { Move(offset.X(), offset.Y()); }
  void Move(T dx, T dy) {
    x_ += dx;
    y_ += dy;
  }

 private:
  T x_;
  T y_;
};

template <typename T>
inline Point<T> operator+(const Point<T>& a, const Point<T>& b) {
  return Point<T>(a.X() + b.X(), a.Y() + b.Y());
}

template <typename T>
inline Point<T>& operator+=(Point<T>& a, const Point<T>& b) {
  a.Move(b.X(), b.Y());
  return a;
}

template <typename T>
inline Point<T> operator-(const Point<T>& a, const Point<T>& b) {
  return Point<T>(a.X() - b.X(), a.Y() - b.Y());
}

template <typename T>
inline Point<T> operator-(const Point<T>& point) {
  return Point<T>(-point.X(), -point.Y());
}

template <typename T>
inline bool operator==(const Point<T>& a, const Point<T>& b) {
  return a.X() == b.X() && a.Y() == b.Y();
}

template <typename T>
inline bool operator!=(const Point<T>& a, const Point<T>& b) {
  return a.X() != b.X() || a.Y() != b.Y();
}

using IntPoint = Point<int>;
using FloatPoint = Point<float>;

}  // namespace geometry
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_GEOMETRY_POINT_H_
