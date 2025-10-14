// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/layers/layer_tree.h"

#include "base/include/fml/time/time_point.h"
#include "base/trace/native/trace_event.h"
#include "clay/flow/embedded_views.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layer_snapshot_store.h"
#include "clay/flow/layers/cacheable_layer.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/paint_utils.h"
#include "clay/flow/raster_cache.h"
#include "clay/gfx/animation/keyframe_set.h"
#include "clay/gfx/animation/keyframes_manager.h"
#include "clay/gfx/animation/transition_manager.h"
#include "clay/gfx/skity_to_skia_utils.h"

namespace clay {

LayerTree::LayerTree(const skity::Vec2& frame_size, float device_pixel_ratio)
    : frame_size_(frame_size),
      device_pixel_ratio_(device_pixel_ratio),
      rasterizer_tracing_threshold_(0),
      checkerboard_raster_cache_images_(false),
      checkerboard_offscreen_layers_(false) {
  FML_DCHECK(device_pixel_ratio_ != 0.0f);
}

LayerTree::~LayerTree() { MarkFrameTimingsForPipelineIfNeeded(); }

void LayerTree::AppendFrameTimings(std::vector<FrameTimingItem>&& timings,
                                   bool pipeline_end) {
  if (frame_timings_.empty()) {
    frame_timings_ = std::move(timings);
  } else {
    frame_timings_.reserve(frame_timings_.size() + timings.size());
    frame_timings_.insert(frame_timings_.end(),
                          std::make_move_iterator(timings.begin()),
                          std::make_move_iterator(timings.end()));
  }

  if (pipeline_end) {
    MarkFrameTimingsForPipelineIfNeeded();
  }
}

void LayerTree::MarkFrameTimingsForPipelineIfNeeded() {
  if (pipeline_id_list_.empty() || !pipeline_end_callback_) {
    frame_timings_.clear();
    return;
  }

  pipeline_end_callback_(std::move(frame_timings_),
                         std::move(pipeline_id_list_));
}

#ifndef ENABLE_SKITY
inline SkColorSpace* GetColorSpace(SkCanvas* canvas) {
  return canvas ? canvas->imageInfo().colorSpace() : nullptr;
}
#endif  // ENABLE_SKITY

bool LayerTree::Preroll(CompositorContext::ScopedFrame& frame,
                        bool ignore_raster_cache, skity::Rect cull_rect) {
  TRACE_EVENT("clay", "LayerTree::Preroll");

  if (!root_layer_) {
    FML_DLOG(ERROR) << "The scene did not specify any layers.";
    return false;
  }

  frame.context().raster_cache().SetCheckboardCacheImages(
      checkerboard_raster_cache_images_);
  LayerStateStack state_stack;
  state_stack.set_preroll_delegate(cull_rect,
                                   frame.root_surface_transformation());
  RasterCache* cache =
      ignore_raster_cache ? nullptr : &frame.context().raster_cache();
  raster_cache_items_.clear();

  PrerollContext context = {
      // clang-format off
      .raster_cache                  = cache,
      .gr_context                    = frame.gr_context(),
      .compositor_state              = frame.compositor_state(),
      .state_stack                   = state_stack,
#ifndef ENABLE_SKITY
      .dst_color_space               = GetColorSpace(frame.canvas()),
#endif // ENABLE_SKITY
      .surface_needs_readback        = false,
      .raster_time                   = frame.context().raster_time(),
      .ui_time                       = frame.context().ui_time(),
      .drawable_image_registry       = frame.context().drawable_image_registry(),
      .frame_device_pixel_ratio      = device_pixel_ratio_,
      .raster_cached_entries         = &raster_cache_items_,
      .request_new_frame             = request_new_frame_
      // clang-format on
  };

  root_layer_->Preroll(&context);

  return context.surface_needs_readback;
}

void LayerTree::TryToRasterCache(
    const std::vector<RasterCacheItem*>& raster_cached_items,
    const PaintContext* paint_context, bool ignore_raster_cache) {
  unsigned i = 0;
  const auto item_size = raster_cached_items.size();
  while (i < item_size) {
    auto* item = raster_cached_items[i];
    if (item->need_caching()) {
      // try to cache current layer
      // If parent failed to cache, just proceed to the next entry
      // cache current entry, this entry's parent must not cache
      if (item->TryToPrepareRasterCache(*paint_context, false)) {
        // if parent cached, then foreach child layer to touch them.
        for (unsigned j = 0; j < item->child_items(); j++) {
          auto* child_item = raster_cached_items[i + j + 1];
          if (child_item->need_caching()) {
            child_item->TryToPrepareRasterCache(*paint_context, true);
          }
        }
        i += item->child_items() + 1;
        continue;
      }
    }
    i++;
  }
}

void LayerTree::SetServiceManagerForAnimation(
    std::shared_ptr<clay::ServiceManager> service_manager) {
  if (!HasAnimations()) {
    return;
  }
  animation_host_->SetServiceManager(service_manager);
}

void LayerTree::ResetServiceManagerForAnimation() {
  if (!HasAnimations()) {
    return;
  }
  animation_host_->ResetServiceManager();
}

void LayerTree::SetAnimationHost(
    std::shared_ptr<clay::AnimationHost> animation_host) {
  animation_host_ = std::move(animation_host);
}

void LayerTree::Paint(CompositorContext::ScopedFrame& frame,
                      bool ignore_raster_cache) const {
  TRACE_EVENT("clay", "LayerTree::Paint");

  if (!root_layer_) {
    FML_DLOG(ERROR) << "The scene did not specify any layers to paint.";
    return;
  }

  LayerStateStack state_stack;
  if (checkerboard_offscreen_layers_) {
    state_stack.set_checkerboard_func(DrawCheckerboard);
  }

  state_stack.set_delegate(frame.canvas());

  // clear the previous snapshots.
  LayerSnapshotStore* snapshot_store = nullptr;
  if (enable_leaf_layer_tracing_) {
    frame.context().snapshot_store().Clear();
    snapshot_store = &frame.context().snapshot_store();
  }

  RasterCache* cache =
      ignore_raster_cache ? nullptr : &frame.context().raster_cache();

  PaintContext context = {
      // clang-format off
      .state_stack                   = state_stack,
      .canvas                        = frame.canvas(),
      .gr_context                    = frame.gr_context(),
#ifndef ENABLE_SKITY
      .dst_color_space               = GetColorSpace(frame.canvas()),
#endif // ENABLE_SKITY
      .compositor_state              = frame.compositor_state(),
      .raster_time                   = frame.context().raster_time(),
      .ui_time                       = frame.context().ui_time(),
      .drawable_image_registry       = frame.context().drawable_image_registry(),
      .raster_cache                  = cache,
      .frame_device_pixel_ratio      = device_pixel_ratio_,
      .layer_snapshot_store          = snapshot_store,
      .enable_leaf_layer_tracing     = enable_leaf_layer_tracing_,
      // clang-format on
  };

  if (cache) {
    cache->EvictUnusedCacheEntries();
    TryToRasterCache(raster_cache_items_, &context, ignore_raster_cache);
  }

  if (root_layer_->needs_painting(context)) {
    root_layer_->Paint(context);
  }
}

void LayerTree::MergeAnimations(LayerTree* prev_layer_tree) {
  if (prev_layer_tree == nullptr || !HasAnimations() ||
      !prev_layer_tree->HasAnimations() || prev_layer_tree == this) {
    return;
  }
  animation_host_->MergeAnimations(prev_layer_tree->animation_host_.get());
}

bool LayerTree::DoAnimations() {
  int64_t now = fml::TimePoint::Now().ToEpochDelta().ToMilliseconds();
  return animation_host_ && animation_host_->DoAnimationFrame(now);
}

bool LayerTree::HasAnimations() const {
  return animation_host_ && animation_host_->HasAnimations();
}

}  // namespace clay
