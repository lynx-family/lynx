// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PAINTER_BOX_PAINTER_H_
#define CLAY_UI_PAINTER_BOX_PAINTER_H_

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rounded_rect.h"
#include "clay/gfx/graphics_context.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/ui/painter/border_side.h"
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
    outer_rounded_rect_.SetRect(rect);
  }

  void SetOutline(const OutlineData& outline_data, const FloatRect& rect_box);

  void SetBorder(const BordersData& borders_data, const FloatRect& box_rect);

  void Paint(GraphicsContext* context, const FloatPoint& offset);
  void PaintBoxDecorationBackground(GraphicsContext* context,
                                    const FloatPoint& offset,
                                    const FloatRect& box_rect);
  // TODO(zhangxiao.ninja): add paint shdow, mask here

 private:
  void SetupPaintDashPathEffect(class Paint& paint, const int length,
                                const float border_thickness,
                                const BorderStyleType border_style) const;
  void PaintBackground(GraphicsContext* context,
                       const BackgroundData& background,
                       const FloatRect& paint_rect);
  void DrawLineForDashed(GraphicsContext* context, const FloatRect& side_rect,
                         float thickness, const BorderSide& side, Color color,
                         BorderStyleType border_style,
                         bool is_outline = false) const;
  void PaintBorders(GraphicsContext* context, const FloatRect& rect,
                    const BordersData& border_data, bool is_outline = false);
  void PaintOutline(GraphicsContext* context, const OutlineData& outline_data);
  void PaintBorderSide(GraphicsContext* context, const FloatRect& border_rect,
                       const BordersData& borders_data,
                       bool is_outline = false);
  void PaintSide(GraphicsContext* context, const FloatRect& border_rect,
                 const BorderSide& side, const BordersData& borders_data,
                 bool is_outline = false);
  void PaintOneBorderSide(GraphicsContext* context, const FloatRect& side_rect,
                          const BorderSide& side,
                          const BorderSide& relative_side1,
                          const BorderSide& relative_side2, const GrPath& path,
                          bool is_outline = false) const;
  void DrawBorderSideFromPath(GraphicsContext* context,
                              const FloatRect& border_rect,
                              const GrPath& border_path, float thickness,
                              float draw_thickness, const BorderSide& side,
                              Color color, BorderStyleType border_style,
                              bool is_outline = false) const;
  bool PaintSimpleSameBorder(GraphicsContext* context,
                             const FloatRect& border_rect,
                             const BordersData& borders_data,
                             bool is_outline = false);

  bool CheckSameColorAndSolidBorder();
  void PaintSameColorAndSolidBorder(GraphicsContext* context);

  void ClipBorderSideForComplexInnerPath(GraphicsContext* context,
                                         const BorderSide& side,
                                         bool is_outline = false) const;
  void ClipBorderSidePolygon(GraphicsContext* context, const BorderSide& side,
                             bool should_aa1, bool should_aa2,
                             bool is_outline = false) const;

  bool IsBackgroundBleedAvoidanceShrink();
  FloatRoundedRect GetBackgroundRoundedRect(const FloatRect& paint_rect);
  bool IsBorderClip();

  BorderSide box_sides_[4];
  BorderSide outline_box_sides_[4];
  FloatRoundedRect outer_rounded_rect_;
  FloatRoundedRect inner_rounded_rect_;
  FloatRoundedRect outline_outer_rounded_rect_;
  FloatRoundedRect outline_inner_rounded_rect_;
  RenderBox* render_object_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_PAINTER_BOX_PAINTER_H_
