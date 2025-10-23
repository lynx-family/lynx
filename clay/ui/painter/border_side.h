// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BORDER_SIDE_H_
#define CLAY_UI_PAINTER_BORDER_SIDE_H_

#include "clay/gfx/style/borders_data.h"
#include "clay/gfx/style/color.h"

namespace clay {

class BorderSide {
 public:
  enum SideIndex {
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3,
  };

  BorderSide(float border_width, const Color& border_color,
             BorderStyleType border_style)
      : width_(border_width),
        color_(border_color),
        side_index_(TOP),
        style_(border_style) {
    if (style_ == BorderStyleType::kDouble && border_width < 3)
      style_ = BorderStyleType::kSolid;
  }

  BorderSide(float border_width, const Color& border_color, SideIndex index,
             BorderStyleType border_style)
      : width_(border_width),
        color_(border_color),
        side_index_(index),
        style_(border_style) {
    if (style_ == BorderStyleType::kDouble && border_width < 3)
      style_ = BorderStyleType::kSolid;
  }

  BorderSide()
      : width_(0),
        color_(0),
        side_index_(TOP),
        style_(BorderStyleType::kHide) {}

  bool Visible() const { return color_.Alpha(); }
  void GetDoubleBorderStripeWidths(float& outer_width,
                                   float& inner_width) const {
    outer_width = width_ / 3;
    inner_width = width_ * 2 / 3;
  }

  float InnerWidth() const { return width_ * 2 / 3; }

  float OuterWidth() const { return width_ / 3; }

  bool ObscuresBackgroundEdge() const {
    if (color_.Alpha() < 255 || style_ == BorderStyleType::kHide ||
        style_ == BorderStyleType::kNone)
      return false;

    if (style_ == BorderStyleType::kDotted ||
        style_ == BorderStyleType::kDashed)
      return false;

    return true;
  }

  float width_;
  Color color_;
  SideIndex side_index_;
  BorderStyleType style_;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BORDER_SIDE_H_
