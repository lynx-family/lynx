// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BOX_PAINTER_H_
#define CLAY_UI_PAINTER_BOX_PAINTER_H_

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/ui/painter/border_painter.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

struct BackgroundData;
class BordersData;
class OutlineData;
class RenderObject;

class BoxPainter {
 public:
  explicit BoxPainter(RenderBox* box);

  void SetOuterRect(const FloatRect& rect) {
    border_painter_.SetOuterRect(rect);
  }

  void SetOutline(const OutlineData& outline_data, const FloatRect& rect_box) {
    border_painter_.SetOutline(outline_data, rect_box);
  }

  void SetBorder(const BordersData& borders_data, const FloatRect& box_rect) {
    border_painter_.SetBorder(borders_data, box_rect);
  }

  void Paint(GraphicsContext* context, const FloatPoint& offset);
  void PaintBoxDecorationBackground(GraphicsContext* context,
                                    const FloatPoint& offset,
                                    const FloatRect& box_rect);
  // TODO(zhangxiao.ninja): add paint shdow, mask here

 private:
  void PaintBackground(GraphicsContext* context,
                       const BackgroundData& background,
                       const FloatRect& paint_rect);
  BorderPainter border_painter_;
  RenderBox* render_object_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BOX_PAINTER_H_
