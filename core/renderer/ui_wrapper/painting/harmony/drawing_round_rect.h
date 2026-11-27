// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_DRAWING_ROUND_RECT_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_DRAWING_ROUND_RECT_H_

#include <native_drawing/drawing_round_rect.h>

#include <cmath>
#include <cstddef>
#include <memory>

namespace lynx::tasm {

using DrawingRoundRect =
    std::unique_ptr<OH_Drawing_RoundRect,
                    decltype(&OH_Drawing_RoundRectDestroy)>;

inline DrawingRoundRect CreateDrawingRoundRect(const OH_Drawing_Rect* rect,
                                               const float (&radii)[8],
                                               float density) {
  DrawingRoundRect round_rect(
      rect ? OH_Drawing_RoundRectCreate(rect, 0.f, 0.f) : nullptr,
      OH_Drawing_RoundRectDestroy);
  if (!round_rect) {
    return round_rect;
  }
  constexpr OH_Drawing_CornerPos kCornerPositions[] = {
      CORNER_POS_TOP_LEFT, CORNER_POS_TOP_RIGHT, CORNER_POS_BOTTOM_RIGHT,
      CORNER_POS_BOTTOM_LEFT};
  for (std::size_t i = 0; i < 4; ++i) {
    const float x = radii[2 * i] * density;
    const float y = radii[2 * i + 1] * density;
    if (!std::isfinite(x) || !std::isfinite(y)) {
      return {nullptr, OH_Drawing_RoundRectDestroy};
    }
    // A corner with either zero axis is square.
    if (x > 0.f && y > 0.f) {
      OH_Drawing_RoundRectSetCorner(round_rect.get(), kCornerPositions[i],
                                    {x, y});
    }
  }
  return round_rect;
}

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_HARMONY_DRAWING_ROUND_RECT_H_
