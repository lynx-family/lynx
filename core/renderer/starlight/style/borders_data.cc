// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/style/borders_data.h"

#include "core/renderer/starlight/style/default_layout_style.h"
#include "core/style/color.h"

namespace lynx {
namespace starlight {

BordersData::BordersData(bool css_align_with_legacy_w3c)
    : width_top(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER)),
      width_right(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER)),
      width_bottom(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER)),
      width_left(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER)),
      radius_x_top_left(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_x_top_right(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_x_bottom_right(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_x_bottom_left(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_y_top_left(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_y_top_right(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_y_bottom_right(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      radius_y_bottom_left(DefaultLayoutStyle::SL_DEFAULT_RADIUS()),
      color_top(DefaultColor::DEFAULT_BORDER_COLOR),
      color_right(DefaultColor::DEFAULT_BORDER_COLOR),
      color_bottom(DefaultColor::DEFAULT_BORDER_COLOR),
      color_left(DefaultColor::DEFAULT_BORDER_COLOR),
      style_top(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER_STYLE)),
      style_right(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER_STYLE)),
      style_bottom(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER_STYLE)),
      style_left(DEFAULT_CSS_VALUE(css_align_with_legacy_w3c, BORDER_STYLE)),
      css_align_with_legacy_w3c_(css_align_with_legacy_w3c) {}

void BordersData::Reset() {
  width_top = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER);
  width_right = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER);
  width_bottom = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER);
  width_left = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER);
  radius_x_top_left = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_x_top_right = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_x_bottom_right = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_x_bottom_left = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_y_top_left = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_y_top_right = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_y_bottom_right = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  radius_y_bottom_left = DefaultLayoutStyle::SL_DEFAULT_RADIUS();
  color_top = DefaultColor::DEFAULT_COLOR;
  color_right = DefaultColor::DEFAULT_COLOR;
  color_bottom = DefaultColor::DEFAULT_COLOR;
  color_left = DefaultColor::DEFAULT_COLOR;
  style_top = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE);
  style_right = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE);
  style_bottom = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE);
  style_left = DEFAULT_CSS_VALUE(css_align_with_legacy_w3c_, BORDER_STYLE);
}
}  // namespace starlight
}  // namespace lynx
