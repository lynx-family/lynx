// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BORDER_PAINTER_H_
#define CLAY_UI_PAINTER_BORDER_PAINTER_H_

#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/outline_data.h"
#include "clay/ui/painter/border_side.h"

namespace clay {

class GraphicsCanvas;
class Paint;

// Draws border/outline geometry without a render object. The origin converts
// stored box coordinates to the caller's local painting coordinates.
class BorderPainter {
 public:
  explicit BorderPainter(const FloatPoint& origin = {}) : origin_(origin) {}
  void SetOuterRect(const FloatRect& rect) {
    outer_rounded_rect_.SetRect(rect);
  }
  const FloatRoundedRect& OuterRect() const { return outer_rounded_rect_; }
  void SetBorder(const BordersData& borders_data, const FloatRect& box_rect);
  void SetOutline(const OutlineData& outline_data, const FloatRect& box_rect);
  void PaintBorders(GraphicsCanvas* context, const FloatRect& rect,
                    const BordersData& borders_data) const;
  void PaintOutline(GraphicsCanvas* context,
                    const OutlineData& outline_data) const;
  FloatRoundedRect GetBackgroundRoundedRect(const FloatRect& paint_rect) const;

 private:
  bool IsBorderClip() const;

  static void SetupPaintDashPathEffect(Paint& paint, int length,
                                       float border_thickness,
                                       BorderStyleType border_style);
  void DrawLineForDashed(GraphicsCanvas* context, const FloatRect& side_rect,
                         float thickness, const BorderSide& side, Color color,
                         BorderStyleType border_style) const;
  void PaintBorderSide(GraphicsCanvas* context, const FloatRect& border_rect,
                       const BordersData& borders_data,
                       bool is_outline = false) const;
  void PaintSide(GraphicsCanvas* context, const FloatRect& border_rect,
                 const BorderSide& side, const BordersData& borders_data,
                 bool is_outline = false) const;
  void PaintOneBorderSide(GraphicsCanvas* context, const FloatRect& side_rect,
                          const BorderSide& side,
                          const BorderSide& relative_side1,
                          const BorderSide& relative_side2, const GrPath& path,
                          const BordersData& borders_data,
                          bool is_outline = false) const;
  void DrawBorderSideFromPath(GraphicsCanvas* context,
                              const FloatRect& border_rect,
                              const GrPath& border_path, float thickness,
                              const BorderSide& side, Color color,
                              BorderStyleType border_style,
                              bool is_outline = false) const;

  bool CheckSameColorAndSolidBorder() const;
  void PaintSameColorAndSolidBorder(GraphicsCanvas* context) const;

  void ClipBorderSideForComplexInnerPath(GraphicsCanvas* context,
                                         const BorderSide& side,
                                         const BordersData& borders_data) const;
  void ClipBorderSidePolygon(GraphicsCanvas* context, const BorderSide& side,
                             bool should_aa1, bool should_aa2,
                             bool is_outline = false) const;

  static void DrawLineForBoxSide(GraphicsCanvas* context, int x1, int y1,
                                 int x2, int y2, BorderSide::SideIndex side,
                                 Color, BorderStyleType, int adjacent_width1,
                                 int adjacent_width2, bool antialias = false);

  static void DrawDashedOrDottedBoxSide(GraphicsCanvas* context, int x1, int y1,
                                        int x2, int y2,
                                        BorderSide::SideIndex side, Color,
                                        int thickness, int length,
                                        BorderStyleType, bool antialias);
  static void DrawDoubleBoxSide(GraphicsCanvas* context, int x1, int y1, int x2,
                                int y2, int length, BorderSide::SideIndex side,
                                Color, int thickness, int adjacent_width1,
                                int adjacent_width2, bool antialias);
  static void DrawRidgeOrGrooveBoxSide(GraphicsCanvas* context, int x1, int y1,
                                       int x2, int y2,
                                       BorderSide::SideIndex side, Color,
                                       BorderStyleType, int adjacent_width1,
                                       int adjacent_width2, bool antialias);
  static void DrawSolidBoxSide(GraphicsCanvas* context, int x1, int y1, int x2,
                               int y2, BorderSide::SideIndex side, Color,
                               int adjacent_width1, int adjacent_width2,
                               bool antialias);
  bool IsBackgroundBleedAvoidanceShrink() const;

  BorderSide box_sides_[4];
  BorderSide outline_box_sides_[4];
  FloatRoundedRect outer_rounded_rect_;
  FloatRoundedRect inner_rounded_rect_;
  FloatRoundedRect outline_outer_rounded_rect_;
  FloatRoundedRect outline_inner_rounded_rect_;
  FloatPoint origin_;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BORDER_PAINTER_H_
