// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <skity/geometry/rect.hpp>

#include "src/geometry/math.hpp"

namespace skity {
namespace clay {

bool Rect::SetBoundsCheck(const Point* pts, int count) {
  if (count <= 0) {
    SetEmpty();
    return true;
  }

  Vec2 min, max;
  if (count & 1) {
    min = max = {pts->x, pts->y};
    pts += 1;
    count -= 1;
  } else {
    min = Vec2{Vec4::Min(pts[0], pts[1])};
    max = Vec2{Vec4::Max(pts[0], pts[1])};
    pts += 2;
    count -= 2;
  }

  Vec2 accum = min * 0.f;
  while (count) {
    Vec2 x = Vec2{pts[0]};
    Vec2 y = Vec2{pts[1]};
    accum *= x;
    accum *= y;
    min = Vec2::Min(min, Vec2::Min(x, y));
    max = Vec2::Max(max, Vec2::Max(x, y));
    pts += 2;
    count -= 2;
  }

  accum *= 0.f;
  bool all_finite = !glm::isinf(accum.x) && !glm::isinf(accum.y);
  if (all_finite) {
    this->SetLTRB(min.x, min.y, max.x, max.y);
  } else {
    this->SetEmpty();
  }
  return all_finite;
}

float Rect::CenterX() const { return FloatHalf * (right_ + left_); }

float Rect::CenterY() const { return FloatHalf * (top_ + bottom_); }

void Rect::ToQuad(Point quad[4]) const {
  quad[0] = {left_, top_, 0, 0};
  quad[1] = {right_, top_, 0, 0};
  quad[2] = {right_, bottom_, 0, 0};
  quad[3] = {left_, bottom_, 0, 0};
}

void Rect::SetX(float x) {
  auto width = Width();
  left_ = x;
  right_ = x + width;
}

void Rect::SetY(float y) {
  auto height = Height();
  top_ = y;
  bottom_ = y + height;
}

void Rect::SetLeft(float left) { left_ = left; }

void Rect::SetTop(float top) { top_ = top; }

void Rect::SetRight(float right) { right_ = right; }

void Rect::SetBottom(float bottom) { bottom_ = bottom; }

void Rect::SetWH(float width, float height) {
  this->SetXYWH(0, 0, width, height);
}

void Rect::Offset(float dx, float dy) {
  left_ += dx;
  top_ += dy;
  right_ += dx;
  bottom_ += dy;
}

void Rect::Inset(float inset) { Inset(inset, inset); }

void Rect::Inset(float dx, float dy) {
  left_ += dx;
  top_ += dy;
  right_ -= dx;
  bottom_ -= dy;
}

void Rect::Outset(float outset) { Outset(outset, outset); }

void Rect::Outset(float dx, float dy) { Inset(-dx, -dy); }

void Rect::RoundOut() {
  left_ = floorf(left_);
  top_ = floorf(top_);
  right_ = ceilf(right_);
  bottom_ = ceilf(bottom_);
}

void Rect::RoundIn() {
  left_ = ceilf(left_);
  top_ = ceilf(top_);
  right_ = floorf(right_);
  bottom_ = floorf(bottom_);
}

void Rect::Round() {
  left_ = roundf(left_);
  top_ = roundf(top_);
  right_ = roundf(right_);
  bottom_ = roundf(bottom_);
}

Rect Rect::MakeSorted() const {
  return MakeLTRB(std::min(left_, right_), std::min(top_, bottom_),
                  std::max(left_, right_), std::max(top_, bottom_));
}

Rect Rect::MakeOffset(float dx, float dy) const {
  return MakeLTRB(left_ + dx, top_ + dy, right_ + dx, bottom_ + dy);
}

Rect Rect::MakeInset(float dx, float dy) const {
  return MakeLTRB(left_ + dx, top_ + dy, right_ - dx, bottom_ - dy);
}

Rect Rect::MakeOutset(float dx, float dy) const {
  return MakeLTRB(left_ - dx, top_ - dy, right_ + dx, bottom_ + dy);
}

void Rect::Join(const Rect& r) {
  if (r.IsEmpty()) {
    return;
  }

  if (this->IsEmpty()) {
    *this = r;
  } else {
    left_ = std::min(left_, r.left_);
    top_ = std::min(top_, r.top_);
    right_ = std::max(right_, r.right_);
    bottom_ = std::max(bottom_, r.bottom_);
  }
}

bool Rect::Intersect(Rect const& rect) {
  float l = std::max(left_, rect.left_);
  float r = std::min(right_, rect.right_);
  float t = std::max(top_, rect.top_);
  float b = std::min(bottom_, rect.bottom_);

  if (!(l < r && t < b)) {
    return false;
  }

  SetLTRB(l, t, r, b);
  return true;
}

bool Rect::IsFinite() const {
  float accum = 0;
  accum *= left_;
  accum *= top_;
  accum *= right_;
  accum *= bottom_;

  return !FloatIsNan(accum);
}

float Rect::HalfWidth(Rect const& rect) {
  return rect.right_ * FloatHalf - rect.left_ * FloatHalf;
}

float Rect::HalfHeight(Rect const& rect) {
  return rect.bottom_ * FloatHalf - rect.top_ * FloatHalf;
}

bool Rect::Intersect(const Rect& a, const Rect& b) {
  float left = std::max(a.Left(), b.Left());
  float top = std::max(a.Top(), b.Top());
  float right = std::min(a.Right(), b.Right());
  float bottom = std::min(a.Bottom(), b.Bottom());

  auto tmp = Rect::MakeLTRB(left, top, right, bottom);

  return !tmp.IsEmpty();
}

}  // namespace clay
}  // namespace skity
