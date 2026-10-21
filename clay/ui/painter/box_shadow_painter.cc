// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/box_shadow_painter.h"

#include <memory>

#include "clay/gfx/graphics_context.h"
#include "clay/gfx/rendering_backend.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/ui/common/utils/floating_comparison.h"

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

}  // namespace

void PaintBoxShadow(GraphicsCanvas* canvas, const skity::RRect& shadow_rect,
                    const skity::RRect& clip_rect, Color color,
                    float blur_radius, bool inset) {
  Paint paint;
  paint.setAntiAlias(true);
  PreparePaint(paint, color, blur_radius);
  GraphicsContext::AutoRestore saver(canvas, true);
  if (!inset) {
    // Only keep the painting region out of box.
    GrPath path;
    PATH_ADD_RRECT(path, clip_rect);
    canvas->ClipPath(path, GrClipOp::kDifference, true);
    canvas->DrawRRect(shadow_rect, paint);
  } else {
    // This is a workaround for skia's anti alias issue. We need to calculate
    // the intersection first and then draw the new path.
    GrPath path_hole_rrect, path_inner_rrect, intersect_path, draw_path;
    PATH_ADD_RRECT(path_hole_rrect, shadow_rect);
    PATH_ADD_RRECT(path_inner_rrect, clip_rect);
    // Cast int to PathOp.
    PATH_OP(path_hole_rrect, path_inner_rrect, 1, intersect_path);
    PATH_ADD_RRECT(draw_path, clip_rect);
    PATH_ADD_PATH(draw_path, intersect_path);
    // Cast int to fill type.
    PATH_SET_FILL_TYPE(draw_path, 1);
    // Only keep painting region in the box without border.
    canvas->ClipRRect(clip_rect, GrClipOp::kIntersect, true);
    canvas->DrawPath(draw_path, paint);
  }
}

// Paint a series of shadows one by one sequentially onto a region.
// |box_rect| contains border width but without radii.
void PaintBoxShadows(GraphicsCanvas* canvas, const std::vector<Shadow>& shadows,
                     const FloatRect& box_rect, const BordersData* borders) {
  auto layout_rect = skity::Rect::MakeXYWH(box_rect.x(), box_rect.y(),
                                           box_rect.width(), box_rect.height());
  // create contour rounded rect with border radius considered.
  skity::RRect contour_rrect = skity::RRect::MakeRect(layout_rect);
  if (borders) {
    contour_rrect = RoundedRectInBorder(layout_rect, *borders);
  }

  // the first shadow would be displayed on the top
  // most, so the drawing order should be reverse
  for (auto it = shadows.rbegin(); it != shadows.rend(); ++it) {
    const Shadow& shadow = *it;
    auto shadow_rect = contour_rrect;
    shadow_rect.Offset(shadow.offset_x, shadow.offset_y);
    auto clip_rect = contour_rrect;
    if (!shadow.inset) {
      shadow_rect.Outset(shadow.spread_radius, shadow.spread_radius,
                         &shadow_rect);
    } else {
      if (borders) {
        // Inset shadow will be affected by borders.
        RRectInBorder(*borders, shadow_rect);
        RRectInBorder(*borders, clip_rect);
      }
      if (RoughlyNotZero(shadow.spread_radius)) {
        shadow_rect.Inset(shadow.spread_radius, shadow.spread_radius,
                          &shadow_rect);
      }
    }
    PaintBoxShadow(canvas, shadow_rect, clip_rect, shadow.color,
                   shadow.blur_radius, shadow.inset);
  }
}

}  // namespace clay
