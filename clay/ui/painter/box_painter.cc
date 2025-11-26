// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/box_painter.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/gfx/style/color.h"
#include "clay/gfx/style/outline_data.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/common/utils/floating_comparison.h"
#include "clay/ui/painter/border_side.h"
#include "clay/ui/painter/box_shadow_painter.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/painter/object_painter.h"

namespace clay {

static const float kExtendFill = 1e-2f;

// TODO(zhangxiao.ninja): use or not
#if 0
static FloatRect CalculateSideRect(const FloatRoundedRect& outer_border,
                                   const BorderSide& side) {
    FloatRect side_rect = outer_border.rect();
    float width = side.width_;

    if (side.side_index_ == BorderSide::TOP)
        side_rect.SetHeight(width);
    else if (side.side_index_ == BorderSide::BOTTOM)
        side_rect.ShiftYEdgeTo(side_rect.MaxY() - width);
    else if (side.side_index_ == BorderSide::LEFT)
        side_rect.SetWidth(width);
    else
        side_rect.ShiftXEdgeTo(side_rect.MaxX() - width);

    return side_rect;
}
#endif

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

static void PaintSolidBorder(GraphicsContext* context, const FloatRect& rect,
                             const BordersData& borders_data,
                             bool is_outline = false) {
  int color = borders_data.color_left_;
  float border_width = borders_data.width_left_;

  Paint paint;
  paint.setColor(Color(color));
  if (!borders_data.HasBorderRadius()) {
    paint.setDrawStyle(DrawStyle::kStroke);
    paint.setStrokeWidth(border_width);
    FloatRect stroke_rect(rect);
    stroke_rect.Inflate(-border_width / 2.f);
    context->DrawRect(stroke_rect, paint);
  } else {
    // Activate antialiasing here to ensure that Skia enters the fast path
    paint.setAntiAlias(true);
    paint.setDrawStyle(DrawStyle::kFill);
    auto get_rect = [&](float radii_offset) -> skity::RRect {
      FloatRect stroke_rect(rect);
      stroke_rect.Inflate(-radii_offset);
      skity::RRect rrect;
      SetRRectRadii(rrect, stroke_rect, borders_data, radii_offset);
      return rrect;
    };
    skity::RRect inner_rect = get_rect(border_width);
    skity::RRect outer_rect = get_rect(0.f);
    context->DrawDRRect(outer_rect, inner_rect, paint);
  }
}

static void PaintDoubleBorder(GraphicsContext* context, const FloatRect& rect,
                              const BordersData& borders_data,
                              bool is_outline = false) {
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

BoxPainter::BoxPainter(RenderBox* object) : render_object_(object) {
  FloatRect box_rect(render_object_->GetFrameRect());
  SetOuterRect(box_rect);

  if (render_object_->HasOutline()) {
    SetOutline(render_object_->Outline(), box_rect);
  }

  if (render_object_->HasBorder()) {
    SetBorder(render_object_->Border(), box_rect);
  }
}

void BoxPainter::SetOutline(const OutlineData& outline_data,
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

void BoxPainter::SetBorder(const BordersData& borders_data,
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

void BoxPainter::Paint(GraphicsContext* context, const FloatPoint& offset) {
  auto rect = render_object_->GetFrameRect();
  // remove left and top
  rect.SetX(0);
  rect.SetY(0);
  PaintBoxDecorationBackground(context, offset, rect);
  if (render_object_->HasDefaultFocusRing()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    FloatRoundedRect round_rect;
    FloatRect float_rect(
        -num_value::kFocusRingThickness / 2.f,
        -num_value::kFocusRingThickness / 2.f,
        outer_rounded_rect_.rect().width() + num_value::kFocusRingThickness,
        outer_rounded_rect_.rect().height() + num_value::kFocusRingThickness);

    FloatRoundedRect::Radii radii = outer_rounded_rect_.radii();
    if (!radii.IsZero()) {
      radii.Expand(num_value::kFocusRingThickness / 2.f,
                   num_value::kFocusRingThickness / 2.f,
                   num_value::kFocusRingThickness / 2.f,
                   num_value::kFocusRingThickness / 2.f);
    }
    round_rect.SetRect(float_rect);
    round_rect.SetRadii(radii);
    ObjectPainter::DrawDefaultFocusRing(context, round_rect);
  }
}

// Paint sequence: background -> shadows -> borders -> outline
void BoxPainter::PaintBoxDecorationBackground(GraphicsContext* context,
                                              const FloatPoint& offset,
                                              const FloatRect& box_rect) {
  bool has_border = render_object_->HasBorder();

  skity::Rect draw_rect = skity::Rect::MakeXYWH(
      box_rect.x(), box_rect.y(), box_rect.width(), box_rect.height());
  skity::RRect round_rect;
  bool has_radius = has_border && render_object_->Border().HasBorderRadius();
  if (has_radius) {
    const auto& borders_data = render_object_->Border();
    SetRRectRadii(round_rect, draw_rect, borders_data);
  } else {
    round_rect.SetRect(draw_rect);
  }

  if (render_object_->HasBackground()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());

    PaintBackground(context, render_object_->Background(), box_rect);
  }

  if (render_object_->HasShadow()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    BoxShadowPainter(render_object_).Paint(context, box_rect);
  }

  if (has_border) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    if (IsBorderClip()) {
      context->ClipRRect(round_rect, GrClipOp::kIntersect, true);
    }
    PaintBorders(context, box_rect, render_object_->Border());
  }

  if (render_object_->HasOutline()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    PaintOutline(context, render_object_->Outline());
  }

#ifndef NDEBUG
  if (render_object_->EnableRepaintBoundaryBorders() &&
      render_object_->IsRepaintBoundary()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    // Tag repaint boundary areas for debugging.
    class Paint paint;
    paint.setColor(Color::kBlue());
    paint.setDrawStyle(DrawStyle::kStroke);
    paint.setStrokeWidth(2.f);
    context->DrawRect(box_rect, paint);
  }
#endif  // NDEBUG
}

void BoxPainter::PaintBackground(GraphicsContext* context,
                                 const BackgroundData& background,
                                 const FloatRect& paint_rect) {
  bool has_visible_color = (background.background_color.Alpha() != 0);
  bool has_image = !background.images.empty();
  if (!has_visible_color && !has_image) {
    return;
  }

  bool has_border = render_object_->HasBorder();
  bool has_radius = has_border && render_object_->Border().HasBorderRadius();

  // if background is not transparent, draw color background
  if (has_visible_color) {
    class Paint paint;
    paint.setColor(background.background_color);
    if (background.background_color.Alpha() == 0xff) {
      paint.setBlendMode(BlendMode::kSrc);
    }
#if defined(ENABLE_ANTIALIAS)
    // setAntiAlias in case topper transform layer has rotation.
    // INFO(feiyue.1998): disable antialias for background, or e2e test
    // maskWithTransform will fail.
    paint.setAntiAlias(render_object_->CanApplyAA());
#endif
    if (background.clips.size() > 0) {
      FloatRect clip_rect;
      FloatPoint offset(render_object_->location());
      auto clip = background.clips[0];
      if (clip == ClayBackgroundClipType::kPaddingBox) {
        clip_rect = render_object_->PaddingRect();
      } else if (clip == ClayBackgroundClipType::kContentBox) {
        clip_rect = render_object_->ContentRect();
      }
      // opposite to canvas.Translate() in PaintBoxDecorationBackground().
      clip_rect.Move(-offset.x(), -offset.y());
      if (!clip_rect.IsEmpty()) {
        context->ClipRect(clip_rect, GrClipOp::kIntersect, false);
      }
    }
    if (render_object_->HasBackgroundColorRasterAnimation()) {
      FML_DCHECK(render_object_->HasValidID());
      paint.setDynamicOpType(DynamicOpType::kSetBackgroundColor);
    }
    if (has_radius) {
      context->DrawRRect(GetBackgroundRoundedRect(paint_rect), paint);
    } else {
      context->DrawRect(paint_rect, paint);
    }
  }

  if (has_radius && has_image) {
    auto rrect = GetBackgroundRoundedRect(paint_rect);
    context->ClipRRect(rrect, GrClipOp::kIntersect, true);
  }

  // The background images are drawn on stacking context layers on top of each
  // other. The first layer specified is drawn as if it is closest to the user.
  for (int index = background.images.size() - 1; index >= 0; index--) {
    ClayBackgroundOriginType origin = ClayBackgroundOriginType::kBorderBox;
    if (!background.origins.empty()) {
      size_t origin_index = index % background.origins.size();
      origin = background.origins[origin_index];
    }
    ClayBackgroundClipType clip = ClayBackgroundClipType::kBorderBox;
    if (!background.clips.empty()) {
      size_t clip_index = index % background.clips.size();
      clip = background.clips[clip_index];
    }
    BackgroundSize size_x;
    BackgroundSize size_y;
    if (!background.sizes.empty() && background.sizes.size() >= 2) {
      if (static_cast<size_t>(index) * 2 >= background.sizes.size()) {
        size_x = background.sizes[background.sizes.size() - 2];
        size_y = background.sizes[background.sizes.size() - 1];
      } else {
        size_x = background.sizes[index * 2];
        size_y = background.sizes[index * 2 + 1];
      }
    }
    BackgroundPosition position_x{0.f, ClayPlatformLengthUnit::kPercentage};
    BackgroundPosition position_y{0.f, ClayPlatformLengthUnit::kPercentage};
    if (background.positions.size() >= 2) {
      size_t position_index = index % (background.positions.size() / 2);
      position_x = background.positions[position_index * 2];
      position_y = background.positions[position_index * 2 + 1];
    }
    ClayBackgroundRepeatType repeat_x = ClayBackgroundRepeatType::kRepeat;
    ClayBackgroundRepeatType repeat_y = ClayBackgroundRepeatType::kRepeat;
    if (background.repeats.size() >= 2) {
      size_t repeat_index = index % (background.repeats.size() / 2);
      repeat_x = background.repeats[repeat_index * 2];
      repeat_y = background.repeats[repeat_index * 2 + 1];
    }

    // |paint_rect| was translated with RenderObject's location.
    ImagePainter(render_object_)
        .PaintBackgroundImage(context, paint_rect, background.images[index],
                              origin, clip, size_x, size_y, position_x,
                              position_y, repeat_x, repeat_y);
  }
}

void BoxPainter::PaintBorders(GraphicsContext* context, const FloatRect& rect,
                              const BordersData& borders_data,
                              bool is_outline) {
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
      if (PaintSimpleSameBorder(context, rect, borders_data, is_outline)) {
        return;
      }
    }
  }
  if (!is_outline && borders_data.HasBorderWidth() &&
      borders_data.HasBorderRadius() && CheckSameColorAndSolidBorder()) {
    PaintSameColorAndSolidBorder(context);
    return;
  }

  PaintBorderSide(context, rect, borders_data, is_outline);
}

void BoxPainter::PaintOutline(GraphicsContext* context,
                              const OutlineData& outline_data) {
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

bool BoxPainter::PaintSimpleSameBorder(GraphicsContext* context,
                                       const FloatRect& border_rect,
                                       const BordersData& borders_data,
                                       bool is_outline) {
  if (borders_data.style_left_ == BorderStyleType::kSolid) {
    PaintSolidBorder(context, border_rect, borders_data, is_outline);
    return true;
  } else if (borders_data.style_left_ == BorderStyleType::kDouble) {
    PaintDoubleBorder(context, border_rect, borders_data, is_outline);
    return true;
  }

  return false;
}

bool BoxPainter::CheckSameColorAndSolidBorder() {
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

void BoxPainter::PaintSameColorAndSolidBorder(GraphicsContext* context) {
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
  FloatPoint offset(render_object_->location());
  context->Translate(-offset.x(), -offset.y());

  class Paint paint;
  // Activate antialiasing here to ensure that Skia enters the fast path
  paint.setAntiAlias(true);
  paint.setColor(valid_color);

  paint.setDrawStyle(DrawStyle::kFill);
  context->DrawDRRect(outer_rounded_rect_, inner_rounded_rect_, paint);
}

void BoxPainter::PaintBorderSide(GraphicsContext* context,
                                 const FloatRect& border_rect,
                                 const BordersData& borders_data,
                                 bool is_outline) {
  std::vector<BorderSide> box_sides;
  if (is_outline) {
    // build top side
    if (outline_box_sides_[BorderSide::TOP].Visible()) {
      box_sides.push_back(outline_box_sides_[BorderSide::TOP]);
    }
    // build right side
    if (outline_box_sides_[BorderSide::RIGHT].Visible()) {
      box_sides.push_back(outline_box_sides_[BorderSide::RIGHT]);
    }
    if (outline_box_sides_[BorderSide::BOTTOM].Visible()) {
      box_sides.push_back(outline_box_sides_[BorderSide::BOTTOM]);
    }
    // build left side
    if (outline_box_sides_[BorderSide::LEFT].Visible()) {
      box_sides.push_back(outline_box_sides_[BorderSide::LEFT]);
    }
  } else {
    // build top side
    if (box_sides_[BorderSide::TOP].Visible()) {
      box_sides.push_back(box_sides_[BorderSide::TOP]);
    }
    // build right side
    if (box_sides_[BorderSide::RIGHT].Visible()) {
      box_sides.push_back(box_sides_[BorderSide::RIGHT]);
    }
    if (box_sides_[BorderSide::BOTTOM].Visible()) {
      box_sides.push_back(box_sides_[BorderSide::BOTTOM]);
    }
    // build left side
    if (box_sides_[BorderSide::LEFT].Visible()) {
      box_sides.push_back(box_sides_[BorderSide::LEFT]);
    }
  }

  for (auto box_side : box_sides) {
    PaintSide(context, border_rect, box_side, borders_data, is_outline);
  }
}

void BoxPainter::PaintSide(GraphicsContext* context,
                           const FloatRect& border_rect, const BorderSide& side,
                           const BordersData& borders_data, bool is_outline) {
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
        PaintOneBorderSide(
            context, side_rect, side, outline_box_sides_[BorderSide::LEFT],
            outline_box_sides_[BorderSide::RIGHT], path, is_outline);
      } else {
        PaintOneBorderSide(context, side_rect, side,
                           box_sides_[BorderSide::LEFT],
                           box_sides_[BorderSide::RIGHT], path, is_outline);
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
        PaintOneBorderSide(
            context, side_rect, side, outline_box_sides_[BorderSide::LEFT],
            outline_box_sides_[BorderSide::RIGHT], path, is_outline);
      } else {
        PaintOneBorderSide(context, side_rect, side,
                           box_sides_[BorderSide::LEFT],
                           box_sides_[BorderSide::RIGHT], path, is_outline);
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
        PaintOneBorderSide(
            context, side_rect, side, outline_box_sides_[BorderSide::TOP],
            outline_box_sides_[BorderSide::BOTTOM], path, is_outline);
      } else {
        PaintOneBorderSide(context, side_rect, side,
                           box_sides_[BorderSide::TOP],
                           box_sides_[BorderSide::BOTTOM], path, is_outline);
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
        PaintOneBorderSide(
            context, side_rect, side, outline_box_sides_[BorderSide::TOP],
            outline_box_sides_[BorderSide::BOTTOM], path, is_outline);
      } else {
        PaintOneBorderSide(context, side_rect, side,
                           box_sides_[BorderSide::TOP],
                           box_sides_[BorderSide::BOTTOM], path, is_outline);
      }
      break;
    }
    default:
      break;
  }
}

void BoxPainter::PaintOneBorderSide(GraphicsContext* context,
                                    const FloatRect& side_rect,
                                    const BorderSide& side,
                                    const BorderSide& relative_side1,
                                    const BorderSide& relative_side2,
                                    const GrPath& path, bool is_outline) const {
  bool should_aa1 =
      !relative_side1.Visible() || relative_side1.color_ != side.color_;
  bool should_aa2 =
      !relative_side2.Visible() || relative_side2.color_ != side.color_;
  if (relative_side1.style_ == BorderStyleType::kNone ||
      relative_side1.style_ == BorderStyleType::kHide) {
    should_aa1 = false;
  } else {
    should_aa1 = true;
  }

  if (relative_side2.style_ == BorderStyleType::kNone ||
      relative_side2.style_ == BorderStyleType::kHide) {
    should_aa2 = false;
  } else {
    should_aa2 = true;
  }

  if (side.style_ == BorderStyleType::kInset ||
      side.style_ == BorderStyleType::kOutset) {
    should_aa1 = true;
    should_aa2 = true;
  }
  if (!PATH_IS_EMPTY(path)) {
    GraphicsContext::AutoRestore saver(context, true);
    FloatPoint offset(render_object_->location());
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
        ClipBorderSideForComplexInnerPath(context, side);
      }
    }
    float thickness = std::max(std::max(side.width_, relative_side1.width_),
                               relative_side2.width_);
    if (is_outline) {
      DrawBorderSideFromPath(context, outline_outer_rounded_rect_.rect(), path,
                             side.width_, thickness, side, side.color_,
                             side.style_, true);
    } else {
      DrawBorderSideFromPath(context, outer_rounded_rect_.rect(), path,
                             side.width_, thickness, side, side.color_,
                             side.style_);
    }

  } else if (side.style_ == BorderStyleType::kDashed ||
             side.style_ == BorderStyleType::kDotted) {
    // dashed and dotted will be drawn with help of DashEffectPath
    GraphicsContext::AutoRestore saver(context, true);
    FloatPoint offset(render_object_->location());
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
    ObjectPainter::DrawLineForBoxSide(
        context, side_rect.x(), side_rect.y(), side_rect.MaxX(),
        side_rect.MaxY(), side.side_index_, side.color_, side.style_,
        relative_side1.width_, relative_side2.width_, true);
  }
}

void BoxPainter::DrawLineForDashed(GraphicsContext* context,
                                   const FloatRect& side_rect, float thickness,
                                   const BorderSide& side, Color color,
                                   BorderStyleType border_style,
                                   bool is_outline) const {
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

void BoxPainter::SetupPaintDashPathEffect(
    class Paint& paint, const int length, const float border_thickness,
    const BorderStyleType border_style) const {
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

void BoxPainter::DrawBorderSideFromPath(GraphicsContext* context,
                                        const FloatRect& border_rect,
                                        const GrPath& border_path,
                                        float thickness, float draw_thickness,
                                        const BorderSide& side, Color color,
                                        BorderStyleType border_style,
                                        bool is_outline) const {
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
                               draw_thickness, side, color,
                               BorderStyleType::kSolid, is_outline);
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
                               draw_thickness, side, color,
                               BorderStyleType::kSolid, is_outline);
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
                               draw_thickness, side, color, s1, is_outline);
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
                               draw_thickness, side, color, s2, is_outline);
      }
      break;
    }
    case BorderStyleType::kInset:
      if (side.side_index_ == BorderSide::TOP ||
          side.side_index_ == BorderSide::LEFT) {
        color = color.Dark();
      }
      DrawBorderSideFromPath(context, border_rect, border_path, thickness,
                             draw_thickness, side, color,
                             BorderStyleType::kSolid, is_outline);
      break;
    case BorderStyleType::kOutset:
      if (side.side_index_ == BorderSide::BOTTOM ||
          side.side_index_ == BorderSide::RIGHT) {
        color = color.Dark();
      }
      DrawBorderSideFromPath(context, border_rect, border_path, thickness,
                             draw_thickness, side, color,
                             BorderStyleType::kSolid, is_outline);
      break;
    default:
      break;
  }
}

void BoxPainter::ClipBorderSideForComplexInnerPath(GraphicsContext* context,
                                                   const BorderSide& side,
                                                   bool is_outline) const {
  context->ClipRect(CalculateSideRectIncludingInner(outer_rounded_rect_, side,
                                                    render_object_->Border()),
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

void BoxPainter::ClipBorderSidePolygon(GraphicsContext* context,
                                       const BorderSide& side, bool should_aa1,
                                       bool should_aa2, bool is_outline) const {
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

bool BoxPainter::IsBackgroundBleedAvoidanceShrink() {
  for (auto& side : box_sides_) {
    if (!side.ObscuresBackgroundEdge()) {
      return false;
    }
  }
  return true;
}

FloatRoundedRect BoxPainter::GetBackgroundRoundedRect(
    const FloatRect& paint_rect) {
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

bool BoxPainter::IsBorderClip() {
  for (auto& side : box_sides_) {
    if (side.style_ == BorderStyleType::kDotted ||
        side.style_ == BorderStyleType::kDashed) {
      return true;
    }
  }
  return false;
}

}  // namespace clay
