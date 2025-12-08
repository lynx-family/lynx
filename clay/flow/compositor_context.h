// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_COMPOSITOR_CONTEXT_H_
#define CLAY_FLOW_COMPOSITOR_CONTEXT_H_

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/raster_thread_merger.h"
#include "clay/common/graphics/drawable_image.h"
#include "clay/flow/compositor/compositor_state.h"
#include "clay/flow/diff_context.h"
#include "clay/flow/embedded_views.h"
#include "clay/flow/layer_snapshot_store.h"
#include "clay/flow/raster_cache.h"
#include "clay/flow/stopwatch.h"
#include "clay/gfx/rendering_backend.h"

namespace clay {

class LayerTree;

enum class RasterStatus {
  // Frame has been successfully rasterized.
  kSuccess,
  // Failed to rasterize the frame.
  kFailed,
  // Layer tree was discarded due to LayerTreeDiscardCallback or inability to
  // access the GPU.
  kDiscarded,
};

class FrameDamage {
 public:
  // Sets previous layer tree for calculating frame damage. If not set, entire
  // frame will be repainted.
  void SetPreviousLayerTree(const LayerTree* prev_layer_tree) {
    prev_layer_tree_ = prev_layer_tree;
  }

  // Adds additional damage (accumulated for double / triple buffering).
  // This is area that will be repainted alongside any changed part.
  void AddAdditionalDamage(const skity::Rect& damage) {
    additional_damage_.Join(damage);
  }

  // Specifies clip rect alignment.
  void SetClipAlignment(int horizontal, int vertical) {
    horizontal_clip_alignment_ = horizontal;
    vertical_clip_alignment_ = vertical;
  }

  // Calculates clip rect for current rasterization. This is diff of layer tree
  // and previous layer tree + any additional provided damage.
  // If previous layer tree is not specified, clip rect will be null optional,
  // but the paint region of layer_tree will be calculated so that it can be
  // used for diffing of subsequent frames.
  std::optional<skity::Rect> ComputeClipRect(clay::LayerTree& layer_tree,
                                             bool has_raster_cache);

  // See Damage::frame_damage.
  std::optional<skity::Rect> GetFrameDamage() const {
    return damage_ ? std::make_optional(damage_->frame_damage) : std::nullopt;
  }

  // See Damage::buffer_damage.
  std::optional<skity::Rect> GetBufferDamage() {
    return damage_ ? std::make_optional(damage_->buffer_damage) : std::nullopt;
  }

 private:
  skity::Rect additional_damage_ = skity::Rect::MakeEmpty();
  std::optional<Damage> damage_;
  const LayerTree* prev_layer_tree_ = nullptr;
  int vertical_clip_alignment_ = 1;
  int horizontal_clip_alignment_ = 1;
};

class CompositorContext {
 public:
  class ScopedFrame {
   public:
    ScopedFrame(CompositorContext& context, clay::GrContext* gr_context,
                clay::GrCanvas* canvas, CompositorState* compositor_state,
                const skity::Matrix& root_surface_transformation,
                bool instrumentation_enabled, bool surface_supports_readback);

    ~ScopedFrame();

    clay::GrCanvas* canvas() { return canvas_; }
    clay::GrContext* gr_context() const { return gr_context_; }
    CompositorState* compositor_state() const { return compositor_state_; }

    CompositorContext& context() const { return context_; }

    const skity::Matrix& root_surface_transformation() const {
      return root_surface_transformation_;
    }

    bool surface_supports_readback() { return surface_supports_readback_; }

    RasterStatus Raster(
        LayerTree& layer_tree, bool ignore_raster_cache,
        FrameDamage* frame_damage,
        uint32_t background_color = Color::kTransparent().Value(),
        std::function<void()> before_draw_callback = nullptr);

   private:
    CompositorContext& context_;
    CompositorState* compositor_state_;
    clay::GrContext* gr_context_;
    clay::GrCanvas* canvas_;
    const skity::Matrix root_surface_transformation_;
    const bool instrumentation_enabled_;
    const bool surface_supports_readback_;

    BASE_DISALLOW_COPY_AND_ASSIGN(ScopedFrame);
  };

  CompositorContext();

  explicit CompositorContext(Stopwatch::RefreshRateUpdater& updater);

  virtual ~CompositorContext();

  virtual std::unique_ptr<ScopedFrame> AcquireFrame(
      clay::GrContext* gr_context, clay::GrCanvas* canvas,
      CompositorState* compositor_state,
      const skity::Matrix& root_surface_transformation,
      bool instrumentation_enabled, bool surface_supports_readback);
  void OnGrContextCreated();

  void OnGrContextDestroyed();

  RasterCache& raster_cache() { return raster_cache_; }

  std::shared_ptr<DrawableImageRegistry> drawable_image_registry() {
    return drawable_image_registry_;
  }

  const Counter& frame_count() const { return frame_count_; }

  const Stopwatch& raster_time() const { return raster_time_; }

  Stopwatch& ui_time() { return ui_time_; }

  LayerSnapshotStore& snapshot_store() { return layer_snapshot_store_; }

  std::vector<RasterCacheInfo>* GetRasterCacheInfo() {
    return raster_cache_.GetRasterCacheInfo();
  }

 private:
  RasterCache raster_cache_;
  std::shared_ptr<DrawableImageRegistry> drawable_image_registry_;
  Counter frame_count_;
  Stopwatch raster_time_;
  Stopwatch ui_time_;
  LayerSnapshotStore layer_snapshot_store_;

  /// Only used by default constructor of `CompositorContext`.
  FixedRefreshRateUpdater fixed_refresh_rate_updater_;

  void BeginFrame(ScopedFrame& frame, bool enable_instrumentation);

  void EndFrame(ScopedFrame& frame, bool enable_instrumentation);

  BASE_DISALLOW_COPY_AND_ASSIGN(CompositorContext);
};

}  // namespace clay

#endif  // CLAY_FLOW_COMPOSITOR_CONTEXT_H_
