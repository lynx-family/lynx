// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BOX_SHADOW_PAINTER_H_
#define CLAY_UI_PAINTER_BOX_SHADOW_PAINTER_H_

#include <vector>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/graphics_canvas.h"
#include "clay/gfx/style/shadow.h"

namespace clay {

class BordersData;

// Draw shadows in reverse declaration order using the box and border geometry.
void PaintBoxShadows(GraphicsCanvas* canvas, const std::vector<Shadow>& shadows,
                     const FloatRect& box_rect,
                     const BordersData* borders = nullptr);

// Draw a shadow with precomputed contours. For inset shadows, shadow_rect is
// the hole and clip_rect is the inner contour; otherwise they are the shadow
// shape and the original box contour.
void PaintBoxShadow(GraphicsCanvas* canvas, const skity::RRect& shadow_rect,
                    const skity::RRect& clip_rect, Color color,
                    float blur_radius, bool inset);

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BOX_SHADOW_PAINTER_H_
