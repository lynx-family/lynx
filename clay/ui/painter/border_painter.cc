// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/border_painter.h"

#include <algorithm>
#include <cmath>
#include <memory>

#include "clay/gfx/graphics_canvas.h"
#include "clay/gfx/graphics_context.h"
#include "clay/ui/common/utils/floating_comparison.h"

namespace clay {

static const float kExtendFill = 1e-2f;

static FloatRect CalculateSideRectIncludingInner(
    const FloatRoundedRect& outer_border, const BorderSide& side,
    const BordersData& border_data) {
  FloatRect side_rect = outer_border.rect();
  float width;

  switch (side.side_index_) {
    case BorderSide::TOP:
      width = side_rect.height() - border_data.width_bottom_;
      side_rect.SetHeight(width);
      break;
    case BorderSide::BOTTOM:
      width = side_rect.height() - border_data.width_top_;
      side_rect.SetY(side_rect.MaxY() - width);
      side_rect.SetHeight(width);
      break;
    case BorderSide::LEFT:
      width = side_rect.width() - border_data.width_right_;
      side_rect.SetWidth(width);
      break;
    case BorderSide::RIGHT:
      width = side_rect.width() - border_data.width_left_;
      side_rect.SetX(side_rect.MaxX() - width);
      side_rect.SetWidth(width);
      break;
    default:
      break;
  }

  return side_rect;
}

static FloatRoundedRect CalculateAdjustedInnerBorder(
    const FloatRoundedRect& inner_border, const BorderSide& side) {
  // Expand the inner border as necessary to make it a rounded rect (i.e. radii
  // contained within each edge). This function relies on the fact we only get
  // radii not contained within each edge if one of the radii for an edge is
  // zero, so we can shift the arc towards the zero radius corner.
  FloatRoundedRect::Radii new_radii = inner_border.radii();
  FloatRect new_rect = inner_border.rect();

  float overshoot;
  float max_radii;

  switch (side.side_index_) {
    case BorderSide::TOP:
      overshoot = new_radii.TopLeft().width() + new_radii.TopRight().width() -
                  new_rect.width();
      // FIXME: once we start pixel-snapping rounded rects after this point, the
      // overshoot concept should disappear.
      if (overshoot > 0.1) {
        new_rect.SetWidth(new_rect.width() + overshoot);
        if (!new_radii.TopLeft().width()) {
          new_rect.Move(-overshoot, 0);
        }
      }
      new_radii.SetBottomLeft(FloatSize(0, 0));
      new_radii.SetBottomRight(FloatSize(0, 0));
      max_radii =
          std::max(new_radii.TopLeft().height(), new_radii.TopRight().height());
      if (max_radii > new_rect.height()) {
        new_rect.SetHeight(max_radii);
      }
      break;

    case BorderSide::BOTTOM:
      overshoot = new_radii.BottomLeft().width() +
                  new_radii.BottomRight().width() - new_rect.width();
      if (overshoot > 0.1) {
        new_rect.SetWidth(new_rect.width() + overshoot);
        if (!new_radii.BottomLeft().width()) {
          new_rect.Move(-overshoot, 0);
        }
      }
      new_radii.SetTopLeft(FloatSize(0, 0));
      new_radii.SetTopRight(FloatSize(0, 0));
      max_radii = std::max(new_radii.BottomLeft().height(),
                           new_radii.BottomRight().height());
      if (max_radii > new_rect.height()) {
        new_rect.Move(0, new_rect.height() - max_radii);
        new_rect.SetHeight(max_radii);
      }
      break;

    case BorderSide::LEFT:
      overshoot = new_radii.TopLeft().height() +
                  new_radii.BottomLeft().height() - new_rect.height();
      if (overshoot > 0.1) {
        new_rect.SetHeight(new_rect.height() + overshoot);
        if (!new_radii.TopLeft().height()) {
          new_rect.Move(0, -overshoot);
        }
      }
      new_radii.SetTopRight(FloatSize(0, 0));
      new_radii.SetBottomRight(FloatSize(0, 0));
      max_radii =
          std::max(new_radii.TopLeft().width(), new_radii.BottomLeft().width());
      if (max_radii > new_rect.width()) {
        new_rect.SetWidth(max_radii);
      }
      break;

    case BorderSide::RIGHT:
      overshoot = new_radii.TopRight().height() +
                  new_radii.BottomRight().height() - new_rect.height();
      if (overshoot > 0.1) {
        new_rect.SetHeight(new_rect.height() + overshoot);
        if (!new_radii.TopRight().height()) {
          new_rect.Move(0, -overshoot);
        }
      }
      new_radii.SetTopLeft(FloatSize(0, 0));
      new_radii.SetBottomLeft(FloatSize(0, 0));
      max_radii = std::max(new_radii.TopRight().width(),
                           new_radii.BottomRight().width());
      if (max_radii > new_rect.width()) {
        new_rect.Move(new_rect.width() - max_radii, 0);
        new_rect.SetWidth(max_radii);
      }
      break;
  }

  return FloatRoundedRect(new_rect, new_radii);
}

static void SetRRectRadii(skity::RRect& rrect, const skity::Rect& rect,
                          const BordersData& borders_data,
                          float radii_offset = 0) {
  skity::Vec2 radii[4];
  radii[skity::RRect::Corner::kUpperLeft] = {
      borders_data.radius_x_top_left_ - radii_offset,
      borders_data.radius_y_top_left_ - radii_offset};
  radii[skity::RRect::Corner::kUpperRight] = {
      borders_data.radius_x_top_right_ - radii_offset,
      borders_data.radius_y_top_right_ - radii_offset};
  radii[skity::RRect::Corner::kLowerRight] = {
      borders_data.radius_x_bottom_right_ - radii_offset,
      borders_data.radius_y_bottom_right_ - radii_offset};
  radii[skity::RRect::Corner::kLowerLeft] = {
      borders_data.radius_x_bottom_left_ - radii_offset,
      borders_data.radius_y_bottom_left_ - radii_offset};
  rrect.SetRectRadii(rect, radii);
}

static void PaintSolidBorder(GraphicsCanvas* context, const FloatRect& rect,
                             const BordersData& borders_data) {
  int color = borders_data.color_left_;
  float border_width = borders_data.width_left_;

  Paint paint;
  paint.setColor(Color(color));
  // Activate antialiasing here to ensure that Skia enters the fast path
  paint.setAntiAlias(true);
  paint.setDrawStyle(DrawStyle::kStroke);
  paint.setStrokeWidth(border_width);
  if (!borders_data.HasBorderRadius()) {
    FloatRect stroke_rect(rect);
    stroke_rect.Inflate(-border_width / 2.f);
    context->DrawRect(stroke_rect, paint);
  } else {
    auto get_rect = [&](float radii_offset) -> skity::RRect {
      FloatRect stroke_rect(rect);
      stroke_rect.Inflate(-radii_offset);
      skity::RRect rrect;
      SetRRectRadii(rrect, stroke_rect, borders_data, radii_offset);
      return rrect;
    };
    skity::RRect rrect = get_rect(border_width * 0.5f);
    context->DrawRRect(rrect, paint);
  }
}

static void PaintDoubleBorder(GraphicsCanvas* context, const FloatRect& rect,
                              const BordersData& borders_data) {
  int color = borders_data.color_left_;
  float border_width = borders_data.width_left_;
  BorderSide double_side(border_width, Color(color), BorderStyleType::kDouble);
  float outer_width, inner_width = 0;
  double_side.GetDoubleBorderStripeWidths(outer_width, inner_width);

  skity::Rect outer_rect =
      skity::Rect::MakeXYWH(rect.x(), rect.y(), rect.width(), rect.height());
  skity::Rect inner_rect = skity::Rect::MakeXYWH(
      rect.x() + border_width, rect.y() + border_width,
      rect.width() - border_width * 2, rect.height() - border_width * 2);

  skity::Rect inner_third_rect = skity::Rect::MakeXYWH(
      rect.x() + inner_width, rect.y() + inner_width,
      rect.width() - inner_width * 2, rect.height() - inner_width * 2);
  Paint paint;
  paint.setDrawStyle(DrawStyle::kStroke);
  paint.setColor(Color(color));

  skity::Rect draw_outer_rect = outer_rect;
  draw_outer_rect.Inset(outer_width / 2);
  float inner_stroke_width = inner_rect.X() - inner_third_rect.X();
  skity::Rect draw_inner_rect = inner_third_rect;
  draw_inner_rect.Inset(inner_stroke_width / 2);
  if (!borders_data.HasBorderRadius()) {
    paint.setStrokeWidth(outer_width);
    context->DrawRect(draw_outer_rect, paint);

    paint.setStrokeWidth(inner_stroke_width);
    context->DrawRect(draw_inner_rect, paint);
  } else {
#if defined(ENABLE_ANTIALIAS)
    paint.setAntiAlias(true);
#endif
    skity::RRect round_outer_rect;
    SetRRectRadii(round_outer_rect, draw_outer_rect, borders_data,
                  outer_width / 2.f);
    paint.setStrokeWidth(outer_width);
    context->DrawRRect(round_outer_rect, paint);

    skity::RRect round_inner_rect;
    SetRRectRadii(round_inner_rect, draw_inner_rect, borders_data,
                  inner_width + inner_stroke_width / 2.f);
    paint.setStrokeWidth(inner_stroke_width);
    context->DrawRRect(round_inner_rect, paint);
  }
}

static bool PaintSimpleSameBorder(GraphicsCanvas* context,
                                  const FloatRect& border_rect,
                                  const BordersData& borders_data) {
  if (borders_data.style_left_ == BorderStyleType::kSolid) {
    PaintSolidBorder(context, border_rect, borders_data);
    return true;
  } else if (borders_data.style_left_ == BorderStyleType::kDouble) {
    PaintDoubleBorder(context, border_rect, borders_data);
    return true;
  }

  return false;
}

void BorderPainter::SetOutline(const OutlineData& outline_data,
                               const FloatRect& box_rect) {
  float width_offset = outline_data.width_ + outline_data.offset_;
  FloatRect outline_outer_rect(box_rect);
  outline_outer_rect.Expand(width_offset, width_offset, width_offset,
                            width_offset);
  outline_outer_rounded_rect_.SetRect(outline_outer_rect);
  FloatRect outline_inner_rect(box_rect);
  outline_inner_rect.Expand(outline_data.offset_, outline_data.offset_,
                            outline_data.offset_, outline_data.offset_);
  outline_inner_rounded_rect_.SetRect(outline_inner_rect);

  outline_box_sides_[BorderSide::TOP] =
      BorderSide(outline_data.width_, Color(outline_data.color_),
                 BorderSide::TOP, outline_data.style_);
  outline_box_sides_[BorderSide::RIGHT] =
      BorderSide(outline_data.width_, Color(outline_data.color_),
                 BorderSide::RIGHT, outline_data.style_);
  outline_box_sides_[BorderSide::BOTTOM] =
      BorderSide(outline_data.width_, Color(outline_data.color_),
                 BorderSide::BOTTOM, outline_data.style_);
  outline_box_sides_[BorderSide::LEFT] =
      BorderSide(outline_data.width_, Color(outline_data.color_),
                 BorderSide::LEFT, outline_data.style_);
}

void BorderPainter::SetBorder(const BordersData& borders_data,
                              const FloatRect& box_rect) {
  float left_width = borders_data.width_left_;
  float top_width = borders_data.width_top_;
  float right_width = borders_data.width_right_;
  float bottom_width = borders_data.width_bottom_;
  FloatRect inner_rect(box_rect);
  inner_rect.Expand(-top_width, -right_width, -bottom_width, -left_width);
  inner_rounded_rect_.SetRect(inner_rect);
  if (borders_data.HasBorderRadius()) {
    FloatRoundedRect::Radii radii(
        FloatSize(borders_data.radius_x_top_left_,
                  borders_data.radius_y_top_left_),
        FloatSize(borders_data.radius_x_top_right_,
                  borders_data.radius_y_top_right_),
        FloatSize(borders_data.radius_x_bottom_left_,
                  borders_data.radius_y_bottom_left_),
        FloatSize(borders_data.radius_x_bottom_right_,
                  borders_data.radius_y_bottom_right_));
    outer_rounded_rect_.SetRadii(radii);
    outer_rounded_rect_.ConstraintRadii();
    FloatRoundedRect::Radii inner_rect_radii = outer_rounded_rect_.radii();
    inner_rect_radii.Shrink(top_width, right_width, bottom_width, left_width);
    inner_rounded_rect_.SetRadii(inner_rect_radii);
    inner_rounded_rect_.ConstraintRadii();
  } else {
    outer_rounded_rect_.SetRadii({});
    inner_rounded_rect_.SetRadii({});
  }
  box_sides_[BorderSide::TOP] =
      BorderSide(top_width, Color(borders_data.color_top_), BorderSide::TOP,
                 borders_data.style_top_);
  box_sides_[BorderSide::RIGHT] =
      BorderSide(right_width, Color(borders_data.color_right_),
                 BorderSide::RIGHT, borders_data.style_right_);
  box_sides_[BorderSide::BOTTOM] =
      BorderSide(bottom_width, Color(borders_data.color_bottom_),
                 BorderSide::BOTTOM, borders_data.style_bottom_);
  box_sides_[BorderSide::LEFT] =
      BorderSide(left_width, Color(borders_data.color_left_), BorderSide::LEFT,
                 borders_data.style_left_);
}

void BorderPainter::PaintBorders(GraphicsCanvas* context, const FloatRect& rect,
                                 const BordersData& borders_data) const {
  const bool needs_clip = IsBorderClip();
  GraphicsContext::AutoRestore saver(context, needs_clip);
  if (needs_clip) {
    skity::RRect clip_rect;
    if (borders_data.HasBorderRadius()) {
      SetRRectRadii(clip_rect, rect, borders_data);
    } else {
      clip_rect.SetRect(rect);
    }
    context->ClipRRect(clip_rect, GrClipOp::kIntersect, true);
  }
  if (borders_data.HasSameWidth() && borders_data.HasSameStyle() &&
      borders_data.HasSameColor() && borders_data.width_top_ != 0) {
    // Notice: we should make sure in each corner, x-radius equals to
    // y-radius(HasSimpleRadii) and greater than or equal to border-width. But
    // no need to promise all corners are same(HasSameRadii) with this simple
    // paint method. They are two different cases.
    if (!borders_data.HasBorderRadius() ||
        (borders_data.HasSimpleRadii() &&
         borders_data.radius_x_top_left_ >= borders_data.width_top_ &&
         borders_data.radius_x_top_right_ >= borders_data.width_top_ &&
         borders_data.radius_x_bottom_left_ >= borders_data.width_top_ &&
         borders_data.radius_x_bottom_right_ >= borders_data.width_top_)) {
      if (PaintSimpleSameBorder(context, rect, borders_data)) {
        return;
      }
    }
  }
  if (borders_data.HasBorderWidth() && borders_data.HasBorderRadius() &&
      CheckSameColorAndSolidBorder()) {
    PaintSameColorAndSolidBorder(context);
    return;
  }
  PaintBorderSide(context, rect, borders_data);
}

void BorderPainter::PaintOutline(GraphicsCanvas* context,
                                 const OutlineData& outline_data) const {
  GraphicsContext::AutoRestore saver(context, true);
  BordersData borders_data;
  borders_data.width_top_ = borders_data.width_right_ =
      borders_data.width_bottom_ = borders_data.width_left_ =
          outline_data.width_;
  borders_data.color_top_ = borders_data.color_right_ =
      borders_data.color_bottom_ = borders_data.color_left_ =
          outline_data.color_;
  borders_data.style_top_ = borders_data.style_right_ =
      borders_data.style_bottom_ = borders_data.style_left_ =
          outline_data.style_;

  FloatRect rect = outline_outer_rounded_rect_.rect();
  // (0,0) is border_box location at top left
  // outline is out of the border_box
  rect.SetX(0 - borders_data.width_left_);
  rect.SetY(0 - borders_data.width_top_);
  if (outline_data.style_ == BorderStyleType::kDashed ||
      outline_data.style_ == BorderStyleType::kDotted) {
    context->ClipRect(rect, GrClipOp::kIntersect, true);
  }
  PaintBorderSide(context, rect, borders_data, true);
}

bool BorderPainter::CheckSameColorAndSolidBorder() const {
  bool flag = false;
  for (size_t i = 0; i < 4; ++i) {
    if (box_sides_[i].width_ != 0 &&
        box_sides_[i].style_ == BorderStyleType::kSolid) {
      flag = true;
      for (size_t k = 0; k < 4; ++k) {
        if (k != i && box_sides_[k].width_ != 0 &&
            (box_sides_[i].style_ != box_sides_[k].style_ ||
             box_sides_[i].color_ != box_sides_[k].color_)) {
          return false;
        }
      }
      break;
    }
  }
  return flag;
}

void BorderPainter::PaintSameColorAndSolidBorder(
    GraphicsCanvas* context) const {
  Color valid_color;
  BorderStyleType valid_style;
  for (auto& box_side : box_sides_) {
    if (box_side.width_ != 0) {
      valid_color = box_side.color_;
      valid_style = box_side.style_;
      FML_DCHECK(valid_style == BorderStyleType::kSolid);
      break;
    }
  }
  GraphicsContext::AutoRestore saver(context, true);
  const FloatPoint& offset = origin_;
  context->Translate(-offset.x(), -offset.y());

  class Paint paint;
  // Activate antialiasing here to ensure that Skia enters the fast path
  paint.setAntiAlias(true);
  paint.setColor(valid_color);

  paint.setDrawStyle(DrawStyle::kFill);
  context->DrawDRRect(outer_rounded_rect_, inner_rounded_rect_, paint);
}

void BorderPainter::PaintBorderSide(GraphicsCanvas* context,
                                    const FloatRect& border_rect,
                                    const BordersData& borders_data,
                                    bool is_outline) const {
  const auto& sides = is_outline ? outline_box_sides_ : box_sides_;
  for (const auto& side : sides) {
    if (side.Visible()) {
      PaintSide(context, border_rect, side, borders_data, is_outline);
    }
  }
}

void BorderPainter::PaintSide(GraphicsCanvas* context,
                              const FloatRect& border_rect,
                              const BorderSide& side,
                              const BordersData& borders_data,
                              bool is_outline) const {
  if (side.width_ <= 0) {
    return;
  }

  GrPath path;
  skity::RRect round_rect = outer_rounded_rect_;
  if (is_outline) {
    round_rect = outline_outer_rounded_rect_;
  }
  bool use_path =
      !(borders_data.HasSameWidth() && borders_data.HasSameColor() &&
        borders_data.HasSameStyle() && !borders_data.HasBorderRadius() &&
        (side.style_ == BorderStyleType::kSolid ||
         side.style_ == BorderStyleType::kDouble));

  if ((side.style_ == BorderStyleType::kDashed ||
       side.style_ == BorderStyleType::kDotted) &&
      !borders_data.HasBorderRadius()) {
    // non-RRect dashed/dotted shoule be painted by lines
    use_path = false;
  }
  if (is_outline) {
    if (side.style_ == BorderStyleType::kDashed ||
        side.style_ == BorderStyleType::kDotted) {
      use_path = false;
    } else {
      use_path = true;
    }
  }

  switch (side.side_index_) {
    case BorderSide::TOP: {
      FloatRect side_rect;
      if (use_path) {
        PATH_ADD_RRECT(path, round_rect);
      } else {
        side_rect = FloatRect(border_rect.x(), border_rect.y(),
                              border_rect.width(), side.width_);
      }

      if (is_outline) {
        PaintOneBorderSide(context, side_rect, side,
                           outline_box_sides_[BorderSide::LEFT],
                           outline_box_sides_[BorderSide::RIGHT], path,
                           borders_data, is_outline);
      } else {
        PaintOneBorderSide(
            context, side_rect, side, box_sides_[BorderSide::LEFT],
            box_sides_[BorderSide::RIGHT], path, borders_data, is_outline);
      }
      break;
    }
    case BorderSide::BOTTOM: {
      FloatRect side_rect;
      if (use_path) {
        PATH_ADD_RRECT(path, round_rect);
      } else {
        side_rect = FloatRect(border_rect.x(), border_rect.MaxY() - side.width_,
                              border_rect.width(), side.width_);
      }

      if (is_outline) {
        PaintOneBorderSide(context, side_rect, side,
                           outline_box_sides_[BorderSide::LEFT],
                           outline_box_sides_[BorderSide::RIGHT], path,
                           borders_data, is_outline);
      } else {
        PaintOneBorderSide(
            context, side_rect, side, box_sides_[BorderSide::LEFT],
            box_sides_[BorderSide::RIGHT], path, borders_data, is_outline);
      }
      break;
    }
    case BorderSide::RIGHT: {
      FloatRect side_rect;
      if (use_path) {
        PATH_ADD_RRECT(path, round_rect);
      } else {
        side_rect = FloatRect(border_rect.MaxX() - side.width_, border_rect.y(),
                              side.width_, border_rect.height());
      }

      if (is_outline) {
        PaintOneBorderSide(context, side_rect, side,
                           outline_box_sides_[BorderSide::TOP],
                           outline_box_sides_[BorderSide::BOTTOM], path,
                           borders_data, is_outline);
      } else {
        PaintOneBorderSide(
            context, side_rect, side, box_sides_[BorderSide::TOP],
            box_sides_[BorderSide::BOTTOM], path, borders_data, is_outline);
      }
      break;
    }
    case BorderSide::LEFT: {
      FloatRect side_rect;
      if (use_path) {
        PATH_ADD_RRECT(path, round_rect);
      } else {
        side_rect = FloatRect(border_rect.x(), border_rect.y(), side.width_,
                              border_rect.height());
      }
      if (is_outline) {
        PaintOneBorderSide(context, side_rect, side,
                           outline_box_sides_[BorderSide::TOP],
                           outline_box_sides_[BorderSide::BOTTOM], path,
                           borders_data, is_outline);
      } else {
        PaintOneBorderSide(
            context, side_rect, side, box_sides_[BorderSide::TOP],
            box_sides_[BorderSide::BOTTOM], path, borders_data, is_outline);
      }
      break;
    }
    default:
      break;
  }
}

void BorderPainter::PaintOneBorderSide(
    GraphicsCanvas* context, const FloatRect& side_rect, const BorderSide& side,
    const BorderSide& relative_side1, const BorderSide& relative_side2,
    const GrPath& path, const BordersData& borders_data,
    bool is_outline) const {
  bool should_aa1 = relative_side1.style_ != BorderStyleType::kNone &&
                    relative_side1.style_ != BorderStyleType::kHide;
  bool should_aa2 = relative_side2.style_ != BorderStyleType::kNone &&
                    relative_side2.style_ != BorderStyleType::kHide;

  if (side.style_ == BorderStyleType::kInset ||
      side.style_ == BorderStyleType::kOutset) {
    should_aa1 = true;
    should_aa2 = true;
  }
  if (!PATH_IS_EMPTY(path)) {
    GraphicsContext::AutoRestore saver(context, true);
    const FloatPoint& offset = origin_;
    context->Translate(-offset.x(), -offset.y());
    // clip to get the real border area
    if (!is_outline) {
      context->ClipRRect(outer_rounded_rect_, GrClipOp::kIntersect, true);
      if (inner_rounded_rect_.IsRenderable() &&
          !inner_rounded_rect_.IsEmpty()) {
        context->ClipRRect(inner_rounded_rect_, GrClipOp::kDifference, true);
      }
    }
    if (is_outline) {
      ClipBorderSidePolygon(context, side, should_aa1, should_aa2, true);
    } else {
      if (inner_rounded_rect_.IsRenderable()) {
        ClipBorderSidePolygon(context, side, should_aa1, should_aa2);
      } else {
        ClipBorderSideForComplexInnerPath(context, side, borders_data);
      }
    }
    if (is_outline) {
      DrawBorderSideFromPath(context, outline_outer_rounded_rect_.rect(), path,
                             side.width_, side, side.color_, side.style_, true);
    } else {
      DrawBorderSideFromPath(context, outer_rounded_rect_.rect(), path,
                             side.width_, side, side.color_, side.style_);
    }

  } else if (side.style_ == BorderStyleType::kDashed ||
             side.style_ == BorderStyleType::kDotted) {
    // dashed and dotted will be drawn with help of DashEffectPath
    GraphicsContext::AutoRestore saver(context, true);
    const FloatPoint& offset = origin_;
    context->Translate(-offset.x(), -offset.y());
    ClipBorderSidePolygon(context, side, should_aa1, should_aa2, is_outline);
    if (is_outline) {
      DrawLineForDashed(context, outline_outer_rounded_rect_.rect(),
                        side.width_, side, side.color_, side.style_);
    } else {
      DrawLineForDashed(context, outer_rounded_rect_.rect(), side.width_, side,
                        side.color_, side.style_);
    }
  } else {
    // it seems that logic can not jump to this branch
    BorderPainter::DrawLineForBoxSide(
        context, side_rect.x(), side_rect.y(), side_rect.MaxX(),
        side_rect.MaxY(), side.side_index_, side.color_, side.style_,
        relative_side1.width_, relative_side2.width_, true);
  }
}

void BorderPainter::DrawLineForDashed(GraphicsCanvas* context,
                                      const FloatRect& side_rect,
                                      float thickness, const BorderSide& side,
                                      Color color,
                                      BorderStyleType border_style) const {
  class Paint paint;
#if defined(ENABLE_ANTIALIAS)
  paint.setAntiAlias(true);
#endif
  paint.setColor(color);
  // The stroke is doubled here because the provided path is the
  // outside edge of the border so half the stroke is clipped off.
  // The extra multiplier is so that the clipping mask can antialias
  // the edges to prevent jaggies.
  paint.setStrokeWidth(thickness * 2.0f);
  float dash_x1 = 0.f, dash_x2 = 0.f, dash_y1 = 0.f, dash_y2 = 0.f;
  auto getDashPathEffect = [&border_style](float border_width,
                                           float border_length) {
    float section_length = (border_width >= 1 ? border_width : 1) *
                           (border_style == BorderStyleType::kDotted ? 2 : 6) *
                           0.5f;
    int new_section_count =
        (static_cast<int>((border_length / section_length - 0.5f) * 0.5f)) * 2 +
        1;
    float intervals[] = {border_length / new_section_count,
                         border_length / new_section_count};
    std::shared_ptr<PathEffect> ret = DashPathEffect::Make(intervals, 2, 0);
    return ret;
  };
  float length = 0.f;
  switch (side.side_index_) {
    case BorderSide::TOP: {
      length = side_rect.width();
      dash_x1 = side_rect.x();
      dash_x2 = side_rect.MaxX();
      dash_y1 = side_rect.y();
      dash_y2 = side_rect.y();
      break;
    }
    case BorderSide::BOTTOM: {
      length = side_rect.width();
      dash_x1 = side_rect.x();
      dash_x2 = side_rect.MaxX();
      dash_y1 = side_rect.MaxY();
      dash_y2 = side_rect.MaxY();
      break;
    }
    case BorderSide::LEFT: {
      length = side_rect.height();
      dash_x1 = side_rect.x();
      dash_x2 = side_rect.x();
      dash_y1 = side_rect.y();
      dash_y2 = side_rect.MaxY();
      break;
    }
    case BorderSide::RIGHT: {
      length = side_rect.height();
      dash_x1 = side_rect.MaxX();
      dash_x2 = side_rect.MaxX();
      dash_y1 = side_rect.y();
      dash_y2 = side_rect.MaxY();
      break;
    }
  }
  auto ret = getDashPathEffect(thickness, length);
  if (ret) {
    paint.setPathEffect(ret);
    context->DrawLine(dash_x1, dash_y1, dash_x2, dash_y2, paint);
  }
}

void BorderPainter::SetupPaintDashPathEffect(
    class Paint& paint, const int length, const float border_thickness,
    const BorderStyleType border_style) {
  auto SelectBestDashGap = [](float stroke_length, float dash_length,
                              float gap_length) {
    // Determine what number of dashes gives the minimum deviation
    // from gapLength between dashes. Set the gap to that width.
    float min_num_dashes =
        floorf((stroke_length + gap_length) / (dash_length + gap_length));
    float max_num_dashes = min_num_dashes + 1;
    float min_gap =
        (stroke_length - min_num_dashes * dash_length) / (min_num_dashes - 1);
    float max_gap =
        (stroke_length - max_num_dashes * dash_length) / (max_num_dashes - 1);
    return (max_gap <= 0) ||
                   (fabs(min_gap - gap_length) < fabs(max_gap - gap_length))
               ? min_gap
               : max_gap;
  };
  float dash_width = border_thickness;
  if (border_style == BorderStyleType::kDashed) {
    float dash_length = dash_width;
    float gap_length = dash_length;
    // gap_ratio and dash_ratio can be changed to support
    // different render logics
    float gap_ratio = 3.f, dash_ratio = 3.f;
    dash_length *= dash_ratio;
    gap_length *= gap_ratio;
    if (length <= 2 * dash_length + gap_length) {
      // set end dashes properly by shrink both dash and gap length
      float multiplier = length / (2 * dash_length + gap_length);
      float intervals[2] = {dash_length * multiplier, gap_length * multiplier};
      paint.setPathEffect(DashPathEffect::Make(intervals, 2, 0));
    } else {
      float gap = SelectBestDashGap(length, dash_length, gap_length);
      float intervals[2] = {dash_length, gap};
      paint.setPathEffect(DashPathEffect::Make(intervals, 2, 0));
    }
  } else if (border_style == BorderStyleType::kDotted) {
    // Adjust the width to get equal dot spacing as much as possible.
    float per_dot_length = dash_width * 2;
    if (length < per_dot_length) {
      // Not enough space for 2 dots. Just draw 1 by giving a gap that is
      // bigger than the length.
      float intervals[2] = {static_cast<float>(dash_width), per_dot_length};
      paint.setPathEffect(DashPathEffect::Make(intervals, 2, 0));
      return;
    }
    // Epsilon ensures that we get a whole dot at the end of the line,
    // even if that dot is a little inside the true endpoint. Without it
    // we can drop the end dot due to rounding along the line.
    static const float kEpsilon = 1.0e-2f;
    float gap = SelectBestDashGap(length, dash_width, dash_width);
    float intervals[2] = {static_cast<float>(dash_width), gap - kEpsilon};
    paint.setPathEffect(DashPathEffect::Make(intervals, 2, 0));
  }
}

void BorderPainter::DrawBorderSideFromPath(
    GraphicsCanvas* context, const FloatRect& border_rect,
    const GrPath& border_path, float thickness, const BorderSide& side,
    Color color, BorderStyleType border_style, bool is_outline) const {
  if (thickness <= 0) {
    return;
  }

  if (border_style == BorderStyleType::kDouble && thickness < 3) {
    border_style = BorderStyleType::kSolid;
  }

  switch (border_style) {
    case BorderStyleType::kNone:
    case BorderStyleType::kHide:
    case BorderStyleType::kUndefined:
      return;
    case BorderStyleType::kDotted:
    case BorderStyleType::kDashed: {
      GraphicsContext::AutoRestore saver(context, true);
      class Paint paint;
#if defined(ENABLE_ANTIALIAS)
      paint.setAntiAlias(true);
#endif
      paint.setColor(color);
      paint.setDrawStyle(DrawStyle::kStroke);
      // The stroke is doubled here because the provided path is the
      // outside edge of the border so half the stroke is clipped off.
      // The extra multiplier is so that the clipping mask can antialias
      // the edges to prevent jaggies.
      paint.setStrokeWidth(side.width_ * 2.0f);
      int length =
          static_cast<int>(GraphicsContext::GetPathLength(border_path));
      SetupPaintDashPathEffect(paint, length, side.width_, border_style);
      context->DrawPath(border_path, paint);
      return;
    }
    case BorderStyleType::kSolid: {
      GraphicsContext::AutoRestore saver(context, true);
      class Paint paint;
#if defined(ENABLE_ANTIALIAS)
      paint.setAntiAlias(true);
#endif
      paint.setDrawStyle(DrawStyle::kFill);
      paint.setColor(color);
      if (is_outline) {
        context->DrawDRRect(outline_outer_rounded_rect_,
                            outline_inner_rounded_rect_, paint);
      } else {
        context->DrawRRect(outer_rounded_rect_, paint);
      }

      return;
    }
    case BorderStyleType::kDouble: {
      {
        GraphicsContext::AutoRestore saver(context, true);
        FloatRect inner_rect = outer_rounded_rect_.rect();
        inner_rect.Expand(
            -box_sides_[0].InnerWidth(), -box_sides_[1].InnerWidth(),
            -box_sides_[2].InnerWidth(), -box_sides_[3].InnerWidth());
        FloatRoundedRect::Radii radii = outer_rounded_rect_.radii();
        if (!radii.IsZero()) {
          radii.Shrink(box_sides_[0].InnerWidth(), box_sides_[1].InnerWidth(),
                       box_sides_[2].InnerWidth(), box_sides_[3].InnerWidth());
        }

        if (is_outline) {
          inner_rect = outline_outer_rounded_rect_.rect();
          inner_rect.Expand(-outline_box_sides_[0].InnerWidth(),
                            -outline_box_sides_[1].InnerWidth(),
                            -outline_box_sides_[2].InnerWidth(),
                            -outline_box_sides_[3].InnerWidth());
          radii = outline_outer_rounded_rect_.radii();
        }

        FloatRoundedRect inner_clip(inner_rect, radii);
        if (inner_clip.IsRounded()) {
          context->ClipRRect(inner_clip, GrClipOp::kIntersect, true);
        } else {
          context->ClipRect(inner_clip.rect(), GrClipOp::kIntersect, true);
        }
        DrawBorderSideFromPath(context, border_rect, border_path, thickness,
                               side, color, BorderStyleType::kSolid,
                               is_outline);
      }

      {
        GraphicsContext::AutoRestore saver(context, true);
        FloatRect outer_rect = outer_rounded_rect_.rect();
        outer_rect.Expand(
            -box_sides_[0].OuterWidth(), -box_sides_[1].OuterWidth(),
            -box_sides_[2].OuterWidth(), -box_sides_[3].OuterWidth());
        FloatRoundedRect::Radii radii = outer_rounded_rect_.radii();
        if (!radii.IsZero()) {
          radii.Shrink(box_sides_[0].OuterWidth(), box_sides_[1].OuterWidth(),
                       box_sides_[2].OuterWidth(), box_sides_[3].OuterWidth());
        }

        if (is_outline) {
          outer_rect = outline_outer_rounded_rect_.rect();
          outer_rect.Expand(-outline_box_sides_[0].OuterWidth(),
                            -outline_box_sides_[1].OuterWidth(),
                            -outline_box_sides_[2].OuterWidth(),
                            -outline_box_sides_[3].OuterWidth());
          radii = outline_outer_rounded_rect_.radii();
        }

        FloatRoundedRect outer_clip(outer_rect, radii);
        if (outer_clip.IsRounded()) {
          context->ClipRRect(outer_clip, GrClipOp::kDifference, true);
        } else {
          context->ClipRect(outer_clip.rect(), GrClipOp::kDifference, true);
        }

        DrawBorderSideFromPath(context, border_rect, border_path, thickness,
                               side, color, BorderStyleType::kSolid,
                               is_outline);
      }
      return;
    }
    case BorderStyleType::kRidge:
    case BorderStyleType::kGroove: {
      BorderStyleType s1;
      BorderStyleType s2;
      if (border_style == BorderStyleType::kGroove) {
        s1 = BorderStyleType::kInset;
        s2 = BorderStyleType::kOutset;
      } else {
        s1 = BorderStyleType::kOutset;
        s2 = BorderStyleType::kInset;
      }

      float top_width = (box_sides_[0].width_) / 2;
      float right_width = (box_sides_[1].width_) / 2;
      float bottom_width = (box_sides_[2].width_) / 2;
      float left_width = (box_sides_[3].width_) / 2;

      if (is_outline) {
        top_width = (outline_box_sides_[0].width_) / 2;
        right_width = (outline_box_sides_[1].width_) / 2;
        bottom_width = (outline_box_sides_[2].width_) / 2;
        left_width = (outline_box_sides_[3].width_) / 2;
      }

      // Paint outer only
      {
        GraphicsContext::AutoRestore saver(context, true);

        FloatRect inner_rect(border_rect);
        inner_rect.Expand(-top_width, -right_width, -bottom_width, -left_width);
        FloatRoundedRect::Radii radii = outer_rounded_rect_.radii();
        if (is_outline) {
          radii = outline_outer_rounded_rect_.radii();
        }
        if (!radii.IsZero()) {
          radii.Shrink(top_width, right_width, bottom_width, left_width);
        }
        FloatRoundedRect inner_clip(inner_rect, radii);
        if (inner_clip.IsRounded()) {
          context->ClipRRect(inner_clip, GrClipOp::kDifference, true);
        } else {
          context->ClipRect(inner_clip.rect(), GrClipOp::kDifference, false);
        }
        DrawBorderSideFromPath(context, border_rect, border_path, thickness,
                               side, color, s1, is_outline);
      }

      // Paint inner only
      {
        GraphicsContext::AutoRestore saver(context, true);

        FloatRect inner_rect(border_rect);
        inner_rect.Expand(-top_width, -right_width, -bottom_width, -left_width);
        FloatRoundedRect::Radii radii = outer_rounded_rect_.radii();
        if (is_outline) {
          radii = outline_outer_rounded_rect_.radii();
        }
        if (!radii.IsZero()) {
          radii.Shrink(top_width, right_width, bottom_width, left_width);
        }
        FloatRoundedRect inner_clip(inner_rect, radii);
        if (inner_clip.IsRounded()) {
          context->ClipRRect(inner_clip, GrClipOp::kIntersect, true);
        } else {
          context->ClipRect(inner_clip.rect(), GrClipOp::kIntersect, false);
        }
        DrawBorderSideFromPath(context, border_rect, border_path, thickness,
                               side, color, s2, is_outline);
      }
      break;
    }
    case BorderStyleType::kInset:
      if (side.side_index_ == BorderSide::TOP ||
          side.side_index_ == BorderSide::LEFT) {
        color = color.Dark();
      }
      DrawBorderSideFromPath(context, border_rect, border_path, thickness, side,
                             color, BorderStyleType::kSolid, is_outline);
      break;
    case BorderStyleType::kOutset:
      if (side.side_index_ == BorderSide::BOTTOM ||
          side.side_index_ == BorderSide::RIGHT) {
        color = color.Dark();
      }
      DrawBorderSideFromPath(context, border_rect, border_path, thickness, side,
                             color, BorderStyleType::kSolid, is_outline);
      break;
    default:
      break;
  }
}

void BorderPainter::ClipBorderSideForComplexInnerPath(
    GraphicsCanvas* context, const BorderSide& side,
    const BordersData& borders_data) const {
  context->ClipRect(
      CalculateSideRectIncludingInner(outer_rounded_rect_, side, borders_data),
      GrClipOp::kIntersect, false);
  FloatRoundedRect adjusted_inner_rect =
      CalculateAdjustedInnerBorder(inner_rounded_rect_, side);
  if (!adjusted_inner_rect.IsEmpty()) {
    if (!adjusted_inner_rect.IsRounded()) {
      context->ClipRect(adjusted_inner_rect.rect(), GrClipOp::kDifference,
                        true);
      return;
    }
    context->ClipRRect(adjusted_inner_rect, GrClipOp::kDifference, true);
  }
}

void BorderPainter::ClipBorderSidePolygon(GraphicsCanvas* context,
                                          const BorderSide& side,
                                          bool should_aa1, bool should_aa2,
                                          bool is_outline) const {
  FloatPoint quad[4];

  FloatRect outer_rect(outer_rounded_rect_.rect());
  FloatRect inner_rect(inner_rounded_rect_.rect());
  if (is_outline) {
    outer_rect = outline_outer_rounded_rect_.rect();
    inner_rect = outline_inner_rounded_rect_.rect();
  }

  // For each side, create a quad that encompasses all parts of that side that
  // may draw, including areas inside the innerBorder.
  //
  //         0----------------3
  //       0  \              /  0
  //       |\  1----------- 2  /|
  //       | 1                1 |
  //       | |                | |
  //       | |                | |
  //       | 2                2 |
  //       |/  1------------2  \|
  //       3  /              \  3
  //         0----------------3
  //
  switch (side.side_index_) {
    case BorderSide::TOP:
      quad[0] = outer_rect.MinXMinYCorner();
      quad[1] = inner_rect.MinXMinYCorner();
      quad[2] = inner_rect.MaxXMinYCorner();
      quad[3] = outer_rect.MaxXMinYCorner();

      if (!is_outline) {
        if (!inner_rounded_rect_.radii().TopLeft().IsZero()) {
          FindIntersection(
              quad[0], quad[1],
              FloatPoint(
                  quad[1].x() + inner_rounded_rect_.radii().TopLeft().width(),
                  quad[1].y()),
              FloatPoint(
                  quad[1].x(),
                  quad[1].y() + inner_rounded_rect_.radii().TopLeft().height()),
              &quad[1]);
        }

        if (!inner_rounded_rect_.radii().TopRight().IsZero()) {
          FindIntersection(
              quad[3], quad[2],
              FloatPoint(
                  quad[2].x() - inner_rounded_rect_.radii().TopRight().width(),
                  quad[2].y()),
              FloatPoint(quad[2].x(),
                         quad[2].y() +
                             inner_rounded_rect_.radii().TopRight().height()),
              &quad[2]);
        }
      }

      break;

    case BorderSide::LEFT:
      quad[0] = (outer_rect.MinXMinYCorner());
      quad[1] = (inner_rect.MinXMinYCorner());
      quad[2] = (inner_rect.MinXMaxYCorner());
      quad[3] = (outer_rect.MinXMaxYCorner());

      if (!is_outline) {
        if (!inner_rounded_rect_.radii().TopLeft().IsZero()) {
          FindIntersection(
              quad[0], quad[1],
              FloatPoint(
                  quad[1].x() + inner_rounded_rect_.radii().TopLeft().width(),
                  quad[1].y()),
              FloatPoint(
                  quad[1].x(),
                  quad[1].y() + inner_rounded_rect_.radii().TopLeft().height()),
              &quad[1]);
        }

        if (!inner_rounded_rect_.radii().BottomLeft().IsZero()) {
          FindIntersection(
              quad[3], quad[2],
              FloatPoint(quad[2].x() +
                             inner_rounded_rect_.radii().BottomLeft().width(),
                         quad[2].y()),
              FloatPoint(quad[2].x(),
                         quad[2].y() -
                             inner_rounded_rect_.radii().BottomLeft().height()),
              &quad[2]);
        }
      }

      break;

    case BorderSide::BOTTOM:
      quad[0] = (outer_rect.MinXMaxYCorner());
      quad[1] = (inner_rect.MinXMaxYCorner());
      quad[2] = (inner_rect.MaxXMaxYCorner());
      quad[3] = (outer_rect.MaxXMaxYCorner());
      if (!is_outline) {
        if (!inner_rounded_rect_.radii().BottomLeft().IsZero()) {
          FindIntersection(
              quad[0], quad[1],
              FloatPoint(quad[1].x() +
                             inner_rounded_rect_.radii().BottomLeft().width(),
                         quad[1].y()),
              FloatPoint(quad[1].x(),
                         quad[1].y() -
                             inner_rounded_rect_.radii().BottomLeft().height()),
              &quad[1]);
        }
        if (!inner_rounded_rect_.radii().BottomRight().IsZero()) {
          FindIntersection(
              quad[3], quad[2],
              FloatPoint(quad[2].x() -
                             inner_rounded_rect_.radii().BottomRight().width(),
                         quad[2].y()),
              FloatPoint(
                  quad[2].x(),
                  quad[2].y() -
                      inner_rounded_rect_.radii().BottomRight().height()),
              &quad[2]);
        }
      }
      break;

    case BorderSide::RIGHT:
      quad[0] = (outer_rect.MaxXMinYCorner());
      quad[1] = (inner_rect.MaxXMinYCorner());
      quad[2] = (inner_rect.MaxXMaxYCorner());
      quad[3] = (outer_rect.MaxXMaxYCorner());

      if (!is_outline) {
        if (!inner_rounded_rect_.radii().TopRight().IsZero()) {
          FindIntersection(
              quad[0], quad[1],
              FloatPoint(
                  quad[1].x() - inner_rounded_rect_.radii().TopRight().width(),
                  quad[1].y()),
              FloatPoint(quad[1].x(),
                         quad[1].y() +
                             inner_rounded_rect_.radii().TopRight().height()),
              &quad[1]);
        }

        if (!inner_rounded_rect_.radii().BottomRight().IsZero()) {
          FindIntersection(
              quad[3], quad[2],
              FloatPoint(quad[2].x() -
                             inner_rounded_rect_.radii().BottomRight().width(),
                         quad[2].y()),
              FloatPoint(
                  quad[2].x(),
                  quad[2].y() -
                      inner_rounded_rect_.radii().BottomRight().height()),
              &quad[2]);
        }
      }

      break;
  }

  if (should_aa1 && should_aa2) {
    GrPath path;
    GraphicsContext::SetPathFromPoints(&path, 4, quad);
    context->ClipPath(path, GrClipOp::kIntersect, false);
    return;
  }
  // If antialiasing settings for the first edge and second edge is different,
  // they have to be addressed separately. We do this by breaking the quad into
  // two parallelograms, made by moving quad[1] and quad[2].
  float ax = quad[1].x() - quad[0].x();
  float ay = quad[1].y() - quad[0].y();
  float bx = quad[2].x() - quad[1].x();
  float by = quad[2].y() - quad[1].y();
  float cx = quad[3].x() - quad[2].x();
  float cy = quad[3].y() - quad[2].y();

  float r1, r2;
  if (RoughlyEqualToZero(bx) && RoughlyEqualToZero(by)) {
    // The quad was actually a triangle.
    r1 = r2 = 1.0f;
  } else {
    r1 = (-ax * by + ay * bx) / (cx * by - cy * bx) + kExtendFill;
    r2 = (-cx * by + cy * bx) / (ax * by - ay * bx) + kExtendFill;
  }

  if (!(should_aa1 || should_aa2)) {
    FloatPoint first_quad[4];
    first_quad[0] = quad[0];
    first_quad[1] = FloatPoint(quad[0].x() - r1 * cx, quad[0].y() - r1 * cy);
    first_quad[2] = FloatPoint(quad[3].x() + r2 * ax, quad[3].y() + r2 * ay);
    first_quad[3] = quad[3];

    GrPath path;
    GraphicsContext::SetPathFromPoints(&path, 4, first_quad);
    context->ClipPath(path, GrClipOp::kIntersect, should_aa1);
    return;
  }

  if (should_aa1) {
    FloatPoint first_quad[4];
    first_quad[0] = quad[0];
    first_quad[1] = quad[1];
    first_quad[2] = FloatPoint(quad[3].x() + r2 * ax, quad[3].y() + r2 * ay);
    first_quad[3] = quad[3];
    GrPath path;
    GraphicsContext::SetPathFromPoints(&path, 4, first_quad);
    context->ClipPath(path, GrClipOp::kIntersect, should_aa1);
  }

  if (should_aa2) {
    FloatPoint second_quad[4];
    second_quad[0] = quad[0];
    second_quad[1] = FloatPoint(quad[0].x() - r1 * cx, quad[0].y() - r1 * cy);
    second_quad[2] = quad[2];
    second_quad[3] = quad[3];

    GrPath path;
    GraphicsContext::SetPathFromPoints(&path, 4, second_quad);
    context->ClipPath(path, GrClipOp::kIntersect, should_aa2);
  }
}

bool BorderPainter::IsBackgroundBleedAvoidanceShrink() const {
  for (auto& side : box_sides_) {
    if (!side.ObscuresBackgroundEdge()) {
      return false;
    }
  }
  return true;
}

FloatRoundedRect BorderPainter::GetBackgroundRoundedRect(
    const FloatRect& paint_rect) const {
  FloatRoundedRect round_rect;
  if (IsBackgroundBleedAvoidanceShrink()) {
    float fractionalInset = 0.5f;
    for (auto& side : box_sides_) {
      if (side.style_ == BorderStyleType::kDouble) {
        fractionalInset = 0.16f;
        break;
      }
    }

    float top_inset = -fractionalInset * (box_sides_[0].width_);
    float right_inset = -fractionalInset * (box_sides_[1].width_);
    float bottom_inset = -fractionalInset * (box_sides_[2].width_);
    float left_inset = -fractionalInset * (box_sides_[3].width_);

    FloatRect inset_rect(paint_rect.x(), paint_rect.y(),
                         outer_rounded_rect_.rect().width(),
                         outer_rounded_rect_.rect().height());
    inset_rect.Expand(top_inset, right_inset, bottom_inset, left_inset);
    FloatRoundedRect::Radii inset_radii(outer_rounded_rect_.radii());
    inset_radii.Expand(top_inset, right_inset, bottom_inset, left_inset);

    round_rect.SetRect(inset_rect);
    round_rect.SetRadii(inset_radii);
  } else {
    FloatRect rect(paint_rect.x(), paint_rect.y(),
                   outer_rounded_rect_.rect().width(),
                   outer_rounded_rect_.rect().height());
    round_rect.SetRect(rect);
    round_rect.SetRadii(outer_rounded_rect_.radii());
  }
  return round_rect;
}

bool BorderPainter::IsBorderClip() const {
  for (auto& side : box_sides_) {
    if (side.style_ == BorderStyleType::kDotted ||
        side.style_ == BorderStyleType::kDashed) {
      return true;
    }
  }
  return false;
}

void BorderPainter::DrawLineForBoxSide(GraphicsCanvas* context, int x1, int y1,
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

void BorderPainter::DrawDashedOrDottedBoxSide(GraphicsCanvas* context, int x1,
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
      GraphicsContext::DrawLine(context, paint, FloatPoint(x1, mid_y),
                                FloatPoint(x2, mid_y), thickness, stroke_style);
      break;
    }
    case BorderSide::RIGHT:
    case BorderSide::LEFT: {
      int mid_x = x1 + thickness / 2;
      GraphicsContext::DrawLine(context, paint, FloatPoint(mid_x, y1),
                                FloatPoint(mid_x, y2), thickness, stroke_style);
      break;
    }
  }
}

void BorderPainter::DrawDoubleBoxSide(GraphicsCanvas* context, int x1, int y1,
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
            skity::Rect::MakeXYWH(x1, y1, length, third_of_thickness), paint);
        context->DrawRect(skity::Rect::MakeXYWH(x1, y2 - third_of_thickness,
                                                length, third_of_thickness),
                          paint);
        break;
      }
      case BorderSide::LEFT:
      case BorderSide::RIGHT: {
        Paint paint;
        paint.setColor(color);
        context->DrawRect(
            skity::Rect::MakeXYWH(x1, y1, third_of_thickness, length), paint);
        context->DrawRect(skity::Rect::MakeXYWH(x2 - third_of_thickness, y1,
                                                third_of_thickness, length),
                          paint);
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

void BorderPainter::DrawRidgeOrGrooveBoxSide(
    GraphicsCanvas* context, int x1, int y1, int x2, int y2,
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

void BorderPainter::DrawSolidBoxSide(GraphicsCanvas* context, int x1, int y1,
                                     int x2, int y2, BorderSide::SideIndex side,
                                     Color color, int adjacent_width1,
                                     int adjacent_width2, bool antialias) {
  if (adjacent_width1 == 0.0 && adjacent_width2 == 0.0) {
    Paint paint;
    paint.setColor(color);
    context->DrawRect(skity::Rect::MakeXYWH(x1, y1, x2 - x1, y2 - y1), paint);
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

  GraphicsContext::FillPolygon(context, 4, quad, color, antialias);
}

}  // namespace clay
