// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_RECT_H_
#define CLAY_GFX_GEOMETRY_RECT_H_

#include <algorithm>

#include "clay/gfx/geometry/point.h"
#include "clay/gfx/geometry/size.h"
#include "skity/geometry/rect.hpp"

namespace clay {

class Rect {
 public:
  Rect() = default;
  Rect(const Point& location, const Size& size)
      : location_(location), size_(size) {}
  Rect(const Size& size) : size_(size) {}
  Rect(int x, int y, int w, int h) : location_(x, y), size_(w, h) {}

  int x() const { return location_.x(); }
  int y() const { return location_.y(); }
  int MaxX() const { return x() + width(); }
  int MaxY() const { return y() + height(); }
  int width() const { return size_.width(); }
  int height() const { return size_.height(); }

  void SetX(int x) { location_.SetX(x); }
  void SetY(int y) { location_.SetY(y); }
  void SetWidth(int width) { size_.SetWidth(width); }
  void SetHeight(int height) { size_.SetHeight(height); }

  const Point& location() const { return location_; }
  const Size& size() const { return size_; }
  void SetLocation(const Point& location) { location_ = location; }
  void SetSize(const Size& size) { size_ = size; }

  bool IsEmpty() const { return size_.IsEmpty(); }

  void Move(int dx, int dy) { location_.Move(dx, dy); }

  void Expand(int dw, int dh) { size_.Expand(dw, dh); }

  void Expand(float top, float right, float bottom, float left) {
    location_.Move(-left, -top);
    size_.Expand(left + right, top + bottom);
  }

  void Contract(int dw, int dh) { size_.Expand(-dw, -dh); }

  bool Contains(int px, int py) const {
    return px >= x() && px < MaxX() && py >= y() && py < MaxY();
  }

  bool Contains(const Point& point) const {
    return Contains(point.x(), point.y());
  }

  bool Contains(const Rect& other) const {
    return x() <= other.x() && MaxX() >= other.MaxX() && y() <= other.y() &&
           MaxY() >= other.MaxY();
  }

  bool IsIntersects(const Rect& other) const {
    return !IsEmpty() && !other.IsEmpty() && x() < other.MaxX() &&
           other.x() < MaxX() && y() < other.MaxY() && other.y() < MaxY();
  }

  void Intersect(const Rect& other) {
    int left = std::max(x(), other.x());
    int top = std::max(y(), other.y());
    int right = std::min(MaxX(), other.MaxX());
    int bottom = std::min(MaxY(), other.MaxY());

    // Return a clean empty rectangle for non-intersecting cases.
    if (left >= right || top >= bottom) {
      left = 0;
      top = 0;
      right = 0;
      bottom = 0;
    }

    location_ = Point(left, top);
    size_ = Size(right - left, bottom - top);
  }

  void Unite(const Rect& other) {
    // Handle empty special cases first.
    if (other.IsEmpty()) return;
    if (IsEmpty()) {
      *this = other;
      return;
    }

    Point new_location(std::min(x(), other.x()), std::min(y(), other.y()));
    Point new_max_point(std::max(MaxX(), other.MaxX()),
                        std::max(MaxY(), other.MaxY()));

    location_ = new_location;
    size_ = new_max_point - new_location;
  }

  void Clear() {
    location_.SetX(0);
    location_.SetY(0);
    size_.SetWidth(0);
    size_.SetHeight(0);
  }

  operator skity::Rect() const {
    skity::Rect rect =
        skity::Rect::MakeXYWH(x(), y(), MaxX() - x(), MaxY() - y());
    return rect;
  }

 private:
  Point location_;
  Size size_;
};

inline bool operator==(const Rect& lhs, const Rect& rhs) {
  return lhs.location() == rhs.location() && lhs.size() == rhs.size();
}

inline bool operator!=(const Rect& lhs, const Rect& rhs) {
  return !(lhs == rhs);
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_RECT_H_
