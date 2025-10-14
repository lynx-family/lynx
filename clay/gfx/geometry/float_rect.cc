// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_rect.h"

#include <algorithm>
#include <sstream>

#include "clay/gfx/geometry/rect.h"

namespace clay {

FloatRect::FloatRect(const Rect& rect)
    : location_(rect.location()), size_(rect.size()) {}

FloatRect::FloatRect(const skity::Rect& r)
    : location_(r.Left(), r.Top()), size_(r.Width(), r.Height()) {}

bool FloatRect::Contains(const FloatRect& other) const {
  return x() <= other.x() && MaxX() >= other.MaxX() && y() <= other.y() &&
         MaxY() >= other.MaxY();
}

void FloatRect::Intersect(const FloatRect& other) {
  float left = std::max(x(), other.x());
  float top = std::max(y(), other.y());
  float right = std::min(MaxX(), other.MaxX());
  float bottom = std::min(MaxY(), other.MaxY());

  // Return a clean empty rectangle for non-intersecting cases.
  if (left >= right || top >= bottom) {
    left = 0;
    top = 0;
    right = 0;
    bottom = 0;
  }

  SetLocationAndSizeFromEdges(left, top, right, bottom);
}

bool FloatRect::Intersects(const FloatRect& other) const {
  return x() < other.MaxX() && MaxX() > other.x() && y() < other.MaxY() &&
         MaxY() > other.y();
}

void FloatRect::Extend(const FloatPoint& p) {
  float min_x = std::min(x(), p.x());
  float min_y = std::min(y(), p.y());
  float max_x = std::max(this->MaxX(), p.x());
  float max_y = std::max(this->MaxY(), p.y());

  SetLocationAndSizeFromEdges(min_x, min_y, max_x, max_y);
}

void FloatRect::ExpandToInclude(const FloatRect& rect, bool exclude_empty) {
  if (exclude_empty) {
    if (IsEmpty()) {
      *this = rect;
      return;
    } else if (rect.IsEmpty()) {
      return;
    }
  }

  float min_x = std::min(x(), rect.x());
  float min_y = std::min(y(), rect.y());
  float max_x = std::max(this->MaxX(), rect.MaxX());
  float max_y = std::max(this->MaxY(), rect.MaxY());

  SetLocationAndSizeFromEdges(min_x, min_y, max_x, max_y);
}

std::string FloatRect::ToString() const {
  std::stringstream ss;
  ss << location_.x() << "," << location_.y() << " " << size_.width() << "*"
     << size_.height();
  return ss.str();
}
}  // namespace clay
