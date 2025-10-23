// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/render_external_content.h"

#include "clay/fml/logging.h"
#include "clay/ui/compositing/pending_offset_layer.h"
#include "clay/ui/painter/painting_context.h"

namespace clay {

RenderExternalContent::RenderExternalContent() = default;

bool RenderExternalContent::HasClip() const {
  if (has_clip_.has_value()) {
    return has_clip_.value();
  }
  return RenderBox::HasClip();
}

void RenderExternalContent::Paint(PaintingContext& context,
                                  const FloatPoint& offset) {
  RenderBox::Paint(context, offset);

  bool draw_external_image =
      drawable_image_id_.has_value() && drawable_image_id_.value() > -1;
  bool draw_punch_hole =
      render_mode_.has_value() && render_mode_.value() == RenderMode::kHybrid;

  [[maybe_unused]] FloatPoint paint_offset = offset + PaintOffset();
  if (view_id_.has_value()) {
    FML_DCHECK(!drawable_image_id_.has_value());
    context.PushLayer(
        new PendingOffsetLayer({paint_offset.x(), paint_offset.y()}),
        [&](PaintingContext& ctx, const FloatPoint& layer_offset) {
          ctx.AddPlatformView(0, 0, ContentWidth(), ContentHeight(),
                              view_id_.value());
        },
        FloatPoint(0, 0));
  } else if (draw_external_image) {
    FML_DCHECK(!view_id_.has_value());
    context.AddDrawableImage(paint_offset.x(), paint_offset.y(), ContentWidth(),
                             ContentHeight(), drawable_image_id_.value(),
                             fit_mode_);
  } else if (draw_punch_hole) {
    Rect punch_hole_rect(0, 0, ContentWidth(), ContentHeight());
    context.AddPunchHole(punch_hole_rect);
  }
  PaintChildren(context, offset);
}

void RenderExternalContent::SetDrawableImageId(int64_t drawable_image_id) {
  if (drawable_image_id == drawable_image_id_) {
    return;
  }
  drawable_image_id_ = drawable_image_id;
  MarkNeedsPaint();
}

void RenderExternalContent::SetViewId(int64_t view_id) {
  if (view_id_ == view_id) {
    return;
  }
  view_id_ = view_id;
  MarkNeedsPaint();
}

void RenderExternalContent::SetRenderMode(RenderMode render_mode) {
  if (render_mode_ == render_mode) {
    return;
  }
  render_mode_ = render_mode;
  MarkNeedsPaint();
}

}  // namespace clay
