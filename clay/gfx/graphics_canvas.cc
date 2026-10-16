// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/graphics_canvas.h"

#include "clay/gfx/graphics_context.h"

namespace clay {

void GraphicsCanvas::DrawLine(Paint stroke_paint, const FloatPoint& point1,
                              const FloatPoint& point2, float thickness,
                              BorderStyleType style) {
  FloatPoint p1 = point1;
  FloatPoint p2 = point2;
  bool is_vertical_line = (p1.x() == p2.x());
  int width = roundf(thickness);

  if (style == BorderStyleType::kDotted || style == BorderStyleType::kDashed) {
    // Do a rect fill of our endpoints.  This ensures we always have the
    // appearance of being a border.  We then draw the actual dotted/dashed
    // line.
    skity::Rect r1 =
        skity::Rect::MakeLTRB(p1.x(), p1.y(), p1.x() + width, p1.y() + width);
    skity::Rect r2 =
        skity::Rect::MakeLTRB(p2.x(), p2.y(), p2.x() + width, p2.y() + width);

    if (is_vertical_line) {
      r1.Offset(-width / 2, 0);
      r2.Offset(-width / 2, -width);
    } else {
      r1.Offset(0, -width / 2);
      r2.Offset(-width, -width / 2);
    }
    Paint fill_paint;
    fill_paint.setColor(stroke_paint.getColor());
    DrawRect(r1, fill_paint);
    DrawRect(r2, fill_paint);
  }

  GraphicsContext::AdjustLineToPixelBoundaries(&p1, &p2, width, style);
  DrawLine(p1.x(), p1.y(), p2.x(), p2.y(), stroke_paint);
}

void GraphicsCanvas::FillPolygon(size_t num_points, const FloatPoint* points,
                                 const Color& color, bool should_antialias) {
  clay::GrPath path;
  GraphicsContext::SetPathFromPoints(&path, num_points, points);

  Paint paint;
  paint.setAntiAlias(should_antialias);
  paint.setColor(color);

  DrawPath(path, paint);
}

}  // namespace clay
