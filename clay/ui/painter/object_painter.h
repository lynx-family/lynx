// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_OBJECT_PAINTER_H_
#define CLAY_UI_PAINTER_OBJECT_PAINTER_H_

#include "clay/gfx/graphics_context.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/ui/component/component_constants.h"

namespace clay {

class FloatPoint;
class FloatRect;
class FloatSize;
class FloatRoundedRect;
class RenderObject;

class ObjectPainter {
 public:
  explicit ObjectPainter(const RenderObject& layout_object)
      : layout_object_(layout_object) {}

  static void DrawDefaultFocusRing(
      GraphicsContext* context, const FloatRoundedRect& round_rect,
      int thickness = num_value::kFocusRingThickness,
      Color color = Color(color_value::kColorGreen));

  const RenderObject& layout_object_;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_OBJECT_PAINTER_H_
