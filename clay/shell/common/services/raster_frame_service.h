// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_RASTER_FRAME_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_RASTER_FRAME_SERVICE_H_

#include <memory>

#include "base/include/fml/time/timer.h"
#include "clay/common/service/service.h"
#include "clay/flow/layers/layer_tree.h"
#include "clay/shell/common/scheduler/scheduler_client.h"
#include "clay/shell/common/services/sync_compositor_service.h"
#include "clay/shell/common/services/ui_frame_service.h"
#include "clay/shell/common/vsync_waiter.h"

namespace clay {

class Scheduler;

class RasterFrameService
    : public clay::Service<RasterFrameService, clay::Owner::kRaster>,
      public SchedulerClient {
 public:
  static std::shared_ptr<RasterFrameService> Create();

  void SetOutputSurfaceValid(bool valid);
  void SetMeaningfulLayout(bool meaningful_layout);

  // Ask the Rasterizer to submit a new Frame synchronously.
  bool DemandDrawHw();

  void RequestRasterFrame();
  void RequestUIFrame(bool inside_ui_frame = false);

  // Force begin UI frame, and Waitting the LayerTree to be commit. It's used
  // when we need to force begin frame outside the state machine. Should call
  // UIBeginFrame after this, otherwise the state machine may be broken.
  // @see Engine::BeginFrameImmediately
  void ForceBeginFrame();

  // Commit the LayerTree, and then notify the scheduler to perform commit.
  void Commit(std::shared_ptr<LayerTree> layer_tree,
              std::unique_ptr<FrameTimingsRecorder> recorder);

  // Commit with no updates, it's used when we are not able to build a new
  // LayerTree. To avoid the state machine to break, we must do a commit action.
  void CommitWithNoUpdates();

  // Force commit & draw the LayerTree, it's only used for unittests. The
  // shell_test is not able to handle the scheduler with state machine.
  void ForceCommit(std::shared_ptr<LayerTree> layer_tree,
                   std::unique_ptr<FrameTimingsRecorder> recorder);

  void ScheduledActionBeginFrame() override;
  void ScheduledActionCommit() override;
  void ScheduledActionActivePendingTree() override;
  void ScheduledActionRasterInvalidate() override;
  void ScheduledActionDraw() override;
  void ScheduledActionUploadImage() override;

  void PrepareForRecycle();
  void CleanForRecycle();

  void NotifyUploadTaskRegistered();

 private:
  void ScheduledFrameDeadline(fml::TimeDelta delay);
  void RequestFrameSignal();
  void RequestVsync();
  void OnVsync(std::unique_ptr<FrameTimingsRecorder> recorder);

  void OnFrameDeadlineArrived();

  void OnInit(clay::ServiceManager& service_manager,
              const clay::RasterServiceContext& ctx) override;
  void OnDestroy() override;

  void StopSchedulerAndCleanLayerTree();

  Rasterizer* rasterizer_ = nullptr;
  std::unique_ptr<Scheduler> scheduler_;

  std::shared_ptr<VsyncWaiter> vsync_waiter_;
  bool vsync_requested_ = false;
  std::unique_ptr<FrameTimingsRecorder> frame_timings_recorder_;
  clay::Puppet<clay::Owner::kRaster, SyncCompositorService>
      sync_compositor_service_;
  bool using_sync_compositor_ = false;

  clay::Puppet<clay::Owner::kRaster, UIFrameService> ui_frame_service_;
  bool force_begin_frame_ = false;

  std::unique_ptr<fml::OneshotTimer> raster_frame_deadline_timer_;

  // We manage three layer trees: one being built, one being committed to
  // pending, and one being active.
  // The `committing_layer_tree_` is currently being built, will be committed to
  // pending_layer_tree with appropriate timing.
  std::shared_ptr<LayerTree> committing_layer_tree_;
  std::unique_ptr<FrameTimingsRecorder> committing_recorder_;
  // The `pending_layer_tree_` is currently being committed, will be activated
  // with appropriate timing.
  std::shared_ptr<LayerTree> pending_layer_tree_;
  std::unique_ptr<FrameTimingsRecorder> pending_recorder_;
  // The `active_layer_tree_` is currently being activated.
  std::shared_ptr<LayerTree> active_layer_tree_;
  std::unique_ptr<FrameTimingsRecorder> active_recorder_;

  uint64_t page_unique_id_ = 0;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_RASTER_FRAME_SERVICE_H_
