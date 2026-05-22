// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/box_shadow_painter.h"

#include <memory>
#include <vector>

#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/shadow.h"
#include "clay/ui/common/utils/floating_comparison.h"
#include "clay/ui/rendering/render_box.h"

namespace clay {

namespace {
// Return the inner contour rect with border radius considered in.
// New radii will be reduced according to border width and may be reach 0.f
skity::RRect RoundedRectInBorder(const skity::Rect& rect,
                                 const BordersData& borders) {
  skity::RRect contour_rrect;
  skity::Vec2 corners[4] = {
      {borders.radius_x_top_left_, borders.radius_y_top_left_},
      {borders.radius_x_top_right_, borders.radius_y_top_right_},
      {borders.radius_x_bottom_right_, borders.radius_y_bottom_right_},
      {borders.radius_x_bottom_left_, borders.radius_y_bottom_left_}};
  contour_rrect.SetRectRadii(rect, corners);
  return contour_rrect;
}

// Calculate the rect in borders.
// Besides widths, radii should be considered too.
void RRectInBorder(const BordersData& borders, skity::RRect& rrect) {
  auto tempRect = rrect.GetRect();
  tempRect = skity::Rect::MakeLTRB(tempRect.Left() + borders.width_left_,
                                   tempRect.Top() + borders.width_top_,
                                   tempRect.Right() - borders.width_right_,
                                   tempRect.Bottom() - borders.width_bottom_);

  auto ul_radii = rrect.Radii(skity::RRect::kUpperLeft);
  auto ur_radii = rrect.Radii(skity::RRect::kUpperRight);
  auto lr_radii = rrect.Radii(skity::RRect::kLowerRight);
  auto ll_radii = rrect.Radii(skity::RRect::kLowerLeft);
  if (RoughlyNotZero(borders.width_left_)) {
    ul_radii.x -= borders.width_left_;
    ll_radii.x -= borders.width_left_;
  }
  if (RoughlyNotZero(borders.width_top_)) {
    ul_radii.y -= borders.width_top_;
    ur_radii.y -= borders.width_top_;
  }
  if (RoughlyNotZero(borders.width_right_)) {
    ur_radii.x -= borders.width_right_;
    lr_radii.x -= borders.width_right_;
  }
  if (RoughlyNotZero(borders.width_bottom_)) {
    ll_radii.y -= borders.width_bottom_;
    lr_radii.y -= borders.width_bottom_;
  }

  skity::Vec2 radii[4] = {ul_radii, ur_radii, lr_radii, ll_radii};
  rrect.SetRectRadii(tempRect, radii);
}

void PreparePaint(Paint& paint, Color color, float blur_radius) {
  paint.setColor(color);
  if (RoughlyNotZero(blur_radius)) {
    paint.setMaskFilter(std::make_shared<BlurMaskFilter>(
        BLUR_STYLE_NORMAL, GraphicsContext::ConvertRadiusToSigma(blur_radius)));
  } else {
    paint.setMaskFilter(nullptr);
  }
}

void PaintShadow(GraphicsContext* context, const Shadow& shadow,
                 const skity::RRect& contour_rrect,
                 std::optional<BordersData> borders, Paint& paint) {
  PreparePaint(paint, shadow.color, shadow.blur_radius);
  GraphicsContext::AutoRestore saver(context, true);
  if (!shadow.inset) {
    // Only keep the painting region out of box.
    GrPath path;
    PATH_ADD_RRECT(path, contour_rrect);
    context->ClipPath(path, GrClipOp::kDifference, true);

    auto shadow_rect = contour_rrect;
    shadow_rect.Offset(shadow.offset_x, shadow.offset_y);
    shadow_rect.Outset(shadow.spread_radius, shadow.spread_radius,
                       &shadow_rect);
    context->DrawRRect(shadow_rect, paint);
  } else {
    auto hole_rrect = contour_rrect;
    hole_rrect.Offset(shadow.offset_x, shadow.offset_y);
    auto inner_rrect = contour_rrect;
    if (borders.has_value()) {
      // Inset shadow will be affected by borders.
      RRectInBorder(*borders, hole_rrect);
      RRectInBorder(*borders, inner_rrect);
    }
    if (RoughlyNotZero(shadow.spread_radius)) {
      hole_rrect.Inset(shadow.spread_radius, shadow.spread_radius, &hole_rrect);
    }
    // This is a workaround for skia's anti alias issue. We need to calculate
    // the intersection first and then draw the new path. See meego
    // issue:3000303346 for more details.
    GrPath path_hole_rrect, path_inner_rrect, intersect_path, draw_path;
    PATH_ADD_RRECT(path_hole_rrect, hole_rrect);
    PATH_ADD_RRECT(path_inner_rrect, inner_rrect);
    // Cast int to PathOp.
    PATH_OP(path_hole_rrect, path_inner_rrect, 1, intersect_path);
    PATH_ADD_RRECT(draw_path, inner_rrect);
    PATH_ADD_PATH(draw_path, intersect_path);
    // Cast int to fill type.
    PATH_SET_FILL_TYPE(draw_path, 1);
    // Only keep painting region in the box without border.
    context->ClipRRect(inner_rrect, GrClipOp::kIntersect, true);
    context->DrawPath(draw_path, paint);
  }
}

// Paint a series of shadows one by one sequentially onto a region.
// |layout_rect| contains border width but without radii.
void PaintShadows(GraphicsContext* context, const std::vector<Shadow>& shadows,
                  const skity::Rect& layout_rect,
                  std::optional<BordersData> borders) {
  Paint paint;
  paint.setAntiAlias(true);
  // create contour rounded rect with border radius considered.
  skity::RRect contour_rrect = skity::RRect::MakeRect(layout_rect);
  if (borders.has_value()) {
    contour_rrect = RoundedRectInBorder(layout_rect, borders.value());
  }

  // the first shadow would be displayed on the top
  // most, so the drawing order should be reverse
  for (auto it = shadows.rbegin(); it != shadows.rend(); ++it) {
    PaintShadow(context, *it, contour_rrect, borders, paint);
  }
}
}  // namespace

void BoxShadowPainter::Paint(GraphicsContext* context,
                             const FloatRect& box_rect) {
  if (!render_object_ || !render_object_->HasShadow()) {
    return;
  }

  auto layout_rect = skity::Rect::MakeXYWH(box_rect.x(), box_rect.y(),
                                           box_rect.width(), box_rect.height());
  PaintShadows(context, render_object_->Shadows(), layout_rect,
               render_object_->HasBorder() ? render_object_->Border()
                                           : std::optional<BordersData>());
}

}  // namespace clay
