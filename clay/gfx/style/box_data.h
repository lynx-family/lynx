// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_BOX_DATA_H_
#define CLAY_GFX_STYLE_BOX_DATA_H_

namespace clay {

class BoxData {
 public:
  BoxData() = default;

  float Left() const { return left_; }
  float Top() const { return top_; }
  float Width() const { return width_; }
  float Height() const { return height_; }
  void SetLeft(float left) { left_ = left; }
  void SetWidth(float width) { width_ = width; }
  void SetTop(float top) { top_ = top; }
  void SetHeight(float height) { height_ = height; }

  void SetPaddingLeft(float left) { padding_left_ = left; }
  void SetPaddingTop(float top) { padding_top_ = top; }
  void SetPaddingRight(float right) { padding_right_ = right; }
  void SetPaddingBottom(float bottom) { padding_bottom_ = bottom; }
  float PaddingLeft() const { return padding_left_; }
  float PaddingTop() const { return padding_top_; }
  float PaddingRight() const { return padding_right_; }
  float PaddingBottom() const { return padding_bottom_; }

  void SetMarginLeft(float left) { margin_left_ = left; }
  void SetMarginTop(float top) { margin_top_ = top; }
  void SetMarginRight(float right) { margin_right_ = right; }
  void SetMarginBottom(float bottom) { margin_bottom_ = bottom; }
  float MarginLeft() const { return margin_left_; }
  float MarginTop() const { return margin_top_; }
  float MarginRight() const { return margin_right_; }
  float MarginBottom() const { return margin_bottom_; }

  bool operator==(const BoxData& rhs) const {
    return left_ == rhs.left_ && width_ == rhs.width_ && top_ == rhs.top_ &&
           height_ == rhs.height_ && padding_top_ == rhs.padding_top_ &&
           padding_left_ == rhs.padding_left_ &&
           padding_right_ == rhs.padding_right_ &&
           padding_bottom_ == rhs.padding_bottom_ &&
           margin_top_ == rhs.margin_top_ && margin_left_ == rhs.margin_left_ &&
           margin_right_ == rhs.margin_right_ &&
           margin_bottom_ == rhs.margin_bottom_;
  }
  bool operator!=(const BoxData& rhs) const { return !(*this == rhs); }

 private:
  float left_{0.f};
  float top_{0.f};
  float width_{0.f};
  float height_{0.f};
  float padding_top_{0.f};
  float padding_left_{0.f};
  float padding_right_{0.f};
  float padding_bottom_{0.f};
  float margin_top_{0.f};
  float margin_left_{0.f};
  float margin_right_{0.f};
  float margin_bottom_{0.f};
};
}  // namespace clay

#endif  // CLAY_GFX_STYLE_BOX_DATA_H_
