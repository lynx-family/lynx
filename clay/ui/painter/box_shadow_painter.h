// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BOX_SHADOW_PAINTER_H_
#define CLAY_UI_PAINTER_BOX_SHADOW_PAINTER_H_

#include "clay/gfx/graphics_context.h"

namespace clay {

class RenderBox;

class BoxShadowPainter {
 public:
  explicit BoxShadowPainter(const RenderBox* render_box)
      : render_object_(render_box) {}

  void Paint(GraphicsContext* context, const FloatRect& box_rect);

 private:
  const RenderBox* render_object_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BOX_SHADOW_PAINTER_H_
