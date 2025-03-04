// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_STYLE_BORDERS_DATA_H_
#define CORE_RENDERER_STARLIGHT_STYLE_BORDERS_DATA_H_

#include <array>
#include <optional>
#include <tuple>

#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {
class BordersData {
 public:
  explicit BordersData(bool css_align_with_legacy_w3c = false);
  void Reset();

  float width_top;
  float width_right;
  float width_bottom;
  float width_left;
  NLength radius_x_top_left;
  NLength radius_x_top_right;
  NLength radius_x_bottom_right;
  NLength radius_x_bottom_left;
  NLength radius_y_top_left;
  NLength radius_y_top_right;
  NLength radius_y_bottom_right;
  NLength radius_y_bottom_left;
  unsigned int color_top;
  unsigned int color_right;
  unsigned int color_bottom;
  unsigned int color_left;
  BorderStyleType style_top;
  BorderStyleType style_right;
  BorderStyleType style_bottom;
  BorderStyleType style_left;

  bool css_align_with_legacy_w3c_ = false;

  bool operator==(const BordersData& rhs) const {
    return std::tie(width_top, width_right, width_bottom, width_left,
                    radius_x_top_left, radius_x_top_right,
                    radius_x_bottom_right, radius_x_bottom_left,
                    radius_y_top_left, radius_y_top_right,
                    radius_y_bottom_right, radius_y_bottom_left, color_top,
                    color_right, color_bottom, color_left, style_top,
                    style_right, style_bottom, style_left) ==
           std::tie(rhs.width_top, rhs.width_right, rhs.width_bottom,
                    rhs.width_left, rhs.radius_x_top_left,
                    rhs.radius_x_top_right, rhs.radius_x_bottom_right,
                    rhs.radius_x_bottom_left, rhs.radius_y_top_left,
                    rhs.radius_y_top_right, rhs.radius_y_bottom_right,
                    rhs.radius_y_bottom_left, rhs.color_top, rhs.color_right,
                    rhs.color_bottom, rhs.color_left, rhs.style_top,
                    rhs.style_right, rhs.style_bottom, rhs.style_left);
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_STYLE_BORDERS_DATA_H_
