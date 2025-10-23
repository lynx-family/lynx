// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef INCLUDE_SKITY_GEOMETRY_RECT_HPP
#define INCLUDE_SKITY_GEOMETRY_RECT_HPP

#include <skity/geometry/point.hpp>
#include <skity/macros.hpp>

namespace skity {
namespace clay {

class SKITY_API Rect {
 public:
  constexpr Rect() : Rect{0, 0, 0, 0} {}
  constexpr Rect(float left, float top, float right, float bottom)
      : left_{left}, top_{top}, right_{right}, bottom_{bottom} {}
  constexpr Rect(const Rect&) = default;
  constexpr Rect(Rect&&) = default;
  Rect& operator=(const Rect&) = default;

  bool operator==(const Rect& other) const {
    return this->left_ == other.left_ && this->top_ == other.top_ &&
           this->right_ == other.right_ && this->bottom_ == other.bottom_;
  }

  bool operator!=(const Rect& other) const { return !(*this == other); }
  /**
   * Returns left edge of Rect, if sorted.
   * @return left
   */
  constexpr float X() const { return left_; }
  constexpr float Left() const { return left_; }
  /**
   * Returns top edge of Rect.
   * @return top
   */
  constexpr float Y() const { return top_; }
  constexpr float Top() const { return top_; }
  /**
   * Returns right edge of Rect.
   * @return right
   */
  constexpr float Right() const { return right_; }
  /**
   * Returns bottom edge of rect.
   * @return bottom
   */
  constexpr float Bottom() const { return bottom_; }
  /**
   * Returns span on the x-axis. This dose not check if Rect is sorted.
   * Result may be negative or infinity.
   */
  constexpr float Width() const { return right_ - left_; }
  /**
   * Returns span on the y-axis.
   */
  constexpr float Height() const { return bottom_ - top_; }
  float CenterX() const;
  float CenterY() const;

  /**
   * Returns true if left is equal to or greater than right, or if top is equal
   * to or greater than bottom. Call sort() to reverse rectangles with negative
   * width() or height().
   */
  constexpr bool IsEmpty() const { return !(left_ < right_ && top_ < bottom_); }
  void SetEmpty() { *this = MakeEmpty(); }
  /**
   * Returns for points in quad that enclose Rect ordered as: top-left,
   * top-right, bottom-right, bottom-left
   */
  void ToQuad(Point quad[4]) const;
  /**
   * Set Rect to (left, top, right, bottom)
   * @note left,right and top,bottom need to be sorted.
   * @param left    sorted in left
   * @param top     sorted in top
   * @param right   sorted in right
   * @param bottom  sorted in bottom
   */
  void SetLTRB(float left, float top, float right, float bottom) {
    left_ = left;
    top_ = top;
    right_ = right;
    bottom_ = bottom;
  }
  /**
   * Sets to bounds of Point array with count entries.
   * If count is zero or smaller, or if Point array contains an infinity or NaN,
   * sets to (0, 0, 0, 0).
   *
   * Result is either empty or sorted.
   *
   * @param pts     Point array
   * @param count   entries in array
   */
  void SetBounds(const Point pts[], int count) {
    this->SetBoundsCheck(pts, count);
  }
  bool SetBoundsCheck(const Point pts[], int count);

  void Set(const Point& p0, const Point& p1) {
    left_ = std::min(p0.x, p1.x);
    right_ = std::max(p0.x, p1.x);
    top_ = std::min(p0.y, p1.y);
    bottom_ = std::max(p0.y, p1.y);
  }

  /**
   * Sets Rect to (x, y, x + width, y + height)
   * Dose not validate input; with or height may be nagtive.
   *
   * @param x       stored in left
   * @param y       stored in top
   * @param width   added to x and stored in right
   * @param height  added to y and stored in bottom
   */
  void SetXYWH(float x, float y, float width, float height) {
    left_ = x;
    top_ = y;
    right_ = x + width;
    bottom_ = y + height;
  }

  void SetX(float x);

  void SetY(float y);

  void SetLeft(float left);

  void SetTop(float top);

  void SetRight(float right);

  void SetBottom(float bottom);

  void SetWH(float width, float height);

  void Offset(float dx, float dy);

  void Inset(float inset);

  void Inset(float dx, float dy);

  void Outset(float outset);

  void Outset(float dx, float dy);

  void RoundOut();

  void RoundIn();

  void Round();

  bool IsSorted() const { return left_ <= right_ && top_ <= bottom_; }

  bool IsFinite() const;

  void Sort() {
    if (left_ > right_) {
      std::swap(left_, right_);
    }
    if (top_ > bottom_) {
      std::swap(top_, bottom_);
    }
  }

  Rect MakeSorted() const;

  Rect MakeOffset(float dx, float dy) const;

  Rect MakeInset(float dx, float dy) const;

  Rect MakeOutset(float dx, float dy) const;

  /**
   * Sets Rect to the union of itself and r.
   * @param r expansion Rect
   */
  void Join(Rect const& r);

  /**
   * Returns true if Rect intersects r, and sets Rect to intersection.
   * Returns false if Rect does not intersect r, and leaves Rect unchanged.
   * @param r expansion Rect
   */
  bool Intersect(Rect const& r);

  bool Contains(float x, float y) const {
    return x >= left_ && x < right_ && y >= top_ && y < bottom_;
  }

  bool Contains(const Rect& r) const {
    return !r.IsEmpty() && !this->IsEmpty() && left_ <= r.left_ &&
           top_ <= r.top_ && right_ >= r.right_ && bottom_ >= r.bottom_;
  }

  static constexpr Rect MakeEmpty() { return Rect{0, 0, 0, 0}; }

  static constexpr Rect MakeWH(float width, float height) {
    return Rect{0, 0, width, height};
  }

  static constexpr Rect MakeLTRB(float l, float t, float r, float b) {
    return Rect{l, t, r, b};
  }

  static constexpr Rect MakeXYWH(float x, float y, float w, float h) {
    return Rect{x, y, x + w, y + h};
  }

  static constexpr Rect MakeSize(const Vec2& size) {
    return Rect{0, 0, size.x, size.y};
  }

  static float HalfWidth(Rect const& rect);

  static float HalfHeight(Rect const& rect);

  /**
   * @return true   If a intersects b
   * @return false  If a does not intersect b
   */
  static bool Intersect(Rect const& a, Rect const& b);

 private:
  float left_;
  float top_;
  float right_;
  float bottom_;

  friend class RRect;
};

}  // namespace clay
using Rect = clay::Rect;
}  // namespace skity

#endif  // INCLUDE_SKITY_GEOMETRY_RECT_HPP
