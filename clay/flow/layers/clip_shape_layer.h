// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_CLIP_SHAPE_LAYER_H_
#define CLAY_FLOW_LAYERS_CLIP_SHAPE_LAYER_H_

#include "clay/flow/layers/cacheable_layer.h"
#include "clay/flow/layers/container_layer.h"
#include "clay/flow/paint_utils.h"
#include "skity/geometry/rrect.hpp"

namespace clay {

template <class T>
class ClipShapeLayer : public CacheableContainerLayer {
 public:
  using ClipShape = T;
  ClipShapeLayer(const ClipShape& clip_shape, Clip clip_behavior)
      : CacheableContainerLayer(),
        clip_shape_(clip_shape),
        clip_behavior_(clip_behavior) {
    FML_DCHECK(clip_behavior != Clip::none);
  }

  void Diff(DiffContext* context, const Layer* old_layer) override {
    DiffContext::AutoSubtreeRestore subtree(context);
    auto* prev = static_cast<const ClipShapeLayer<ClipShape>*>(old_layer);
    if (!context->IsSubtreeDirty()) {
      FML_DCHECK(prev);
      if (clip_behavior_ != prev->clip_behavior_ ||
          clip_shape_ != prev->clip_shape_) {
        context->MarkSubtreeDirty(context->GetOldLayerPaintRegion(old_layer));
      }
    }
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
    if (UsesSaveLayer() && context->has_raster_cache()) {
      context->SetTransform(
          RasterCacheUtil::GetIntegralTransCTM(context->GetTransform()));
    }
#endif
    if (context->PushCullRect(clip_shape_bounds())) {
      DiffChildren(context, prev);
    }
    context->SetLayerPaintRegion(this, context->CurrentSubtreeRegion());
  }

  void Preroll(PrerollContext* context) override {
    bool uses_save_layer = UsesSaveLayer();

    // We can use the raster_cache for children only when the use_save_layer is
    // true so if use_save_layer is false we pass the layer_raster_item is
    // nullptr which mean we don't do raster cache logic.
    AutoCache cache =
        AutoCache(uses_save_layer ? layer_raster_cache_item_.get() : nullptr,
                  context, context->state_stack.transform_4x4());

    Layer::AutoPrerollSaveLayerState save =
        Layer::AutoPrerollSaveLayerState::Create(context, UsesSaveLayer());

    auto mutator = context->state_stack.save();
    ApplyClip(mutator);

    skity::Rect child_paint_bounds = skity::Rect::MakeEmpty();
    PrerollChildren(context, &child_paint_bounds);
    if (child_paint_bounds.Intersect(clip_shape_bounds())) {
      set_paint_bounds(child_paint_bounds);
    } else {
      set_paint_bounds(skity::Rect::MakeEmpty());
    }

    // If we use a SaveLayer then we can accept opacity on behalf
    // of our children and apply it in the saveLayer.
    if (uses_save_layer) {
      context->renderable_state_flags = kSaveLayerRenderFlags;
    }
  }

  void Paint(PaintContext& context) const override {
    FML_DCHECK(needs_painting(context));

    auto mutator = context.state_stack.save();
    ApplyClip(mutator);

    if (!UsesSaveLayer()) {
      PaintChildren(context);
      return;
    }

    if (context.raster_cache) {
#ifndef SUPPORT_FRACTIONAL_TRANSLATION
      mutator.integralTransform();
#endif
      auto restore_apply = context.state_stack.applyState(
          paint_bounds(), LayerStateStack::kCallerCanApplyOpacity);
      clay::GrPaint paint;
      if (layer_raster_cache_item_->Draw(context,
                                         context.state_stack.fill(paint))) {
        return;
      }
    }

    mutator.saveLayer(paint_bounds());
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

  bool UsesSaveLayer() const {
    return clip_behavior_ == Clip::antiAliasWithSaveLayer;
  }

 protected:
  virtual const skity::Rect& clip_shape_bounds() const = 0;
  virtual void ApplyClip(LayerStateStack::MutatorContext& mutator) const = 0;
  virtual ~ClipShapeLayer() = default;

  const ClipShape& clip_shape() const { return clip_shape_; }
  Clip clip_behavior() const { return clip_behavior_; }

 private:
  const ClipShape clip_shape_;
  Clip clip_behavior_;

  BASE_DISALLOW_COPY_AND_ASSIGN(ClipShapeLayer);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_CLIP_SHAPE_LAYER_H_
