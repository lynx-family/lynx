// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/painter/box_painter.h"

#include "clay/gfx/animation/picture_animation_type.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/style/borders_data.h"
#include "clay/gfx/style/color.h"
#include "clay/gfx/style/outline_data.h"
#include "clay/ui/common/background_data.h"
#include "clay/ui/painter/box_shadow_painter.h"
#include "clay/ui/painter/image_painter.h"
#include "clay/ui/painter/object_painter.h"

namespace clay {

BoxPainter::BoxPainter(RenderBox* object)
    : border_painter_(object->location()), render_object_(object) {
  FloatRect box_rect(render_object_->GetFrameRect());
  SetOuterRect(box_rect);

  if (render_object_->HasOutline()) {
    SetOutline(render_object_->Outline(), box_rect);
  }

  if (render_object_->HasBorder()) {
    SetBorder(render_object_->Border(), box_rect);
  }
}

void BoxPainter::Paint(GraphicsContext* context, const FloatPoint& offset) {
  auto rect = render_object_->GetFrameRect();
  if (rect.IsEmpty()) {
    return;
  }
  // remove left and top
  rect.SetX(0);
  rect.SetY(0);
  PaintBoxDecorationBackground(context, offset, rect);
  if (render_object_->HasDefaultFocusRing()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    FloatRoundedRect round_rect;
    FloatRect float_rect(-num_value::kFocusRingThickness / 2.f,
                         -num_value::kFocusRingThickness / 2.f,
                         border_painter_.OuterRect().rect().width() +
                             num_value::kFocusRingThickness,
                         border_painter_.OuterRect().rect().height() +
                             num_value::kFocusRingThickness);

    FloatRoundedRect::Radii radii = border_painter_.OuterRect().radii();
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
    border_painter_.PaintBorders(context->Canvas(), box_rect,
                                 render_object_->Border());
  }

  if (render_object_->HasOutline()) {
    GraphicsContext::AutoRestore saver(context, true);
    context->Translate(offset.x(), offset.y());
    border_painter_.PaintOutline(context->Canvas(), render_object_->Outline());
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
  const bool has_raster_color_animation =
      render_object_->HasBackgroundColorRasterAnimation();
  if (!has_visible_color && !has_image && !has_raster_color_animation) {
    return;
  }

  bool has_border = render_object_->HasBorder();
  bool has_radius = has_border && render_object_->Border().HasBorderRadius();

  // Keep a transparent color draw operation when the background color is
  // animated on the raster thread. It is the backing operation whose color is
  // updated by the raster animator.
  if (has_visible_color || has_raster_color_animation) {
    class Paint paint;
    paint.setColor(background.background_color);
#if defined(ENABLE_ANTIALIAS)
    // setAntiAlias in case topper transform layer has rotation.
    // INFO(feiyue.1998): disable antialias for background, or e2e test
    // maskWithTransform will fail.
    paint.setAntiAlias(render_object_->CanApplyAA());
#endif
    if (background.clips.size() > 0) {
      FloatRect clip_rect = render_object_->GetFrameRect();
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
      if (clip == ClayBackgroundClipType::kBorderArea) {
        FloatRect padding_rect = render_object_->PaddingRect();
        padding_rect.Move(-offset.x(), -offset.y());
        context->ClipRect(padding_rect, GrClipOp::kDifference, false);
      }
    }
    if (has_raster_color_animation) {
      FML_DCHECK(render_object_->HasValidID());
      paint.setDynamicOpType(DynamicOpType::kSetBackgroundColor);
    }
    if (has_radius) {
      context->DrawRRect(border_painter_.GetBackgroundRoundedRect(paint_rect),
                         paint);
    } else {
      context->DrawRect(paint_rect, paint);
    }
  }
  FloatRoundedRect paint_rrect;
  if (has_radius && has_image) {
    auto rrect = border_painter_.GetBackgroundRoundedRect(paint_rect);
    if (background.images.size() > 1) {
      context->ClipRRect(rrect, GrClipOp::kIntersect, true);
    } else {
      paint_rrect = rrect;
    }
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
        .PaintBackgroundImage(
            context, paint_rect, paint_rrect, background.images[index], origin,
            clip, size_x, size_y, position_x, position_y, repeat_x, repeat_y);
  }
}

}  // namespace clay
