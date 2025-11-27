// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/geometry/rect.h"

#ifndef CORE_RENDERER_DOM_FRAGMENT_ROUNDED_RECTANGLE_H_
#define CORE_RENDERER_DOM_FRAGMENT_ROUNDED_RECTANGLE_H_

namespace lynx {
namespace tasm {

class RoundedRectangle {
 public:
  RoundedRectangle() = default;
  ~RoundedRectangle() = default;

  const base::geometry::FloatRect& GetRect() const { return rect_; }
  void SetRect(const base::geometry::FloatRect& rect) { rect_ = rect; }

  float GetX() const { return rect_.X(); }
  void SetX(float left) { rect_.SetX(left); }

  float GetY() const { return rect_.Y(); }
  void SetY(float top) { rect_.SetY(top); }

  float GetWidth() const { return rect_.Width(); }
  void SetWidth(float width) { rect_.SetWidth(width); }

  float GetHeight() const { return rect_.Height(); }
  void SetHeight(float height) { rect_.SetHeight(height); }

  float GetRadiusXTopLeft() const { return radius_x_top_left_; }
  void SetRadiusXTopLeft(float r) { radius_x_top_left_ = r; }
  float GetRadiusXTopRight() const { return radius_x_top_right_; }
  void SetRadiusXTopRight(float r) { radius_x_top_right_ = r; }
  float GetRadiusXBottomRight() const { return radius_x_bottom_right_; }
  void SetRadiusXBottomRight(float r) { radius_x_bottom_right_ = r; }
  float GetRadiusXBottomLeft() const { return radius_x_bottom_left_; }
  void SetRadiusXBottomLeft(float r) { radius_x_bottom_left_ = r; }

  float GetRadiusYTopLeft() const { return radius_y_top_left_; }
  void SetRadiusYTopLeft(float r) { radius_y_top_left_ = r; }
  float GetRadiusYTopRight() const { return radius_y_top_right_; }
  void SetRadiusYTopRight(float r) { radius_y_top_right_ = r; }
  float GetRadiusYBottomRight() const { return radius_y_bottom_right_; }
  void SetRadiusYBottomRight(float r) { radius_y_bottom_right_ = r; }
  float GetRadiusYBottomLeft() const { return radius_y_bottom_left_; }
  void SetRadiusYBottomLeft(float r) { radius_y_bottom_left_ = r; }

  bool HasRadius() const {
    return radius_x_top_left_ > 0 || radius_x_top_right_ > 0 ||
           radius_x_bottom_right_ > 0 || radius_x_bottom_left_ > 0 ||
           radius_y_top_left_ > 0 || radius_y_top_right_ > 0 ||
           radius_y_bottom_right_ > 0 || radius_y_bottom_left_ > 0;
  }

 private:
  base::geometry::FloatRect rect_;

  float radius_x_top_left_{0.f};
  float radius_x_top_right_{0.f};
  float radius_x_bottom_right_{0.f};
  float radius_x_bottom_left_{0.f};
  float radius_y_top_left_{0.f};
  float radius_y_top_right_{0.f};
  float radius_y_bottom_right_{0.f};
  float radius_y_bottom_left_{0.f};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_ROUNDED_RECTANGLE_H_
