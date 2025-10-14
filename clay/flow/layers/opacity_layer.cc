// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/opacity_layer.h"

#include <limits>
#include <optional>

#include "clay/flow/animation/animation_mutator.h"
#include "clay/flow/raster_cache_util.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

// the opacity_layer couldn't cache itself, so the cache_threshold is the
// max_int
OpacityLayer::OpacityLayer(uint8_t alpha, const skity::Vec2& offset)
    : alpha_(alpha), offset_(offset), children_can_accept_opacity_(false) {
  UpdateRasterCacheItem(std::numeric_limits<int>::max(), true);
}

void OpacityLayer::Diff(DiffContext* context, const Layer* old_layer) {
  DiffContext::AutoSubtreeRestore subtree(context);
  auto* prev = static_cast<const OpacityLayer*>(old_layer);
  if (!context->IsSubtreeDirty()) {
    FML_DCHECK(prev);
    if (alpha() != prev->alpha() || offset_ != prev->offset_) {
      context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
    }
  }
  if (HasAnimationRunning()) {
    context->MarkSubtreeHasRasterAnimation();
    if (!context->IsSubtreeDirty()) {
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
  DiffChildren(context, prev);
  context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
}

void OpacityLayer::Preroll(PrerollContext* context) {
  auto mutator = context->state_stack.save();
  mutator.translate(offset_);
  mutator.applyOpacity(skity::Rect(), clay::Color::toOpacity(alpha()));

  AutoCache auto_cache = AutoCache(layer_raster_cache_item_.get(), context,
                                   context->state_stack.transform_4x4());
  Layer::AutoPrerollSaveLayerState save =
      Layer::AutoPrerollSaveLayerState::Create(context);

  ContainerLayer::Preroll(context);
  // We store the inheritance ability of our children for |Paint|
  set_children_can_accept_opacity((context->renderable_state_flags &
                                   LayerStateStack::kCallerCanApplyOpacity) !=
                                  0);

  // Now we let our parent layers know that we, too, can inherit opacity
  // regardless of what our children are capable of
  context->renderable_state_flags |= LayerStateStack::kCallerCanApplyOpacity;

  auto rect = paint_bounds();
  rect.Offset(offset_.x, offset_.y);
  set_paint_bounds(rect);

  if (children_can_accept_opacity()) {
    // For opacity layer, we can use raster_cache children only when the
    // children can't accept opacity so if the children_can_accept_opacity we
    // should tell the AutoCache object don't do raster_cache.
    auto_cache.ShouldNotBeCached();
  }
}

void OpacityLayer::Paint(PaintContext& context) const {
  FML_DCHECK(needs_painting(context));
  if (subtree_has_punch_hole()) {
    auto mutator = context.state_stack.save();
    mutator.translate(offset_.x, offset_.y);
    context.only_draw_punch_hole = true;
    PaintChildren(context);
    context.only_draw_punch_hole = false;
  }

  auto mutator = context.state_stack.save();
  mutator.translate(offset_.x, offset_.y);
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
  if (context.raster_cache) {
    mutator.integralTransform();
  }
#endif

  mutator.applyOpacity(child_paint_bounds(), opacity());

  clay::GrPaint paint;
  if (!children_can_accept_opacity()) {
    if (layer_raster_cache_item_->Draw(context,
                                       context.state_stack.fill(paint))) {
      return;
    }
  }

  if (layers().empty()) {
    return;
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

float OpacityLayer::alpha() const {
  if (HasAnimation()) {
    const std::shared_ptr<AnimationMutator>& mutator = GetAnimationMutator();
    return mutator->asOpacity()->alpha();
  }
  return alpha_;
}

}  // namespace clay
