// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_VERTICAL_ALIGN_STYLE_H_
#define CLAY_UI_SHADOW_VERTICAL_ALIGN_STYLE_H_

namespace clay {

enum VerticalAlignType {
  kVerticalAlignDefault = 0,
  kVerticalAlignBaseline = 1,
  kVerticalAlignSub = 2,
  kVerticalAlignSuper = 3,
  kVerticalAlignTop = 4,
  kVerticalAlignTextTop = 5,
  kVerticalAlignMiddle = 6,
  kVerticalAlignBottom = 7,
  kVerticalAlignTextBottom = 8,
  kVerticalAlignLength = 9,
  kVerticalAlignPercent = 10,
  kVerticalAlignCenter = 11,
};

struct FontMetrics {
  double ascent = 0.f;
  double descent = 0.f;
  float x_height = 0.f;
  float line_height = 0.f;
  float top = 0.f;
  float bottom = 0.f;
  float glyph_top = 0.f;
  float glyph_bottom = 0.f;
};

struct VerticalAlign {
  VerticalAlignType type;
  float length;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_VERTICAL_ALIGN_STYLE_H_
