// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/color_filter_layer.h"

#include <utility>

#include "clay/flow/raster_cache_item.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/gfx/comparable.h"
#include "clay/gfx/paint.h"

namespace clay {

ColorFilterLayer::ColorFilterLayer(
    std::shared_ptr<const clay::ColorFilter> filter)
    : CacheableContainerLayer(
          RasterCacheUtil::kMinimumRendersBeforeCachingFilterLayer, true),
      filter_(std::move(filter)) {}

void ColorFilterLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const ColorFilterLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (NotEquals(filter_, prev->filter_)) {
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

void ColorFilterLayer::Preroll(PrerollContext* context) {
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);
  AutoCache cache = AutoCache(layer_raster_cache_item_.get(), context,
                              context->state_stack.transform_4x4());

  ContainerLayer::Preroll(context);

  // Our saveLayer would apply any outstanding opacity or any outstanding
  // image filter before it applies our color filter, but that is in the
  // wrong order compared to how these attributes were applied to the tree
  // (they would have come from one of our ancestors). So we cannot apply
  // those attributes with our saveLayer normally.
  // However, some color filters can commute themselves with an opacity
  // modulation so in that case we can apply the opacity on behalf of our
  // ancestors - otherwise we can apply no attributes.
  if (filter_) {
    context->renderable_state_flags =
        filter_->can_commute_with_opacity()
            ? LayerStateStack::kCallerCanApplyOpacity
            : 0;
  }
  // else - we can apply whatever our children can apply.
}

void ColorFilterLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));
  if (subtree_has_punch_hole()) {
    auto mutator = context.state_stack.save();
    context.only_draw_punch_hole = true;
    PaintChildren(context);
    context.only_draw_punch_hole = false;
  }

  auto mutator = context.state_stack.save();

  if (context.raster_cache) {
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    // Always apply the integral transform in the presence of a raster cache
    // whether or not we will draw from the cache
    mutator.integralTransform();
#endif

    // Try drawing the layer cache item from the cache before applying the
    // color filter if it was cached with the filter applied.
    if (!layer_raster_cache_item_->IsCacheChildren()) {
      clay::GrPaint paint;
      if (layer_raster_cache_item_->Draw(context,
                                         context.state_stack.fill(paint))) {
        return;
      }
    }
  }

  // Now apply the color filter and then try rendering children either from
  // cache or directly.
  mutator.applyColorFilter(paint_bounds(), filter_);

  if (context.raster_cache && layer_raster_cache_item_->IsCacheChildren()) {
    clay::GrPaint paint;
    if (layer_raster_cache_item_->Draw(context,
                                       context.state_stack.fill(paint))) {
      return;
    }
  }

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
