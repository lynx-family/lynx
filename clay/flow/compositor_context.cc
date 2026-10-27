// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/compositor_context.h"

#include <optional>
#include <utility>

#include "clay/flow/layers/layer_tree.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

std::optional<skity::Rect> FrameDamage::ComputeClipRect(
    clay::LayerTree& layer_tree, bool has_raster_cache) {
  if (layer_tree.root_layer()) {
    PaintRegionMap empty_paint_region_map;
    DiffContext context(layer_tree.frame_size(),
                        layer_tree.device_pixel_ratio(),
                        layer_tree.paint_region_map(),
                        prev_layer_tree_ ? prev_layer_tree_->paint_region_map()
                                         : empty_paint_region_map,
                        has_raster_cache);
    context.PushCullRect(skity::Rect::MakeSize(layer_tree.frame_size()));
    {
      DiffContext::AutoSubtreeRestore subtree(&context);
      const Layer* prev_root_layer = nullptr;
      if (!prev_layer_tree_ ||
          prev_layer_tree_->frame_size() != layer_tree.frame_size()) {
        // If there is no previous layer tree assume the entire frame must be
        // repainted.
        context.MarkSubtreeDirty(
            skity::Rect::MakeSize(layer_tree.frame_size()));
      } else {
        prev_root_layer = prev_layer_tree_->root_layer();
      }
      layer_tree.root_layer()->Diff(&context, prev_root_layer);
    }

    damage_ =
        context.ComputeDamage(additional_damage_, horizontal_clip_alignment_,
                              vertical_clip_alignment_);
    return damage_->buffer_damage;
  } else {
    return std::nullopt;
  }
}

CompositorContext::CompositorContext()
    : drawable_image_registry_(std::make_shared<DrawableImageRegistry>()),
      raster_time_(fixed_refresh_rate_updater_),
      ui_time_(fixed_refresh_rate_updater_) {}

CompositorContext::CompositorContext(Stopwatch::RefreshRateUpdater& updater)
    : drawable_image_registry_(std::make_shared<DrawableImageRegistry>()),
      raster_time_(updater),
      ui_time_(updater) {}

CompositorContext::~CompositorContext() = default;

void CompositorContext::BeginFrame(ScopedFrame& frame,
                                   bool enable_instrumentation) {
  if (enable_instrumentation) {
    frame_count_.Increment();
    raster_time_.Start();
  }
}

void CompositorContext::EndFrame(ScopedFrame& frame,
                                 bool enable_instrumentation) {
  if (enable_instrumentation) {
    raster_time_.Stop();
  }
}

std::unique_ptr<CompositorContext::ScopedFrame> CompositorContext::AcquireFrame(
    clay::GrContext* gr_context, clay::GrCanvas* canvas,
    CompositorState* compositor_state,
    const skity::Matrix& root_surface_transformation,
    bool instrumentation_enabled, bool surface_supports_readback) {
  return std::make_unique<ScopedFrame>(
      *this, gr_context, canvas, compositor_state, root_surface_transformation,
      instrumentation_enabled, surface_supports_readback);
}

CompositorContext::ScopedFrame::ScopedFrame(
    CompositorContext& context, clay::GrContext* gr_context,
    clay::GrCanvas* canvas, CompositorState* compositor_state,
    const skity::Matrix& root_surface_transformation,
    bool instrumentation_enabled, bool surface_supports_readback)
    : context_(context),
      compositor_state_(compositor_state),
      gr_context_(gr_context),
      canvas_(canvas),
      root_surface_transformation_(root_surface_transformation),
      instrumentation_enabled_(instrumentation_enabled),
      surface_supports_readback_(surface_supports_readback) {
  context_.BeginFrame(*this, instrumentation_enabled_);
}

CompositorContext::ScopedFrame::~ScopedFrame() {
  context_.EndFrame(*this, instrumentation_enabled_);
}

RasterStatus CompositorContext::ScopedFrame::Raster(
    clay::LayerTree& layer_tree, bool ignore_raster_cache,
    FrameDamage* frame_damage, uint32_t background_color,
    std::function<void()> before_draw_callback) {
  TRACE_EVENT("clay", "CompositorContext::ScopedFrame::Raster");

  std::optional<skity::Rect> clip_rect =
      frame_damage
          ? frame_damage->ComputeClipRect(layer_tree, !ignore_raster_cache)
          : std::nullopt;

  bool root_needs_readback = layer_tree.Preroll(
      *this, ignore_raster_cache, clip_rect ? *clip_rect : kGiantRect);
  bool needs_save_layer = root_needs_readback && !surface_supports_readback();

  if (before_draw_callback) {
    before_draw_callback();
  }

  clay::GrAutoCanvasRestore restore(canvas(), clip_rect.has_value());
  if (canvas()) {
    if (clip_rect) {
      CANVAS_CLIP_RECT(canvas(), *clip_rect);
    }
    if (needs_save_layer) {
      TRACE_EVENT("clay", "Canvas::saveLayer");
      auto bounds = skity::Rect::MakeSize(layer_tree.frame_size());
      clay::GrPaint paint;
      PAINT_SET_BLEND_MODE(paint, clay::BlendMode::kSrc);
      CANVAS_SAVELAYER(canvas(), bounds, paint);
    }

    CANVAS_CLEAR(canvas(), clay::Color(background_color));
  }

  layer_tree.Paint(*this, ignore_raster_cache);

  if (canvas() && needs_save_layer) {
    CANVAS_RESTORE(canvas());
  }

  return RasterStatus::kSuccess;
}

void CompositorContext::OnGrContextCreated() {
  drawable_image_registry_->OnGrContextCreated();
  raster_cache_.Clear();
}

void CompositorContext::OnGrContextDestroyed() {
  drawable_image_registry_->OnGrContextDestroyed();
  raster_cache_.Clear();
}

}  // namespace clay
