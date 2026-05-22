/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright 2021 The Lynx Authors. All rights reserved.
 * Licensed under the Apache License Version 2.0 that can be found in the
 * LICENSE file in the root directory of this source tree.
 */

#ifndef CLAY_GFX_GEOMETRY_FLOAT_ROUNDED_RECT_H_
#define CLAY_GFX_GEOMETRY_FLOAT_ROUNDED_RECT_H_

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "skity/geometry/rrect.hpp"

namespace clay {

class FloatRoundedRect {
 public:
  class Radii {
   public:
    Radii() {}
    Radii(const FloatSize& top_left, const FloatSize& top_right,
          const FloatSize& bottom_left, const FloatSize& bottom_right)
        : top_left_(top_left),
          top_right_(top_right),
          bottom_left_(bottom_left),
          bottom_right_(bottom_right) {}

    void SetTopLeft(const FloatSize& size) { top_left_ = size; }
    void SetTopRight(const FloatSize& size) { top_right_ = size; }
    void SetBottomLeft(const FloatSize& size) { bottom_left_ = size; }
    void SetBottomRight(const FloatSize& size) { bottom_right_ = size; }
    const FloatSize& TopLeft() const { return top_left_; }
    const FloatSize& TopRight() const { return top_right_; }
    const FloatSize& BottomLeft() const { return bottom_left_; }
    const FloatSize& BottomRight() const { return bottom_right_; }

    bool IsZero() const;

    void Expand(float top_width, float bottom_width, float left_width,
                float right_width);
    void Expand(float size) { Expand(size, size, size, size); }
    void Shrink(float top_width, float bottom_width, float left_width,
                float right_width);
    void Shrink(float size) { Shrink(size, size, size, size); }
    void Scale(float factor);

   private:
    FloatSize top_left_;
    FloatSize top_right_;
    FloatSize bottom_left_;
    FloatSize bottom_right_;
  };

  FloatRoundedRect() {}
  explicit FloatRoundedRect(const FloatRect&, const Radii& = Radii());
  FloatRoundedRect(float x, float y, float width, float height);
  FloatRoundedRect(const FloatRect&, const FloatSize& top_left,
                   const FloatSize& top_right, const FloatSize& bottom_left,
                   const FloatSize& bottom_right);

  const FloatRect& rect() const { return rect_; }
  const Radii& radii() const { return radii_; }
  bool IsRounded() const { return !radii_.IsZero(); }
  bool IsEmpty() const { return rect_.IsEmpty(); }

  void SetRect(const FloatRect& rect) { rect_ = rect; }
  void SetRadii(const Radii& radii) { radii_ = radii; }
  void SetOval(const FloatRect& rect);

  void Move(const FloatSize& size) { rect_.Move(size); }
  void ExpandRadii(float size) { radii_.Expand(size); }
  void ShrinkRadii(float size) { radii_.Shrink(size); }

  FloatRect TopLeftCorner() const {
    return FloatRect(rect_.x(), rect_.y(), radii_.TopLeft().width(),
                     radii_.TopLeft().height());
  }
  FloatRect TopRightCorner() const {
    return FloatRect(rect_.MaxX() - radii_.TopRight().width(), rect_.y(),
                     radii_.TopRight().width(), radii_.TopRight().height());
  }
  FloatRect BottomLeftCorner() const {
    return FloatRect(rect_.x(), rect_.MaxY() - radii_.BottomLeft().height(),
                     radii_.BottomLeft().width(), radii_.BottomLeft().height());
  }
  FloatRect BottomRightCorner() const {
    return FloatRect(rect_.MaxX() - radii_.BottomRight().width(),
                     rect_.MaxY() - radii_.BottomRight().height(),
                     radii_.BottomRight().width(),
                     radii_.BottomRight().height());
  }

  void ConstraintRadii();
  bool IsRenderable() const;
  operator skity::RRect() const;

 private:
  FloatRect rect_;
  Radii radii_;
};

inline FloatRoundedRect::operator skity::RRect() const {
  skity::RRect rrect;

  if (IsRounded()) {
    skity::Vec2 radii[4];
    radii[skity::RRect::Corner::kUpperLeft] = {TopLeftCorner().width(),
                                               TopLeftCorner().height()};
    radii[skity::RRect::Corner::kUpperRight] = {TopRightCorner().width(),
                                                TopRightCorner().height()};
    radii[skity::RRect::Corner::kLowerRight] = {BottomRightCorner().width(),
                                                BottomRightCorner().height()};
    radii[skity::RRect::Corner::kLowerLeft] = {BottomLeftCorner().width(),
                                               BottomLeftCorner().height()};

    rrect.SetRectRadii(rect(), radii);
  } else {
    rrect.SetRect(rect());
  }

  return rrect;
}

inline bool operator==(const FloatRoundedRect::Radii& a,
                       const FloatRoundedRect::Radii& b) {
  return a.TopLeft() == b.TopLeft() && a.TopRight() == b.TopRight() &&
         a.BottomLeft() == b.BottomLeft() && a.BottomRight() == b.BottomRight();
}

inline bool operator==(const FloatRoundedRect& a, const FloatRoundedRect& b) {
  return a.rect() == b.rect() && a.radii() == b.radii();
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_FLOAT_ROUNDED_RECT_H_
