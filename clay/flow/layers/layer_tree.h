// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_LAYERS_LAYER_TREE_H_
#define CLAY_FLOW_LAYERS_LAYER_TREE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_delta.h"
#include "clay/common/graphics/drawable_image.h"
#include "clay/common/service/service.h"
#include "clay/flow/animation/animation_host.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/layer.h"
#include "clay/flow/layers/opacity_layer.h"
#include "clay/flow/raster_cache.h"

namespace clay {

using PipelineID = std::string;
using PipelineEndCallback =
    std::function<void(std::vector<FrameTimingItem>, std::vector<PipelineID>)>;

class AnimationHost;

class LayerTree {
 public:
  LayerTree(const skity::Vec2& frame_size, float device_pixel_ratio);
  ~LayerTree();

  // Perform a preroll pass on the tree and return information about
  // the tree that affects rendering this frame.
  //
  // Returns:
  // - a boolean indicating whether or not the top level of the
  //   layer tree performs any operations that require readback
  //   from the root surface.
  bool Preroll(CompositorContext::ScopedFrame& frame,
               bool ignore_raster_cache = false,
               skity::Rect cull_rect = kGiantRect);

  static void TryToRasterCache(
      const std::vector<RasterCacheItem*>& raster_cached_entries,
      const PaintContext* paint_context, bool ignore_raster_cache = false);

  void Paint(CompositorContext::ScopedFrame& frame,
             bool ignore_raster_cache = false) const;

  Layer* root_layer() const { return root_layer_.get(); }

  void set_root_layer(std::shared_ptr<Layer> root_layer) {
    root_layer_ = std::move(root_layer);
  }

  const skity::Vec2& frame_size() const { return frame_size_; }
  float device_pixel_ratio() const { return device_pixel_ratio_; }

  const PaintRegionMap& paint_region_map() const { return paint_region_map_; }
  PaintRegionMap& paint_region_map() { return paint_region_map_; }

  // The number of frame intervals missed after which the compositor must
  // trace the rasterized picture to a trace file. Specify 0 to disable all
  // tracing
  void set_rasterizer_tracing_threshold(uint32_t interval) {
    rasterizer_tracing_threshold_ = interval;
  }

  uint32_t rasterizer_tracing_threshold() const {
    return rasterizer_tracing_threshold_;
  }

  void set_checkerboard_raster_cache_images(bool checkerboard) {
    checkerboard_raster_cache_images_ = checkerboard;
  }

  void set_checkerboard_offscreen_layers(bool checkerboard) {
    checkerboard_offscreen_layers_ = checkerboard;
  }

  /// When `Paint` is called, if leaf layer tracing is enabled, additional
  /// metadata around rasterization of leaf layers is collected.
  ///
  /// See: `LayerSnapshotStore`
  void enable_leaf_layer_tracing(bool enable) {
    enable_leaf_layer_tracing_ = enable;
  }

  bool is_leaf_layer_tracing_enabled() const {
    return enable_leaf_layer_tracing_;
  }
  void SetServiceManagerForAnimation(
      std::shared_ptr<clay::ServiceManager> service_manager);
  void ResetServiceManagerForAnimation();
  void SetAnimationHost(std::shared_ptr<AnimationHost> animation_host);

  bool DoAnimations();
  bool HasAnimations() const;

  // When a layer_tree from ui get consumed by the Raster Thread. It shouldn't
  // be drawn in time to avoid repeat swap-buffer. Inside we will postpone it
  // until the next Raster thread rendering triggered by the compositor
  // animation, and during this process, we also need to synchronize the state
  // from the previous compositor animation.
  // Always get called in Raster Thread.
  void MergeAnimations(LayerTree* old_layer_tree);

  // The `last_draw_vsync_time` will be set after submit the layerTree to the
  //  screen *successfully*
  fml::TimePoint LastDrawVsyncTime() const { return last_draw_vsync_time_; }

  void SetLastDrawVsyncTime(fml::TimePoint time) {
    last_draw_vsync_time_ = time;
  }

  // The Vsync time of generate the layer_tree by the Vsync.
  fml::TimePoint VsyncTimeOnGeneration() const {
    return vsync_time_on_generation_;
  }

  void SetVsyncTimeOnGeneration(fml::TimePoint time) {
    vsync_time_on_generation_ = time;
  }

  int VsyncSequenceId() const {
    FML_DCHECK(vsync_sequence_id_ >= 0);
    return vsync_sequence_id_;
  }

  void SetVsyncSequenceId(int id) { vsync_sequence_id_ = id; }

  bool HasValidVsyncSequenceId() const { return vsync_sequence_id_ != -1; }

  void SetRequestNewFrame(std::function<void()> request_new_frame) {
    request_new_frame_ = request_new_frame;
  }

  void SetPipelineIdList(std::vector<std::string> pipeline_id_list) {
    pipeline_id_list_ = std::move(pipeline_id_list);
  }

  void AppendFrameTimings(std::vector<FrameTimingItem>&& frame_timings,
                          bool pipeline_end = false);

  void SetPipelineEndCallback(PipelineEndCallback callback) {
    pipeline_end_callback_ = std::move(callback);
  }

 private:
  void MarkFrameTimingsForPipelineIfNeeded();

  std::shared_ptr<Layer> root_layer_;

  fml::TimePoint last_draw_vsync_time_;
  fml::TimePoint vsync_time_on_generation_;
  int vsync_sequence_id_ = -1;

  skity::Vec2 frame_size_ = {0, 0};  // Physical pixels.
  const float device_pixel_ratio_;   // Logical / Physical pixels ratio.
  uint32_t rasterizer_tracing_threshold_;
  bool checkerboard_raster_cache_images_;
  bool checkerboard_offscreen_layers_;
  bool enable_leaf_layer_tracing_ = false;

  PaintRegionMap paint_region_map_;

  std::vector<RasterCacheItem*> raster_cache_items_;

  std::shared_ptr<clay::AnimationHost> animation_host_;

  std::function<void()> request_new_frame_;

  std::vector<PipelineID> pipeline_id_list_;
  std::vector<FrameTimingItem> frame_timings_;
  PipelineEndCallback pipeline_end_callback_;

  BASE_DISALLOW_COPY_AND_ASSIGN(LayerTree);
};

}  // namespace clay

#endif  // CLAY_FLOW_LAYERS_LAYER_TREE_H_
