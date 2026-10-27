// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_STAGED_RASTERIZER_H_
#define CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_STAGED_RASTERIZER_H_
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/flow/compositor/segment_recorder.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/shell/common/display_compositor/frame_mailbox.h"
#include "clay/shell/common/display_compositor/raster_output_surface.h"
namespace clay::display {
class PendingFramePublication;
class StagedRasterizer {
 public:
  StagedRasterizer(std::unique_ptr<RasterOutputSurface> surface,
                   Stopwatch::RefreshRateUpdater& updater,
                   std::shared_ptr<DrawableImageRegistry> registry,
                   RasterBindingKey binding,
                   std::shared_ptr<FrameMailbox> mailbox,
                   std::function<void()> wake,
                   std::function<void()> request_production = nullptr);
  ~StagedRasterizer();
  bool IsValid() const;
  bool RasterizeToOutputs(std::shared_ptr<LayerTree> tree,
                          int64_t frame_time_ms);
  bool RasterizeLastTreeToOutputs();
  bool UpdateDynamicSegments(int64_t image_id);
  // Retry only work still pending after intervening tree/content submissions.
  bool RetryPendingProduction();
  const RasterBindingKey& binding() const { return binding_; }
  bool NeedsAnimationFrame() const { return needs_animation_frame_; }
  void OnFrameSubmitted(uint64_t layout);
  void InvalidateUploads() {
    if (surface_) surface_->InvalidateUploads();
  }
  void SetOutputConfig(OutputConfig config);
  void Trim();
  ScreenshotData Screenshot(std::shared_ptr<const CompositorFrame> frame,
                            uint32_t background);
  void Release(std::function<void()> drained);
  std::unique_ptr<RasterOutputSurface> Suspend(std::function<void()> drained);
  std::pair<fml::RefPtr<PaintImage>, bool> SnapshotTree(LayerTree& tree) {
    return IsValid() ? surface_->SnapshotTree(tree, context_)
                     : std::make_pair(fml::RefPtr<PaintImage>(), false);
  }

 private:
  bool Produce(bool all);
  bool DeferProduction();
  bool Publish();
  void ClearScene();
  void CancelPendingProduction();
  std::unique_ptr<RasterOutputSurface> surface_;
  CompositorContext context_;
  const RasterBindingKey binding_;
  std::shared_ptr<FrameMailbox> mailbox_;
  std::function<void()> wake_, request_production_;
  std::shared_ptr<LayerTree> tree_, pending_tree_;
  int64_t pending_tree_frame_time_ms_ = 0;
  std::shared_ptr<PendingFramePublication> pending_publication_;
  std::shared_ptr<int> lifetime_ = std::make_shared<int>(0);
  std::shared_ptr<const PageSceneSnapshot> scene_;
  std::vector<RecordedSegment> segments_;
  std::unordered_map<int64_t, std::vector<size_t>> image_segments_;
  std::vector<TextureEntry> outputs_;
  std::set<int64_t> dirty_;
  uint64_t plan_version_ = 0, render_version_ = 0, production_epoch_ = 0;
  bool split_ = false, needs_animation_frame_ = false,
       context_initialized_ = false, full_production_pending_ = false;
  TimingCorrelation timing_;
  fml::TimePoint first_failure_;
};
}  // namespace clay::display
#endif  // CLAY_SHELL_COMMON_DISPLAY_COMPOSITOR_STAGED_RASTERIZER_H_
