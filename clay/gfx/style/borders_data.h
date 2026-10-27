// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_BORDERS_DATA_H_
#define CLAY_GFX_STYLE_BORDERS_DATA_H_

#include <limits.h>

#include <array>
#include <string>
#include <tuple>

#include "clay/gfx/style/length.h"

namespace clay {

enum class BorderStyleType : unsigned {
  kSolid,
  kDashed,
  kDotted,
  kDouble,
  kGroove,
  kRidge,
  kInset,
  kOutset,
  kHide,
  kNone,
  kUndefined = INT_MAX,  // default no border style
};

class BordersData {
 public:
  BordersData();
  void Reset();

  float width_top_;
  float width_right_;
  float width_bottom_;
  float width_left_;
  float radius_x_top_left_;
  float radius_x_top_right_;
  float radius_x_bottom_right_;
  float radius_x_bottom_left_;
  float radius_y_top_left_;
  float radius_y_top_right_;
  float radius_y_bottom_right_;
  float radius_y_bottom_left_;
  Length radius_x_top_left_length_;
  Length radius_x_top_right_length_;
  Length radius_x_bottom_right_length_;
  Length radius_x_bottom_left_length_;
  Length radius_y_top_left_length_;
  Length radius_y_top_right_length_;
  Length radius_y_bottom_right_length_;
  Length radius_y_bottom_left_length_;
  unsigned int color_top_;
  unsigned int color_right_;
  unsigned int color_bottom_;
  unsigned int color_left_;
  BorderStyleType style_top_;
  BorderStyleType style_right_;
  BorderStyleType style_bottom_;
  BorderStyleType style_left_;

  void SetRadius(size_t side, const Length& x, const Length& y);
  void UpdateRadius(float width, float height);
  bool HasBorderRadius() const {
    return (radius_x_top_left_ != 0.0 && radius_y_top_left_ != 0.0) ||
           (radius_x_top_right_ != 0.0 && radius_y_top_right_ != 0.0) ||
           (radius_x_bottom_right_ != 0.0 && radius_y_bottom_right_ != 0.0) ||
           (radius_x_bottom_left_ != 0.0 && radius_y_bottom_left_ != 0.0);
  }

  bool HasBorderWidth() const {
    return width_left_ != 0.0 || width_right_ != 0.0 || width_top_ != 0.0 ||
           width_bottom_ != 0.0;
  }

  bool HasSameWidth() const {
    return width_top_ == width_left_ && width_top_ == width_right_ &&
           width_top_ == width_bottom_;
  }

  bool HasSimpleRadii() const {
    return radius_x_top_left_ == radius_y_top_left_ &&
           radius_x_top_right_ == radius_y_top_right_ &&
           radius_x_bottom_left_ == radius_y_bottom_left_ &&
           radius_x_bottom_right_ == radius_y_bottom_right_;
  }

  bool HasSameRadii() const {
    return HasSimpleRadii() && radius_x_top_left_ == radius_x_top_right_ &&
           radius_x_top_left_ == radius_x_bottom_right_ &&
           radius_x_top_left_ == radius_x_bottom_left_;
  }

  bool HasSameColor() const {
    return color_top_ == color_left_ && color_top_ == color_bottom_ &&
           color_top_ == color_right_;
  }

  bool HasSameStyle() const {
    return style_top_ == style_left_ && style_top_ == style_bottom_ &&
           style_top_ == style_right_;
  }

  bool operator==(const BordersData& rhs) const {
    return std::tie(width_top_, width_right_, width_bottom_, width_left_,
                    radius_x_top_left_, radius_x_top_right_,
                    radius_x_bottom_right_, radius_x_bottom_left_,
                    radius_y_top_left_, radius_y_top_right_,
                    radius_y_bottom_right_, radius_y_bottom_left_, color_top_,
                    color_right_, color_bottom_, color_left_, style_top_,
                    style_right_, style_bottom_, style_left_) ==
           std::tie(rhs.width_top_, rhs.width_right_, rhs.width_bottom_,
                    rhs.width_left_, rhs.radius_x_top_left_,
                    rhs.radius_x_top_right_, rhs.radius_x_bottom_right_,
                    rhs.radius_x_bottom_left_, rhs.radius_y_top_left_,
                    rhs.radius_y_top_right_, rhs.radius_y_bottom_right_,
                    rhs.radius_y_bottom_left_, rhs.color_top_, rhs.color_right_,
                    rhs.color_bottom_, rhs.color_left_, rhs.style_top_,
                    rhs.style_right_, rhs.style_bottom_, rhs.style_left_);
  }

#ifndef NDEBUG
  std::string ToString() const;
#endif
};

}  // namespace clay

#endif  // CLAY_GFX_STYLE_BORDERS_DATA_H_
