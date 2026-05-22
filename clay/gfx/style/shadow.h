// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_STYLE_SHADOW_H_
#define CLAY_GFX_STYLE_SHADOW_H_

#include "clay/gfx/style/color.h"

namespace clay {

struct Shadow {
  // Note(Xietong): not supported by skia.
  bool inset;
  float offset_x;
  float offset_y;
  float blur_radius;
  // Note(Xietong): not supported by skia.
  float spread_radius;
  Color color;
};

inline bool operator==(const Shadow& a, const Shadow& b) {
  return a.inset == b.inset && a.color == b.color && a.offset_x == b.offset_x &&
         a.offset_y == b.offset_y && a.blur_radius == b.blur_radius &&
         a.spread_radius == b.spread_radius;
}

inline bool operator!=(const Shadow& a, const Shadow& b) { return !(a == b); }

}  // namespace clay

#endif  // CLAY_GFX_STYLE_SHADOW_H_
