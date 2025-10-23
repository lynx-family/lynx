/*
 * Copyright (C) 2003, 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright 2021 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CLAY_GFX_GEOMETRY_FLOAT_RECT_H_
#define CLAY_GFX_GEOMETRY_FLOAT_RECT_H_

#include <algorithm>
#include <string>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_size.h"
#include "skity/geometry/rect.hpp"

namespace clay {

class Rect;

class FloatRect {
 public:
  FloatRect() = default;
  FloatRect(const FloatPoint& location, const FloatSize& size)
      : location_(location), size_(size) {}
  FloatRect(float x, float y, float width, float height)
      : location_(FloatPoint(x, y)), size_(FloatSize(width, height)) {}
  explicit FloatRect(const Rect&);
  explicit FloatRect(const skity::Rect&);

  FloatPoint location() const { return location_; }
  FloatSize size() const { return size_; }

  void SetLocation(const FloatPoint& location) { location_ = location; }
  void SetSize(const FloatSize& size) { size_ = size; }

  float x() const { return location_.x(); }
  float y() const { return location_.y(); }
  float MaxX() const { return x() + width(); }
  float MaxY() const { return y() + height(); }
  float width() const { return size_.width(); }
  float height() const { return size_.height(); }

  float left() const { return location_.x(); }
  float top() const { return location_.y(); }
  float right() const { return location_.x() + size_.width(); }
  float bottom() const { return location_.y() + size_.height(); }
  FloatPoint origin() const { return location_; }

  void SetX(float x) { location_.SetX(x); }
  void SetY(float y) { location_.SetY(y); }
  void SetWidth(float width) { size_.SetWidth(width); }
  void SetHeight(float height) { size_.SetHeight(height); }

  bool IsEmpty() const { return size_.IsEmpty(); }

  FloatPoint Center() const {
    return FloatPoint(x() + width() / 2, y() + height() / 2);
  }

  void Move(const FloatSize& delta) { location_ += delta; }
  void MoveBy(const FloatPoint& delta) { location_.Move(delta.x(), delta.y()); }
  void Move(float dx, float dy) { location_.Move(dx, dy); }

  void Expand(float top, float right, float bottom, float left) {
    location_.Move(-left, -top);
    size_.Expand(left + right, top + bottom);
  }
  void Expand(const FloatSize& size) { size_ += size; }
  void Expand(float dw, float dh) { size_.Expand(dw, dh); }
  void Contract(const FloatSize& size) { size_ -= size; }
  void Contract(float dw, float dh) { size_.Expand(-dw, -dh); }

  void ShiftXEdgeTo(float edge) {
    float delta = edge - x();
    SetX(edge);
    SetWidth(std::max(0.0f, width() - delta));
  }
  void ShiftMaxXEdgeTo(float edge) {
    float delta = edge - MaxX();
    SetWidth(std::max(0.0f, width() + delta));
  }
  void ShiftYEdgeTo(float edge) {
    float delta = edge - y();
    SetY(edge);
    SetHeight(std::max(0.0f, height() - delta));
  }
  void ShiftMaxYEdgeTo(float edge) {
    float delta = edge - MaxY();
    SetHeight(std::max(0.0f, height() + delta));
  }

  void InflateX(float dx) {
    location_.SetX(location_.x() - dx);
    size_.SetWidth(size_.width() + dx + dx);
  }
  void InflateY(float dy) {
    location_.SetY(location_.y() - dy);
    size_.SetHeight(size_.height() + dy + dy);
  }

  void Inflate(float d) {
    InflateX(d);
    InflateY(d);
  }

  FloatPoint MinXMinYCorner() const { return location_; }  // typically topLeft
  FloatPoint MaxXMinYCorner() const {
    return FloatPoint(location_.x() + size_.width(), location_.y());
  }  // typically topRight
  FloatPoint MinXMaxYCorner() const {
    return FloatPoint(location_.x(), location_.y() + size_.height());
  }  // typically bottomLeft
  FloatPoint MaxXMaxYCorner() const {
    return FloatPoint(location_.x() + size_.width(),
                      location_.y() + size_.height());
  }  // typically bottomRight

  bool Contains(const FloatRect&) const;

  void Intersect(const FloatRect&);
  bool Intersects(const FloatRect&) const;
  void Extend(const FloatPoint&);
  void ExpandToInclude(const FloatRect&, bool exclude_empty = false);

  bool Contains(const FloatPoint& point) const {
    return Contains(point.x(), point.y());
  }

  bool Contains(float px, float py) const {
    return px >= x() && px <= MaxX() && py >= y() && py <= MaxY();
  }
  std::string ToString() const;

  operator skity::Rect() const {
    return skity::Rect::MakeXYWH(x(), y(), width(), height());
  }

 private:
  void SetLocationAndSizeFromEdges(float left, float top, float right,
                                   float bottom) {
    location_.SetX(left);
    location_.SetY(top);
    size_.SetWidth(right - left);
    size_.SetHeight(bottom - top);
  }

  FloatPoint location_;
  FloatSize size_;
};

inline FloatRect& operator+=(FloatRect& a, const FloatRect& b) {
  a.Move(b.x(), b.y());
  a.SetWidth(a.width() + b.width());
  a.SetHeight(a.height() + b.height());
  return a;
}

inline FloatRect operator+(const FloatRect& a, const FloatRect& b) {
  FloatRect c = a;
  c += b;
  return c;
}

inline FloatRect operator*(const FloatRect& a, float factor) {
  return FloatRect(a.x() * factor, a.y() * factor, a.width() * factor,
                   a.height() * factor);
}

inline bool operator==(const FloatRect& a, const FloatRect& b) {
  return a.location() == b.location() && a.size() == b.size();
}

inline bool operator!=(const FloatRect& a, const FloatRect& b) {
  return a.location() != b.location() || a.size() != b.size();
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_RECT_H_
