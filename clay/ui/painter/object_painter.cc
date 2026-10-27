// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/object_painter.h"

#include <algorithm>

#include "clay/gfx/geometry/float_rounded_rect.h"

namespace clay {

void ObjectPainter::DrawLineForBoxSide(GraphicsContext* context, int x1, int y1,
                                       int x2, int y2,
                                       BorderSide::SideIndex side, Color color,
                                       BorderStyleType style,
                                       int adjacent_width1, int adjacent_width2,
                                       bool antialias) {
  int thickness;
  int length;
  if (side == BorderSide::TOP || side == BorderSide::BOTTOM) {
    thickness = y2 - y1;
    length = x2 - x1;
  } else {
    thickness = x2 - x1;
    length = y2 - y1;
  }

  // We would like this check to be an ASSERT as we don't want to draw empty
  // borders. However nothing guarantees that the following recursive calls to
  // DrawLineForBoxSide will have positive thickness and length.
  if (length <= 0 || thickness <= 0) {
    return;
  }

  if (style == BorderStyleType::kDouble && thickness < 3) {
    style = BorderStyleType::kSolid;
  }

  switch (style) {
    case BorderStyleType::kNone:
    case BorderStyleType::kHide:
      return;
    case BorderStyleType::kDotted:
    case BorderStyleType::kDashed:
      DrawDashedOrDottedBoxSide(context, x1, y1, x2, y2, side, color, thickness,
                                length, style, antialias);
      break;
    case BorderStyleType::kDouble:
      DrawDoubleBoxSide(context, x1, y1, x2, y2, length, side, color, thickness,
                        adjacent_width1, adjacent_width2, antialias);
      break;
    case BorderStyleType::kRidge:
    case BorderStyleType::kGroove:
      DrawRidgeOrGrooveBoxSide(context, x1, y1, x2, y2, side, color, style,
                               adjacent_width1, adjacent_width2, antialias);
      break;
    case BorderStyleType::kInset:
      if (side == BorderSide::TOP || side == BorderSide::LEFT) {
        DrawSolidBoxSide(context, x1, y1, x2, y2, side, color, adjacent_width1,
                         adjacent_width2, antialias);
      }
      break;
    case BorderStyleType::kOutset:
      if (side == BorderSide::BOTTOM || side == BorderSide::RIGHT) {
        DrawSolidBoxSide(context, x1, y1, x2, y2, side, color, adjacent_width1,
                         adjacent_width2, antialias);
      }
      break;
    case BorderStyleType::kSolid:
      DrawSolidBoxSide(context, x1, y1, x2, y2, side, color, adjacent_width1,
                       adjacent_width2, antialias);
      break;
    case BorderStyleType::kUndefined:
    default:
      break;
  }
}

void ObjectPainter::DrawDashedOrDottedBoxSide(GraphicsContext* context, int x1,
                                              int y1, int x2, int y2,
                                              BorderSide::SideIndex side,
                                              Color color, int thickness,
                                              int length, BorderStyleType style,
                                              bool antialias) {
  Paint paint;
  paint.setColor(color);
  paint.setStrokeWidth(thickness);
  BorderStyleType stroke_style =
      (style == BorderStyleType::kDashed ? BorderStyleType::kDashed
                                         : BorderStyleType::kDotted);

  GraphicsContext::SetupPaintDashPathEffect(&paint, length, thickness,
                                            stroke_style);
  switch (side) {
    case BorderSide::BOTTOM:
    case BorderSide::TOP: {
      int mid_y = y1 + thickness / 2;
      context->DrawLine(paint, FloatPoint(x1, mid_y), FloatPoint(x2, mid_y),
                        thickness, stroke_style);
      break;
    }
    case BorderSide::RIGHT:
    case BorderSide::LEFT: {
      int mid_x = x1 + thickness / 2;
      context->DrawLine(paint, FloatPoint(mid_x, y1), FloatPoint(mid_x, y2),
                        thickness, stroke_style);
      break;
    }
  }
}

void ObjectPainter::DrawDoubleBoxSide(GraphicsContext* context, int x1, int y1,
                                      int x2, int y2, int length,
                                      BorderSide::SideIndex side, Color color,
                                      int thickness, int adjacent_width1,
                                      int adjacent_width2, bool antialias) {
  int third_of_thickness = (thickness + 1) / 3;

  if (adjacent_width1 != 0.0 && adjacent_width2 != 0.0) {
    switch (side) {
      case BorderSide::TOP:
      case BorderSide::BOTTOM: {
        Paint paint;
        paint.setColor(color);
        context->DrawRect(
            paint, skity::Rect::MakeXYWH(x1, y1, length, third_of_thickness));
        context->DrawRect(
            paint, skity::Rect::MakeXYWH(x1, y2 - third_of_thickness, length,
                                         third_of_thickness));
        break;
      }
      case BorderSide::LEFT:
      case BorderSide::RIGHT: {
        Paint paint;
        paint.setColor(color);
        context->DrawRect(
            paint, skity::Rect::MakeXYWH(x1, y1, third_of_thickness, length));
        context->DrawRect(paint,
                          skity::Rect::MakeXYWH(x2 - third_of_thickness, y1,
                                                third_of_thickness, length));
        break;
      }
    }

    return;
  }

  int adjacent1_big_third =
      ((adjacent_width1 > 0) ? adjacent_width1 + 1 : adjacent_width1 - 1) / 3;
  int adjacent2_big_third =
      ((adjacent_width2 > 0) ? adjacent_width2 + 1 : adjacent_width2 - 1) / 3;

  switch (side) {
    case BorderSide::TOP:
      DrawLineForBoxSide(
          context, x1 + std::max((-adjacent_width1 * 2 + 1) / 3, 0), y1,
          x2 - std::max((-adjacent_width2 * 2 + 1) / 3, 0),
          y1 + third_of_thickness, side, color, BorderStyleType::kSolid,
          adjacent1_big_third, adjacent2_big_third, antialias);
      DrawLineForBoxSide(context,
                         x1 + std::max((adjacent_width1 * 2 + 1) / 3, 0),
                         y2 - third_of_thickness,
                         x2 - std::max((adjacent_width2 * 2 + 1) / 3, 0), y2,
                         side, color, BorderStyleType::kSolid,
                         adjacent1_big_third, adjacent2_big_third, antialias);
      break;
    case BorderSide::LEFT:
      DrawLineForBoxSide(context, x1,
                         y1 + std::max((-adjacent_width1 * 2 + 1) / 3, 0),
                         x1 + third_of_thickness,
                         y2 - std::max((-adjacent_width2 * 2 + 1) / 3, 0), side,
                         color, BorderStyleType::kSolid, adjacent1_big_third,
                         adjacent2_big_third, antialias);
      DrawLineForBoxSide(context, x2 - third_of_thickness,
                         y1 + std::max((adjacent_width1 * 2 + 1) / 3, 0), x2,
                         y2 - std::max((adjacent_width2 * 2 + 1) / 3, 0), side,
                         color, BorderStyleType::kSolid, adjacent1_big_third,
                         adjacent2_big_third, antialias);
      break;
    case BorderSide::BOTTOM:
      DrawLineForBoxSide(
          context, x1 + std::max((adjacent_width1 * 2 + 1) / 3, 0), y1,
          x2 - std::max((adjacent_width2 * 2 + 1) / 3, 0),
          y1 + third_of_thickness, side, color, BorderStyleType::kSolid,
          adjacent1_big_third, adjacent2_big_third, antialias);
      DrawLineForBoxSide(context,
                         x1 + std::max((-adjacent_width1 * 2 + 1) / 3, 0),
                         y2 - third_of_thickness,
                         x2 - std::max((-adjacent_width2 * 2 + 1) / 3, 0), y2,
                         side, color, BorderStyleType::kSolid,
                         adjacent1_big_third, adjacent2_big_third, antialias);
      break;
    case BorderSide::RIGHT:
      DrawLineForBoxSide(context, x1,
                         y1 + std::max((adjacent_width1 * 2 + 1) / 3, 0),
                         x1 + third_of_thickness,
                         y2 - std::max((adjacent_width2 * 2 + 1) / 3, 0), side,
                         color, BorderStyleType::kSolid, adjacent1_big_third,
                         adjacent2_big_third, antialias);
      DrawLineForBoxSide(context, x2 - third_of_thickness,
                         y1 + std::max((-adjacent_width1 * 2 + 1) / 3, 0), x2,
                         y2 - std::max((-adjacent_width2 * 2 + 1) / 3, 0), side,
                         color, BorderStyleType::kSolid, adjacent1_big_third,
                         adjacent2_big_third, antialias);
      break;
    default:
      break;
  }
}

void ObjectPainter::DrawRidgeOrGrooveBoxSide(
    GraphicsContext* context, int x1, int y1, int x2, int y2,
    BorderSide::SideIndex side, Color color, BorderStyleType style,
    int adjacent_width1, int adjacent_width2, bool antialias) {
  BorderStyleType s1;
  BorderStyleType s2;
  if (style == BorderStyleType::kGroove) {
    s1 = BorderStyleType::kInset;
    s2 = BorderStyleType::kOutset;
  } else {
    s1 = BorderStyleType::kOutset;
    s2 = BorderStyleType::kInset;
  }

  int adjacent1_big_half =
      ((adjacent_width1 > 0) ? adjacent_width1 + 1 : adjacent_width1 - 1) / 2;
  int adjacent2_big_half =
      ((adjacent_width2 > 0) ? adjacent_width2 + 1 : adjacent_width2 - 1) / 2;

  switch (side) {
    case BorderSide::TOP:
      DrawLineForBoxSide(context, x1 + std::max(-adjacent_width1, 0) / 2, y1,
                         x2 - std::max(-adjacent_width2, 0) / 2,
                         (y1 + y2 + 1) / 2, side, color, s1, adjacent1_big_half,
                         adjacent2_big_half, antialias);
      DrawLineForBoxSide(
          context, x1 + std::max(adjacent_width1 + 1, 0) / 2, (y1 + y2 + 1) / 2,
          x2 - std::max(adjacent_width2 + 1, 0) / 2, y2, side, color, s2,
          adjacent_width1 / 2, adjacent_width2 / 2, antialias);
      break;
    case BorderSide::LEFT:
      DrawLineForBoxSide(context, x1, y1 + std::max(-adjacent_width1, 0) / 2,
                         (x1 + x2 + 1) / 2,
                         y2 - std::max(-adjacent_width2, 0) / 2, side, color,
                         s1, adjacent1_big_half, adjacent2_big_half, antialias);
      DrawLineForBoxSide(
          context, (x1 + x2 + 1) / 2, y1 + std::max(adjacent_width1 + 1, 0) / 2,
          x2, y2 - std::max(adjacent_width2 + 1, 0) / 2, side, color, s2,
          adjacent_width1 / 2, adjacent_width2 / 2, antialias);
      break;
    case BorderSide::BOTTOM:
      DrawLineForBoxSide(context, x1 + std::max(adjacent_width1, 0) / 2, y1,
                         x2 - std::max(adjacent_width2, 0) / 2,
                         (y1 + y2 + 1) / 2, side, color, s2, adjacent1_big_half,
                         adjacent2_big_half, antialias);
      DrawLineForBoxSide(
          context, x1 + std::max(-adjacent_width1 + 1, 0) / 2,
          (y1 + y2 + 1) / 2, x2 - std::max(-adjacent_width2 + 1, 0) / 2, y2,
          side, color, s1, adjacent_width1 / 2, adjacent_width2 / 2, antialias);
      break;
    case BorderSide::RIGHT:
      DrawLineForBoxSide(context, x1, y1 + std::max(adjacent_width1, 0) / 2,
                         (x1 + x2 + 1) / 2,
                         y2 - std::max(adjacent_width2, 0) / 2, side, color, s2,
                         adjacent1_big_half, adjacent2_big_half, antialias);
      DrawLineForBoxSide(context, (x1 + x2 + 1) / 2,
                         y1 + std::max(-adjacent_width1 + 1, 0) / 2, x2,
                         y2 - std::max(-adjacent_width2 + 1, 0) / 2, side,
                         color, s1, adjacent_width1 / 2, adjacent_width2 / 2,
                         antialias);
      break;
  }
}

void ObjectPainter::DrawSolidBoxSide(GraphicsContext* context, int x1, int y1,
                                     int x2, int y2, BorderSide::SideIndex side,
                                     Color color, int adjacent_width1,
                                     int adjacent_width2, bool antialias) {
  if (adjacent_width1 == 0.0 && adjacent_width2 == 0.0) {
    Paint paint;
    paint.setColor(color);
    context->DrawRect(paint, skity::Rect::MakeXYWH(x1, y1, x2 - x1, y2 - y1));
    return;
  }

  FloatPoint quad[4];
  switch (side) {
    case BorderSide::TOP:
      quad[0] = FloatPoint(x1 + std::max(-adjacent_width1, 0), y1);
      quad[1] = FloatPoint(x1 + std::max(adjacent_width1, 0), y2);
      quad[2] = FloatPoint(x2 - std::max(adjacent_width2, 0), y2);
      quad[3] = FloatPoint(x2 - std::max(-adjacent_width2, 0), y1);
      break;
    case BorderSide::BOTTOM:
      quad[0] = FloatPoint(x1 + std::max(adjacent_width1, 0), y1);
      quad[1] = FloatPoint(x1 + std::max(-adjacent_width1, 0), y2);
      quad[2] = FloatPoint(x2 - std::max(-adjacent_width2, 0), y2);
      quad[3] = FloatPoint(x2 - std::max(adjacent_width2, 0), y1);
      break;
    case BorderSide::LEFT:
      quad[0] = FloatPoint(x1, y1 + std::max(-adjacent_width1, 0));
      quad[1] = FloatPoint(x1, y2 - std::max(-adjacent_width2, 0));
      quad[2] = FloatPoint(x2, y2 - std::max(adjacent_width2, 0));
      quad[3] = FloatPoint(x2, y1 + std::max(adjacent_width1, 0));
      break;
    case BorderSide::RIGHT:
      quad[0] = FloatPoint(x1, y1 + std::max(adjacent_width1, 0));
      quad[1] = FloatPoint(x1, y2 - std::max(adjacent_width2, 0));
      quad[2] = FloatPoint(x2, y2 - std::max(-adjacent_width2, 0));
      quad[3] = FloatPoint(x2, y1 + std::max(-adjacent_width1, 0));
      break;
  }

  context->FillPolygon(4, quad, color, antialias);
}

void ObjectPainter::DrawDefaultFocusRing(GraphicsContext* context,
                                         const FloatRoundedRect& round_rect,
                                         int thickness, Color color) {
  Paint paint;
  paint.setColor(color);
  paint.setDrawStyle(DrawStyle::kStroke);
  paint.setStrokeWidth(static_cast<float>(thickness));

  context->DrawRRect(round_rect, paint);
}

}  // namespace clay
