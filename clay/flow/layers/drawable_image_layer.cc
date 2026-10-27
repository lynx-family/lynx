// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/drawable_image_layer.h"

#include <memory>
#ifdef ENABLE_CLAY_DISPLAY_COMPOSITOR
#include "clay/flow/compositor/segment_recorder.h"
#endif

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
#ifdef ENABLE_CLAY_DISPLAY_COMPOSITOR
  if (context.segment_recorder) {
    context.segment_recorder->RecordDynamic(*this, context);
    return;
  }
#endif
  TRACE_EVENT("clay", "DrawableImageLayer::Paint");
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

#ifdef ENABLE_CLAY_DISPLAY_COMPOSITOR
DrawableImageLayer::SegmentPaint DrawableImageLayer::CaptureSegmentPaint(
    PaintContext& context) const {
  MutatorsStack mutators;
  context.state_stack.fill(&mutators);
  GrPaint paint;
  clay::Paint clay_paint;
  context.state_stack.fill(paint);
  context.state_stack.fill(clay_paint);
  auto converter = PixelConverter(context.frame_device_pixel_ratio);
  auto scale = converter.GetPixelRatio<kPixelTypePhysical, kPixelTypeClay>();
  auto bounds = skity::Rect(
      converter.ConvertTo<kPixelTypePhysical>(paint_bounds().Left()),
      converter.ConvertTo<kPixelTypePhysical>(paint_bounds().Top()),
      converter.ConvertTo<kPixelTypePhysical>(paint_bounds().Right()),
      converter.ConvertTo<kPixelTypePhysical>(paint_bounds().Bottom()));
  return
      [registry = context.drawable_image_registry, mutators, paint, clay_paint,
       scale, bounds, id = image_id_, freeze = freeze_, sampling = sampling_,
       fit = fit_mode_](GrCanvas* canvas, GrContext* gpu) {
        auto image = registry ? registry->GetDrawableImage(id) : nullptr;
        if (!image) return;
        GrAutoCanvasRestore restore(canvas, true);
        for (auto it = mutators.Begin(); it != mutators.End(); ++it) {
          switch ((*it)->GetType()) {
            case MutatorType::kTransform:
              canvas->Concat((*it)->GetMatrix());
              break;
            case MutatorType::kClipRect:
              canvas->ClipRect((*it)->GetRect());
              break;
            case MutatorType::kOpacity:
              break;  // Included in the leaf paint.
            default:
              FML_DCHECK(false);
              return;  // Rejected by SegmentPlanner.
          }
        }
        canvas->Scale(scale, scale);
        DrawableImage::PaintContext ctx{canvas, gpu, &paint, &clay_paint};
        image->Paint(ctx, bounds, freeze, ToSk(sampling), fit);
      };
}
#endif
}  // namespace clay
