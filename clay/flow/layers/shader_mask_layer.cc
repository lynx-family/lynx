// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/shader_mask_layer.h"

#include <utility>

#include "clay/flow/raster_cache_util.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

ShaderMaskLayer::ShaderMaskLayer(
    std::shared_ptr<clay::ColorSource> color_source,
    const skity::Rect& mask_rect, clay::BlendMode blend_mode)
    : CacheableContainerLayer(
          RasterCacheUtil::kMinimumRendersBeforeCachingFilterLayer),
      color_source_(std::move(color_source)),
      mask_rect_(mask_rect),
      blend_mode_(blend_mode) {}

ShaderMaskLayer::ShaderMaskLayer(const skity::Rect& mask_rect,
                                 clay::BlendMode blend_mode,
                                 std::shared_ptr<clay::Picture> picture)
    : CacheableContainerLayer(
          RasterCacheUtil::kMinimumRendersBeforeCachingFilterLayer),
      picture_(std::move(picture)),
      is_picture_mask_(true),
      mask_rect_(mask_rect),
      blend_mode_(blend_mode) {}

void ShaderMaskLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const ShaderMaskLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (color_source_ != prev->color_source_ || picture_ != prev->picture_ ||
        is_picture_mask_ != prev->is_picture_mask_ ||
        mask_rect_ != prev->mask_rect_ || blend_mode_ != prev->blend_mode_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  if (context->has_raster_cache()) {
    context->SetTransform(
        RasterCacheUtil::GetIntegralTransCTM(context->GetTransform()));
  }
#endif
  DiffChildren(context, prev);

  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void ShaderMaskLayer::Preroll(PrerollContext* context) {
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);
  AutoCache cache = AutoCache(layer_raster_cache_item_.get(), context,
                              context->state_stack.transform_4x4());

  ContainerLayer::Preroll(context);
  layer_raster_cache_item_->MarkCacheableEffect(context);
  // We always paint with a saveLayer (or a cached rendering),
  // so we can always apply opacity in any of those cases.
  context->renderable_state_flags = kSaveLayerRenderFlags;
}

void ShaderMaskLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));

  auto mutator = context.state_stack.save();

  if (context.raster_cache) {
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    mutator.integralTransform();
#endif

    clay::GrPaint paint;
    if (layer_raster_cache_item_->Draw(context,
                                       context.state_stack.fill(paint))) {
      return;
    }
  }
  mutator.saveLayer(paint_bounds());
  PaintChildren(context);
  clay::GrPaint paint;
  PAINT_SET_BLEND_MODE(paint, blend_mode_);
  if (!is_picture_mask_) {
    auto shader_rect =
        skity::Rect::MakeWH(mask_rect_.Width(), mask_rect_.Height());
    if (color_source_) {
      PAINT_SET_SHADER(paint, color_source_->gr_object());
    }
    CANVAS_TRANSLATE(context.canvas, mask_rect_.Left(), mask_rect_.Top());
    CANVAS_DRAW_RECT(context.canvas, shader_rect, paint);
  } else if (picture_) {
    CANVAS_SAVELAYER(context.canvas, mask_rect_, paint);
    CANVAS_TRANSLATE(context.canvas, mask_rect_.Left(), mask_rect_.Top());
#ifndef ENABLE_SKITY
    context.canvas->drawPicture(picture_->picture()->raw());
#else
    picture_->picture()->raw()->Draw(context.canvas);
#endif
    CANVAS_RESTORE(context.canvas);
  }
}

}  // namespace clay
