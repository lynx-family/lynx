// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_BORDER_PAINTER_H_
#define CLAY_GFX_BORDER_PAINTER_H_

#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/graphics_canvas.h"
#include "clay/gfx/style/border_side.h"
#include "clay/gfx/style/outline_data.h"

namespace clay {

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
                    const BordersData& border_data, bool is_outline = false);
  void PaintOutline(GraphicsCanvas* context, const OutlineData& outline_data);
  bool IsBorderClip();
  FloatRoundedRect GetBackgroundRoundedRect(const FloatRect& paint_rect);
  static void SetRRectRadii(skity::RRect& rrect, const skity::Rect& rect,
                            const BordersData& borders_data,
                            float radii_offset = 0);

 private:
  void SetupPaintDashPathEffect(class Paint& paint, const int length,
                                const float border_thickness,
                                const BorderStyleType border_style) const;
  void DrawLineForDashed(GraphicsCanvas* context, const FloatRect& side_rect,
                         float thickness, const BorderSide& side, Color color,
                         BorderStyleType border_style,
                         bool is_outline = false) const;
  void PaintBorderSide(GraphicsCanvas* context, const FloatRect& border_rect,
                       const BordersData& borders_data,
                       bool is_outline = false);
  void PaintSide(GraphicsCanvas* context, const FloatRect& border_rect,
                 const BorderSide& side, const BordersData& borders_data,
                 bool is_outline = false);
  void PaintOneBorderSide(GraphicsCanvas* context, const FloatRect& side_rect,
                          const BorderSide& side,
                          const BorderSide& relative_side1,
                          const BorderSide& relative_side2, const GrPath& path,
                          const BordersData& borders_data,
                          bool is_outline = false) const;
  void DrawBorderSideFromPath(GraphicsCanvas* context,
                              const FloatRect& border_rect,
                              const GrPath& border_path, float thickness,
                              float draw_thickness, const BorderSide& side,
                              Color color, BorderStyleType border_style,
                              bool is_outline = false) const;
  bool PaintSimpleSameBorder(GraphicsCanvas* context,
                             const FloatRect& border_rect,
                             const BordersData& borders_data,
                             bool is_outline = false);

  bool CheckSameColorAndSolidBorder();
  void PaintSameColorAndSolidBorder(GraphicsCanvas* context);

  void ClipBorderSideForComplexInnerPath(GraphicsCanvas* context,
                                         const BorderSide& side,
                                         const BordersData& borders_data,
                                         bool is_outline = false) const;
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
  bool IsBackgroundBleedAvoidanceShrink();

  BorderSide box_sides_[4];
  BorderSide outline_box_sides_[4];
  FloatRoundedRect outer_rounded_rect_;
  FloatRoundedRect inner_rounded_rect_;
  FloatRoundedRect outline_outer_rounded_rect_;
  FloatRoundedRect outline_inner_rounded_rect_;
  FloatPoint origin_;
};

}  // namespace clay

#endif  // CLAY_GFX_BORDER_PAINTER_H_
