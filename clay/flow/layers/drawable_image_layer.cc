// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/drawable_image_layer.h"

#include <memory>

#include "clay/common/graphics/drawable_image.h"
#include "clay/gfx/pixel_helper.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

namespace {
class PixelConverter : public clay::PixelHelper<clay::kPixelTypeClay> {
 public:
  explicit PixelConverter(float device_pixel_ratio)
      : device_pixel_ratio_(device_pixel_ratio) {}
  float DevicePixelRatio() const override { return device_pixel_ratio_; }

 private:
  float device_pixel_ratio_;
};
}  // namespace

DrawableImageLayer::DrawableImageLayer(const skity::Vec2& offset,
                                       const skity::Vec2& size,
                                       int64_t image_id, bool freeze,
                                       clay::ImageSampling sampling,
                                       DrawableImage::FitMode fit_mode)
    : offset_(offset),
      size_(size),
      image_id_(image_id),
      freeze_(freeze),
      sampling_(sampling),
      fit_mode_(fit_mode) {}

void DrawableImageLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(old_layer);
    auto prev = old_layer->as_drawable_image_layer();
    // TODO(knopp) It would be nice to be able to determine that a texture is
    // dirty
    context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(prev));
  }

  // Make sure DiffContext knows there is a DrawableImageLayer in this subtree.
  // This prevents ContainerLayer from skipping DrawableImageLayer diffing when
  // DrawableImageLayer is inside retained layer.
  // See ContainerLayer::DiffChildren
  // https://github.com/flutter/flutter/issues/92925
  context->MarkSubtreeHasDrawableImageLayer();
  context->AddLayerBounds(
      skity::Rect::MakeXYWH(offset_.x, offset_.y, size_.x, size_.y));
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void DrawableImageLayer::Preroll(PrerollContext* context) {
  set_paint_bounds(
      skity::Rect::MakeXYWH(offset_.x, offset_.y, size_.x, size_.y));
  context->has_drawable_image_layer = true;
  context->renderable_state_flags = LayerStateStack::kCallerCanApplyOpacity;
}

void DrawableImageLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));

  std::shared_ptr<DrawableImage> image =
      context.drawable_image_registry
          ? context.drawable_image_registry->GetDrawableImage(image_id_)
          : nullptr;
  if (!image) {
    TRACE_EVENT_INSTANT("clay", "null image");
    return;
  }

  clay::GrPaint sk_paint;
  clay::Paint clay_paint;
  DrawableImage::PaintContext ctx{
      .canvas = context.canvas,
      .gr_context = context.gr_context,
      .sk_paint = context.state_stack.fill(sk_paint),
      .clay_paint = context.state_stack.fill(clay_paint),
  };

  // Image coordinates are physical pixels. We need to apply scale and convert
  // the bounds to physical pixels.
  auto converter = PixelConverter(context.frame_device_pixel_ratio);
  auto scale =
      converter.GetPixelRatio<clay::kPixelTypePhysical, clay::kPixelTypeClay>();

  clay::GrAutoCanvasRestore auto_restore(context.canvas, true);
  CANVAS_SCALE(context.canvas, scale, scale);

  auto physical_bounds = skity::Rect(
      converter.ConvertTo<clay::kPixelTypePhysical>(paint_bounds().Left()),
      converter.ConvertTo<clay::kPixelTypePhysical>(paint_bounds().Top()),
      converter.ConvertTo<clay::kPixelTypePhysical>(paint_bounds().Right()),
      converter.ConvertTo<clay::kPixelTypePhysical>(paint_bounds().Bottom()));
  image->Paint(ctx, physical_bounds, freeze_, ToSk(sampling_), fit_mode_);
}

}  // namespace clay
