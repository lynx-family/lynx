// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_container.h"

#include <algorithm>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_rounded_rect.h"

namespace clay {
namespace {

#define RADIUS_X(BORDER, POS_Y, POS_X) BORDER.radius_x_##POS_Y##_##POS_X##_
#define RADIUS_Y(BORDER, POS_Y, POS_X) BORDER.radius_y_##POS_Y##_##POS_X##_
#define WIDTH(BORDER, POSITION) BORDER.width_##POSITION##_

#define RADIUS_SIZE(BORDER, POS_Y, POS_X)                                   \
  FloatSize(                                                                \
      std::max(0.f, RADIUS_X(BORDER, POS_Y, POS_X) - WIDTH(BORDER, POS_X)), \
      std::max(0.f, RADIUS_Y(BORDER, POS_Y, POS_X) - WIDTH(BORDER, POS_Y)))

FloatRoundedRect MakeRounded(const FloatRect& rect, const BordersData& border) {
  return FloatRoundedRect(
      rect, RADIUS_SIZE(border, top, left), RADIUS_SIZE(border, top, right),
      RADIUS_SIZE(border, bottom, left), RADIUS_SIZE(border, bottom, right));
}

}  // namespace

const char* RenderContainer::GetName() const { return "RenderContainer"; }

void RenderContainer::Paint(PaintingContext& context,
                            const FloatPoint& offset) {
  if (!CanDisplay()) {
    return;
  }

  RenderBox::Paint(context, offset);
  if (HasClipOrOverflowClip()) {
    auto painter = [this](PaintingContext& ctx,
                          const FloatPoint& layer_offset) {
      PaintChildren(ctx, layer_offset);
    };

    FloatPoint paint_offset = offset + ClipOffset();
    FloatRect clip_rect(paint_offset.x(), paint_offset.y(), ClientWidth(),
                        ClientHeight());
    bool has_radius = false;
    if (Overflow() == CSSProperty::OVERFLOW_X) {
      clip_rect.SetWidth(renderer_->GetFrameSize().width() * 2);
      clip_rect.SetX(clip_rect.x() - renderer_->GetFrameSize().width());
    } else if (Overflow() == CSSProperty::OVERFLOW_Y) {
      clip_rect.SetHeight(renderer_->GetFrameSize().height() * 2);
      clip_rect.SetY(clip_rect.y() - renderer_->GetFrameSize().height());
    } else if (HasBorder() && Border().HasBorderRadius()) {
      has_radius = true;
    }
    if (has_radius) {
      context.PushClipRRect(MakeRounded(clip_rect, Border()), offset, painter);
    } else {
      context.PushClipRect(clip_rect, offset, painter);
    }
  } else {
    PaintChildren(context, offset);
  }
}

}  // namespace clay
