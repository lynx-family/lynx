// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/style/borders_data.h"

#include <sstream>

#include "clay/gfx/style/color.h"

namespace clay {

BordersData::BordersData()
    : width_top_(0.0f),
      width_right_(0.0f),
      width_bottom_(0.0f),
      width_left_(0.0f),
      radius_x_top_left_(0.0f),
      radius_x_top_right_(0.0f),
      radius_x_bottom_right_(0.0f),
      radius_x_bottom_left_(0.0f),
      radius_y_top_left_(0.0f),
      radius_y_top_right_(0.0f),
      radius_y_bottom_right_(0.0f),
      radius_y_bottom_left_(0.0f),

      color_top_(Color::kBlack().Value()),
      color_right_(Color::kBlack().Value()),
      color_bottom_(Color::kBlack().Value()),
      color_left_(Color::kBlack().Value()),
      style_top_(BorderStyleType::kSolid),
      style_right_(BorderStyleType::kSolid),
      style_bottom_(BorderStyleType::kSolid),
      style_left_(BorderStyleType::kSolid) {}

void BordersData::Reset() {
  width_top_ = 0.0f;
  width_right_ = 0.0f;
  width_bottom_ = 0.0f;
  width_left_ = 0.0f;
  radius_x_top_left_ = 0.0f;
  radius_x_top_right_ = 0.0f;
  radius_x_bottom_right_ = 0.0f;
  radius_x_bottom_left_ = 0.0f;
  radius_y_top_left_ = 0.0f;
  radius_y_top_right_ = 0.0f;
  radius_y_bottom_right_ = 0.0f;
  radius_y_bottom_left_ = 0.0f;
  color_top_ = Color::kBlack().Value();
  color_right_ = Color::kBlack().Value();
  color_bottom_ = Color::kBlack().Value();
  color_left_ = Color::kBlack().Value();
  style_top_ = BorderStyleType::kSolid;
  style_right_ = BorderStyleType::kSolid;
  style_bottom_ = BorderStyleType::kSolid;
  style_left_ = BorderStyleType::kSolid;
}

void BordersData::SetRadius(size_t index, const Length& x, const Length& y) {
  switch (index) {
    case 0:
      radius_x_top_left_length_ = x;
      radius_y_top_left_length_ = y;
      break;
    case 1:
      radius_x_top_right_length_ = x;
      radius_y_top_right_length_ = y;
      break;
    case 2:
      radius_x_bottom_right_length_ = x;
      radius_y_bottom_right_length_ = y;
      break;
    case 3:
      radius_x_bottom_left_length_ = x;
      radius_y_bottom_left_length_ = y;
      break;
    default:
      break;
  }
}

static inline float update_value(const Length& length, float v) {
  return length.IsPercent() ? length.GetRawValue() * v : length.GetRawValue();
}

void BordersData::UpdateRadius(float width, float height) {
  radius_y_top_left_ = update_value(radius_y_top_left_length_, height);
  radius_x_top_left_ = update_value(radius_x_top_left_length_, width);
  radius_y_top_right_ = update_value(radius_y_top_right_length_, height);
  radius_x_top_right_ = update_value(radius_x_top_right_length_, width);
  radius_y_bottom_right_ = update_value(radius_y_bottom_right_length_, height);
  radius_x_bottom_right_ = update_value(radius_x_bottom_right_length_, width);
  radius_y_bottom_left_ = update_value(radius_y_bottom_left_length_, height);
  radius_x_bottom_left_ = update_value(radius_x_bottom_left_length_, width);
}

#ifndef NDEBUG
std::string BordersData::ToString() const {
  std::stringstream ss;

  if (HasBorderRadius()) {
    if (HasSimpleRadii() && HasSameRadii()) {
      ss << "border-radius=(" << radius_x_top_left_ << ")";
    } else if (HasSimpleRadii()) {
      ss << "border-radius=(" << radius_x_top_left_ << " "
         << radius_x_top_right_ << " " << radius_x_bottom_right_ << " "
         << radius_x_bottom_left_ << ")";
    } else {
      ss << "border-radius=(" << radius_x_top_left_ << " "
         << radius_x_top_right_ << " " << radius_x_bottom_right_ << " "
         << radius_x_bottom_left_ << " / " << radius_y_top_left_ << " "
         << radius_y_top_right_ << " " << radius_y_bottom_right_ << " "
         << radius_y_bottom_left_ << ")";
    }
  }

  if (HasBorderWidth()) {
    if (HasBorderRadius()) {
      ss << " ";
    }
    if (HasSameWidth() && HasSameStyle() && HasSameColor()) {
      ss << "border=(" << width_top_ << " " << static_cast<unsigned>(style_top_)
         << " " << Color(color_top_).ToString() << ")";
    } else {
      if (HasSameWidth()) {
        ss << "border-width=(" << width_top_ << ")";
      } else {
        ss << "border-width=(" << width_top_ << " " << width_right_ << " "
           << width_bottom_ << " " << width_left_ << ")";
      }
      if (HasSameStyle()) {
        ss << " border-style=(" << static_cast<unsigned>(style_top_) << ")";
      } else {
        ss << " border-style=(" << static_cast<unsigned>(style_top_) << " "
           << static_cast<unsigned>(style_right_) << " "
           << static_cast<unsigned>(style_bottom_) << " "
           << static_cast<unsigned>(style_left_) << ")";
      }
      if (HasSameColor()) {
        ss << " border-color=(" << Color(color_top_).ToString() << ")";
      } else {
        ss << " border-color=(" << Color(color_top_).ToString() << " "
           << Color(color_right_).ToString() << " "
           << Color(color_bottom_).ToString() << " "
           << Color(color_left_).ToString() << ")";
      }
    }
  }
  return ss.str();
}
#endif

}  // namespace clay
