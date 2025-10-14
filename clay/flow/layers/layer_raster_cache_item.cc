// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/layer_raster_cache_item.h"

#include <utility>

#include "clay/flow/layers/container_layer.h"
#include "clay/flow/raster_cache_item.h"
#include "clay/flow/raster_cache_util.h"

namespace clay {

LayerRasterCacheItem::LayerRasterCacheItem(Layer* layer,
                                           int layer_cached_threshold,
                                           bool can_cache_children)
    : RasterCacheItem(
          RasterCacheKeyID(layer->unique_id(), RasterCacheKeyType::kLayer),
          // The layer raster_cache_item's cache state default value is none.
          CacheState::kNone),
      layer_(layer),
      layer_cached_threshold_(layer_cached_threshold),
      can_cache_children_(can_cache_children) {}

void LayerRasterCacheItem::PrerollSetup(PrerollContext* context,
                                        const skity::Matrix& matrix) {
  cache_state_ = CacheState::kNone;
  if (context->raster_cache && context->raster_cached_entries) {
    context->raster_cached_entries->push_back(this);
    child_items_ = context->raster_cached_entries->size();
    matrix_ = matrix;
  }
}

std::unique_ptr<LayerRasterCacheItem> LayerRasterCacheItem::Make(
    Layer* layer, int layer_cache_threshold, bool can_cache_children) {
  return std::make_unique<LayerRasterCacheItem>(layer, layer_cache_threshold,
                                                can_cache_children);
}

void LayerRasterCacheItem::PrerollFinalize(PrerollContext* context,
                                           const skity::Matrix& matrix) {
  if (!context->raster_cache || !context->raster_cached_entries) {
    return;
  }
  // We've marked the cache entry that we would like to cache so it stays
  // alive, but if the following conditions apply then we need to set our
  // state back to kDoNotCache so that we don't populate the entry later.
  if (context->has_platform_view || context->has_drawable_image_layer ||
      context->has_punch_hole_layer || context->has_running_picture_animation ||
      context->has_running_transform_animation || context->has_deferred_image ||
      context->state_stack.content_culled(layer_->paint_bounds())) {
    return;
  }
  child_items_ = context->raster_cached_entries->size() - child_items_;
  if (num_cache_attempts_ >= layer_cached_threshold_) {
    // the layer can be cached
    cache_state_ = CacheState::kCurrent;
    context->raster_cache->MarkSeen(key_id_, matrix_, true);
  } else {
    num_cache_attempts_++;
    // access current layer
    if (can_cache_children_) {
      if (!layer_children_id_.has_value()) {
        auto ids = RasterCacheKeyID::LayerChildrenIds(layer_);
        if (!ids.has_value()) {
          return;
        }
        layer_children_id_.emplace(std::move(ids.value()),
                                   RasterCacheKeyType::kLayerChildren);
      }
      cache_state_ = CacheState::kChildren;
      context->raster_cache->MarkSeen(layer_children_id_.value(), matrix_,
                                      true);
    }
  }
}

std::optional<RasterCacheKeyID> LayerRasterCacheItem::GetId() const {
  switch (cache_state_) {
    case kCurrent:
      return key_id_;
    case kChildren:
      return layer_children_id_;
    default:
      return {};
  }
}

const skity::Rect* LayerRasterCacheItem::GetPaintBoundsFromLayer() const {
  switch (cache_state_) {
    case CacheState::kCurrent:
      return &(layer_->paint_bounds());
    case CacheState::kChildren:
      FML_DCHECK(layer_->as_container_layer());
      return &(layer_->as_container_layer()->child_paint_bounds());
    default:
      FML_DCHECK(cache_state_ != CacheState::kNone);
      return nullptr;
  }
}

bool Rasterize(RasterCacheItem::CacheState cache_state, Layer* layer,
               const PaintContext& paint_context, clay::GrCanvas* canvas) {
  FML_DCHECK(cache_state != RasterCacheItem::CacheState::kNone);
  LayerStateStack state_stack;
  state_stack.set_delegate(canvas);
  state_stack.set_checkerboard_func(
      paint_context.state_stack.checkerboard_func());
  PaintContext context = {
      // clang-format off
      .state_stack                   = state_stack,
      .canvas                        = canvas,
      .gr_context                    = paint_context.gr_context,
#ifndef ENABLE_SKITY
      .dst_color_space               = paint_context.dst_color_space,
#endif  // ENABLE_SKITY
      .compositor_state              = paint_context.compositor_state,
      .raster_time                   = paint_context.raster_time,
      .ui_time                       = paint_context.ui_time,
      .drawable_image_registry       = paint_context.drawable_image_registry,
      .raster_cache                  = paint_context.raster_cache,
      .frame_device_pixel_ratio      = paint_context.frame_device_pixel_ratio,
      // clang-format on
  };

  switch (cache_state) {
    case RasterCacheItem::CacheState::kCurrent:
      FML_DCHECK(layer->needs_painting(context));
      layer->Paint(context);
      break;
    case RasterCacheItem::CacheState::kChildren:
      layer->PaintChildren(context);
      break;
    case RasterCacheItem::CacheState::kNone:
      FML_DCHECK(cache_state != RasterCacheItem::CacheState::kNone);
      return false;
  }
  return true;
}

static const auto* flow_type = "RasterCacheFlow::Layer";

bool LayerRasterCacheItem::TryToPrepareRasterCache(const PaintContext& context,
                                                   bool parent_cached) const {
  has_been_cached_ = false;
  if (!context.raster_cache || parent_cached) {
    return false;
  }
  if (cache_state_ != kNone) {
    if (const skity::Rect* paint_bounds = GetPaintBoundsFromLayer()) {
      RasterCache::Context r_context = {
          // clang-format off
          .gr_context         = context.gr_context,
#ifndef ENABLE_SKITY
          .dst_color_space    = context.dst_color_space,
#endif  // ENABLE_SKITY
          .matrix             = matrix_,
          .logical_rect       = *paint_bounds,
          .flow_type          = flow_type,
          // clang-format on
      };
      has_been_cached_ = context.raster_cache->UpdateCacheEntry(
          GetId().value(), r_context,
          [ctx = context, cache_state = cache_state_,
           layer = layer_](clay::GrCanvas* canvas) {
            Rasterize(cache_state, layer, ctx, canvas);
          });
      return has_been_cached_;
    }
  }
  return false;
}

bool LayerRasterCacheItem::Draw(const PaintContext& context,
                                const clay::GrPaint* paint) const {
  return Draw(context, context.canvas, paint);
}

bool LayerRasterCacheItem::Draw(const PaintContext& context,
                                clay::GrCanvas* canvas,
                                const clay::GrPaint* paint) const {
  if (!context.raster_cache || !canvas) {
    return false;
  }
  switch (cache_state_) {
    case RasterCacheItem::kNone:
      return false;
    case RasterCacheItem::kCurrent: {
      return context.raster_cache->Draw(key_id_, *canvas, paint);
    }
    case RasterCacheItem::kChildren: {
      return context.raster_cache->Draw(layer_children_id_.value(), *canvas,
                                        paint);
    }
  }
}

}  // namespace clay
