// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_PROPERTIES_BORDER_H_
#define CORE_STYLE_PROPERTIES_BORDER_H_
#include "core/style/color.h"

namespace lynx::style {

enum class BorderSide : uint8_t {
  kTop = 0,
  kRight = 1,
  kBottom = 2,
  kLeft = 3,
};

struct Border {
  uint32_t border_top_color = starlight::DefaultColor::DEFAULT_BORDER_COLOR;
  uint32_t border_right_color = starlight::DefaultColor::DEFAULT_BORDER_COLOR;
  uint32_t border_bottom_color = starlight::DefaultColor::DEFAULT_BORDER_COLOR;
  uint32_t border_left_color = starlight::DefaultColor::DEFAULT_BORDER_COLOR;
};

static constexpr uint32_t Border::*kBorderColorMembers[] = {
    &Border::border_top_color, &Border::border_right_color,
    &Border::border_bottom_color, &Border::border_left_color};

}  // namespace lynx::style
#endif  // CORE_STYLE_PROPERTIES_BORDER_H_
