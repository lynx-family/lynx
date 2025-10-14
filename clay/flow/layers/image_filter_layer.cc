// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/image_filter_layer.h"

#include <utility>

#include "clay/flow/layers/layer.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/gfx/comparable.h"

namespace clay {

ImageFilterLayer::ImageFilterLayer(
    std::shared_ptr<const clay::ImageFilter> filter, const skity::Vec2& offset)
    : CacheableContainerLayer(
          RasterCacheUtil::kMinimumRendersBeforeCachingFilterLayer),
      offset_(offset),
      filter_(std::move(filter)),
      transformed_filter_(nullptr) {}

void ImageFilterLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const ImageFilterLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (NotEquals(filter_, prev->filter_) || offset_ != prev->offset_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }

  context->PushTransform(skity::Matrix::Translate(offset_.x, offset_.y));
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  if (context->has_raster_cache()) {
    context->SetTransform(
        RasterCacheUtil::GetIntegralTransCTM(context->GetTransform()));
  }
#endif

  if (filter_) {
    auto filter = filter_->makeWithLocalMatrix(context->GetTransform());
    if (filter) {
      // This transform will be applied to every child rect in the subtree
      context->PushFilterBoundsAdjustment([filter](skity::Rect rect) {
        skity::Rect filter_out_bounds;
        filter->map_device_bounds(rect, skity::Matrix(), filter_out_bounds);
        return filter_out_bounds;
      });
    }
  }
  DiffChildren(context, prev);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void ImageFilterLayer::Preroll(PrerollContext* context) {
  auto mutator = context->state_stack.save();
  mutator.translate(offset_);

  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);

  AutoCache cache = AutoCache(layer_raster_cache_item_.get(), context,
                              context->state_stack.transform_4x4());

  skity::Rect child_bounds = skity::Rect::MakeEmpty();

  PrerollChildren(context, &child_bounds);

  if (!filter_) {
    set_paint_bounds(child_bounds);
    return;
  }

  // Our saveLayer would apply any outstanding opacity or any outstanding
  // color filter after it applies our image filter. So we can apply either
  // of those attributes with our saveLayer.
  context->renderable_state_flags =
      (LayerStateStack::kCallerCanApplyOpacity |
       LayerStateStack::kCallerCanApplyColorFilter);

  child_bounds.RoundOut();
  filter_->map_device_bounds(child_bounds, skity::Matrix(), child_bounds);
  child_bounds.Offset(offset_.x, offset_.y);

  set_paint_bounds(child_bounds);

  // CacheChildren only when the transformed_filter_ doesn't equal null.
  // So in here we reset the LayerRasterCacheItem cache state.
  layer_raster_cache_item_->MarkNotCacheChildren();

  // TODO(lijing.0511): LocalMatrixImageFilter is not supported in skity, so we
  // do not cache children now. Once it is supported, will remove this macro.
  // @see DlLocalMatrixImageFilter::gr_object
#ifndef ENABLE_SKITY
  transformed_filter_ =
      filter_->makeWithLocalMatrix(context->state_stack.transform_4x4());
  if (transformed_filter_) {
    layer_raster_cache_item_->MarkCacheChildren();
  }
#endif  // ENABLE_SKITY
}

void ImageFilterLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));
  if (subtree_has_punch_hole()) {
    auto mutator = context.state_stack.save();
    mutator.translate(offset_);
    context.only_draw_punch_hole = true;
    PaintChildren(context);
    context.only_draw_punch_hole = false;
  }

  auto mutator = context.state_stack.save();
  mutator.translate(offset_);

  if (context.raster_cache) {
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    // Always apply the integral transform in the presence of a raster cache
    // whether or not we will draw from the cache
    mutator.integralTransform();
#endif

    // Try drawing the layer cache item from the cache before applying the
    // image filter if it was cached with the filter applied.
    if (!layer_raster_cache_item_->IsCacheChildren()) {
      clay::GrPaint paint;
      if (layer_raster_cache_item_->Draw(context,
                                         context.state_stack.fill(paint))) {
        return;
      }
    }
  }

  if (context.raster_cache && layer_raster_cache_item_->IsCacheChildren()) {
    // If we render the children from cache then we need the special
    // transformed version of the filter so we must process it into the
    // cache paint object manually.
    FML_DCHECK(transformed_filter_ != nullptr);
    clay::GrPaint paint;
    context.state_stack.fill(paint);
    PAINT_SET_IMAGE_FILTER(paint, transformed_filter_->gr_object());
    if (layer_raster_cache_item_->Draw(context, &paint)) {
      return;
    }
  }

  // Now apply the image filter and then try rendering the children.
  mutator.applyImageFilter(child_paint_bounds(), filter_);

  PaintChildren(context);

#ifndef NDEBUG
  if (context.enable_raster_cache_tag && layer_raster_cache_item_ &&
      layer_raster_cache_item_->has_been_cached()) {
    // Generated raster cache, but not used.
    DrawRasterCacheTag(context.canvas, paint_bounds().Width() / 2,
                       paint_bounds().Height() / 2, 0);
  }
#endif  // NDEBUG
}

}  // namespace clay
