// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/raster_frame_service.h"

#include <utility>

#include "build/build_config.h"
#include "clay/common/service/service_manager.h"
#include "clay/flow/compositor_context.h"
#include "clay/flow/frame_timings.h"
#include "clay/fml/logging.h"
#include "clay/gfx/image/image_upload_manager.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/scheduler/scheduler.h"
#include "clay/shell/common/services/platform_const_service.h"
#include "clay/shell/common/services/vsync_waiter_service.h"

namespace clay {

std::shared_ptr<RasterFrameService> RasterFrameService::Create() {
  return std::make_shared<RasterFrameService>();
}

void RasterFrameService::OnInit(clay::ServiceManager& service_manager,
                                const clay::RasterServiceContext& ctx) {
  clay::Puppet<clay::Owner::kRaster, VsyncWaiterService> vsync_waiter_service =
      service_manager.GetService<VsyncWaiterService>();
  vsync_waiter_ = vsync_waiter_service->CreateVsyncWaiter(
      service_manager.GetTaskRunners()
          ->SelectTaskRunner<clay::Owner::kRaster>());
  rasterizer_ = ctx.rasterizer;
  page_unique_id_ = ctx.page_unique_id;
  clay::Puppet<clay::Owner::kRaster, PlatformConstService>
      platform_const_service =
          service_manager.GetService<PlatformConstService>();
  using_sync_compositor_ =
      platform_const_service->GetSettings().enable_sync_compositor;
  if (using_sync_compositor_) {
    sync_compositor_service_ =
        service_manager.GetService<SyncCompositorService>();
  }

  scheduler_ = std::make_unique<Scheduler>(this, using_sync_compositor_);
  ui_frame_service_ = service_manager.GetService<UIFrameService>();
  raster_frame_deadline_timer_ = std::make_unique<fml::OneshotTimer>(
      service_manager.GetTaskRunners()
          ->SelectTaskRunner<clay::Owner::kRaster>());
}

void RasterFrameService::OnDestroy() {
  if (raster_frame_deadline_timer_) {
    raster_frame_deadline_timer_->Stop();
  }
  StopSchedulerAndCleanLayerTree();
}

void RasterFrameService::SetOutputSurfaceValid(bool valid) {
  scheduler_->SetOutputSurfaceValid(valid);
}

void RasterFrameService::SetMeaningfulLayout(bool meaningful_layout) {
  scheduler_->SetMeaningfulLayout(meaningful_layout);
}

bool RasterFrameService::DemandDrawHw() { return scheduler_->OnDraw(); }

void RasterFrameService::RequestRasterFrame() {
  scheduler_->SetNeedsRasterBeginFrame();
  RequestFrameSignal();
}

void RasterFrameService::RequestUIFrame(bool inside_ui_frame) {
  scheduler_->SetNeedsUIBeginFrame();
  if (!using_sync_compositor_) {
    // Prioritize the UI frame to be scheduled if first frame has been drawn. it
    // works fine even if the calling of `RequestUIFrame` is more frequent than
    // vsync, because the state machine has already cover this case with pending
    // tree.
    // In order not to affect the first frame, we enable this logic only when
    // the first frame has been drawn.
    // In addition, we should also disable this logic if the `RequestUIFrame` is
    // called during the same frame (inside_ui_frame == true).
    if (!inside_ui_frame && scheduler_->IsFirstFrameDrawn()) {
      scheduler_->NotifyReadyToSendUIBeginFrame();
    }
  }
  RequestFrameSignal();
}

void RasterFrameService::ForceBeginFrame() {
  if (pending_layer_tree_) {
    scheduler_->NotifyReadyToActivate();
  }
  FML_DCHECK(!force_begin_frame_);
  // There will be a Commit immediately after this.
  // In force mode, we dont transfer the state machine.
  force_begin_frame_ = true;
}

void RasterFrameService::Commit(
    std::shared_ptr<LayerTree> layer_tree,
    std::unique_ptr<FrameTimingsRecorder> recorder) {
  // It's the response of ScheduledActionBeginFrame from UI thread if we are
  // using async compositor. If we are using sync compositor, it's the Push
  // request of LayerTree.
  if (!layer_tree) {
    CommitWithNoUpdates();
  } else {
    TRACE_EVENT("clay", "RasterFrameService::Commit");
    if (force_begin_frame_) {
      // Should not have pending layer tree here, it has already activate in
      // ForceBeginFrame.
      force_begin_frame_ = false;

      layer_tree->SetRequestNewFrame([weak_ptr = GetWeakPtr()] {
        if (weak_ptr) {
          weak_ptr->RequestRasterFrame();
        }
      });
      // Force draw the layer_tree.
      TRACE_EVENT("clay", "RasterFrameService::ForceDrawLayerTree");
      // Activate the layer tree and then notify scheduler to perform raster
      // immediately for force draw.
      active_layer_tree_ = std::move(layer_tree);
      active_recorder_ = std::move(recorder);
      scheduler_->SetNeedsRasterBeginFrame();
      scheduler_->NotifyReadyToPerformRasterFrame();
    } else {
      committing_layer_tree_ = std::move(layer_tree);
      committing_recorder_ = std::move(recorder);
      if (!using_sync_compositor_) {
        scheduler_->NotifyReadyToCommit(false);
        RequestFrameSignal();
      } else {
        // Push a LayerTree by ClayView, try to trigger
        // ScheduledActionBeginFrame and notify commit.
        scheduler_->SetNeedsUIBeginFrame();
      }
    }
  }
}

void RasterFrameService::CommitWithNoUpdates() {
  TRACE_EVENT("clay", "RasterFrameService::CommitWithNoUpdates");
  if (force_begin_frame_) {
    force_begin_frame_ = false;
  } else {
    if (!using_sync_compositor_) {
      scheduler_->NotifyReadyToCommit(true);
      RequestFrameSignal();
    }
  }
}

void RasterFrameService::ForceCommit(
    std::shared_ptr<LayerTree> layer_tree,
    std::unique_ptr<FrameTimingsRecorder> recorder) {
  layer_tree->SetRequestNewFrame([weak_ptr = GetWeakPtr()] {
    if (weak_ptr) {
      weak_ptr->RequestRasterFrame();
    }
  });
  // Force draw the layer_tree.
  active_layer_tree_ = std::move(layer_tree);
  active_recorder_ = std::move(recorder);
  scheduler_->SetNeedsRasterBeginFrame();
  scheduler_->NotifyReadyToPerformRasterFrame();
}

void RasterFrameService::RequestFrameSignal() {
  if (using_sync_compositor_) {
    // onDraw driven.
    sync_compositor_service_.Act([](auto& impl) { impl.Invalidate(true); });
  } else {
    // vsync driven.
    RequestVsync();
  }
}

void RasterFrameService::RequestVsync() {
  FML_DCHECK(!using_sync_compositor_);
  if (vsync_requested_) {
    return;
  }
  vsync_requested_ = true;
  vsync_waiter_->AsyncWaitForVsync(
      [weak_self =
           GetWeakPtr()](std::unique_ptr<FrameTimingsRecorder> recorder) {
        if (weak_self) {
          weak_self->vsync_requested_ = false;
          weak_self->OnVsync(std::move(recorder));
        }
      });
}

void RasterFrameService::OnVsync(
    std::unique_ptr<FrameTimingsRecorder> recorder) {
  frame_timings_recorder_ = std::move(recorder);
  // The frame deadline time. If the UI frame is arrived, activate that new
  // LayerTree, otherwise, draw the last active LayerTree if needed.
  // Const the deadline time to 1/4 of the vsync period now, we may redefine
  // it later based on historical raster-timings.
  fml::TimePoint raster_frame_deadline =
      frame_timings_recorder_->GetVsyncStartTime() +
      (frame_timings_recorder_->GetVsyncTargetTime() -
       frame_timings_recorder_->GetVsyncStartTime()) /
          4;
  const auto now = fml::TimePoint::Now();
  if (now < raster_frame_deadline && scheduler_->BeginRasterFrameDeadline()) {
    // Start the deadline timer if the scheduler has sent a UIBeginFrame.
    ScheduledFrameDeadline(raster_frame_deadline - now);
  }
  scheduler_->OnBeginRasterFrame();
  // There's bad cases that the `UIFrameService` might use an expired frame
  // timings recorder, because the state machine might not transfer to
  // `ScheduledActionBeginFrame` that may cause not consume the recorder, and
  // this expired recorder might be used in the next `RequestUIFrame`. Reset
  // it and let the `RequestUIFrame` construct a new recorder with current
  // time.
  frame_timings_recorder_.reset();
}

void RasterFrameService::OnFrameDeadlineArrived() {
  TRACE_EVENT("clay", "RasterFrameService::FrameDeadline");
  scheduler_->OnReachRasterFrameDeadline();
}

void RasterFrameService::ScheduledActionBeginFrame() {
  if (using_sync_compositor_) {
    // Notify the scheduler to commit layer tree immediately if the UI thread
    // has already push the LayerTree.
    scheduler_->NotifyReadyToCommit(committing_layer_tree_ == nullptr);
    return;
  }
  RequestFrameSignal();
  std::unique_ptr<FrameTimingsRecorder> recorder =
      std::move(frame_timings_recorder_);
  if (!recorder) {
    recorder = std::make_unique<FrameTimingsRecorder>();
    const fml::TimePoint placeholder_time = fml::TimePoint::Now();
    recorder->RecordVsync(placeholder_time, placeholder_time);
  }
  // Try to pull a new LayerTree from UI thread if it's async compositor.
  // It will respond with RasterFrameService::Commit when it's done.
  ui_frame_service_.Act([recorder = std::move(recorder)](auto& impl) mutable {
    impl.BeginFrame(std::move(recorder));
  });
}

void RasterFrameService::ScheduledActionCommit() {
  FML_DCHECK(raster_frame_deadline_timer_);
  if (!raster_frame_deadline_timer_->Stopped()) {
    raster_frame_deadline_timer_->Stop();
  }
  FML_DCHECK(!pending_layer_tree_);
  pending_layer_tree_ = std::move(committing_layer_tree_);
  pending_recorder_ = std::move(committing_recorder_);
  scheduler_->NotifyReadyToActivate();
}

void RasterFrameService::ScheduledActionActivePendingTree() {
  FML_DCHECK(pending_layer_tree_);
  if (pending_layer_tree_) {
    if (!using_sync_compositor_) {
      // Try to request next vsync in async mode.
      RequestFrameSignal();
    }
    active_layer_tree_ = std::move(pending_layer_tree_);
    active_recorder_ = std::move(pending_recorder_);
    active_layer_tree_->SetRequestNewFrame([weak_ptr = GetWeakPtr()] {
      if (weak_ptr) {
        weak_ptr->RequestRasterFrame();
      }
    });
  }
}

void RasterFrameService::ScheduledActionRasterInvalidate() {}

void RasterFrameService::ScheduledActionDraw() {
  FML_DCHECK(raster_frame_deadline_timer_);
  RequestFrameSignal();
  if (!raster_frame_deadline_timer_->Stopped()) {
    raster_frame_deadline_timer_->Stop();
  }
  std::unique_ptr<FrameTimingsRecorder> recorder = std::move(active_recorder_);
  if (!recorder) {
    recorder = std::move(frame_timings_recorder_);
    const fml::TimePoint placeholder_time = fml::TimePoint::Now();
    if (!recorder) {
      recorder = std::make_unique<FrameTimingsRecorder>();
      recorder->RecordVsync(placeholder_time, placeholder_time);
    }
    recorder->RecordBuildStart(placeholder_time);
    recorder->RecordBuildEnd(placeholder_time);
  } else {
    frame_timings_recorder_.reset();
  }
  TRACE_EVENT_WITH_FRAME_NUMBER(recorder, "clay",
                                "RasterFrameService::DrawLayerTree");
  rasterizer_->Draw(active_layer_tree_, std::move(recorder));
  scheduler_->NotifyActiveTreeHasBeenDrawn();
}

void RasterFrameService::ScheduledFrameDeadline(fml::TimeDelta delay) {
  FML_DCHECK(raster_frame_deadline_timer_);
  if (!raster_frame_deadline_timer_->Stopped()) {
    raster_frame_deadline_timer_->Stop();
  }
  // Start deadline timer.
  raster_frame_deadline_timer_->Start(delay, [weak_ptr = GetWeakPtr()]() {
    if (weak_ptr) {
      weak_ptr->OnFrameDeadlineArrived();
    }
  });
}

void RasterFrameService::ScheduledActionUploadImage() {
  uint64_t left_tasks =
      ImageUploadManager::GetInstance().ProcessSingleTask(page_unique_id_);
  scheduler_->OnImageUploadTaskFinished(left_tasks);
}

void RasterFrameService::PrepareForRecycle() { scheduler_->Resume(); }

void RasterFrameService::CleanForRecycle() { StopSchedulerAndCleanLayerTree(); }

void RasterFrameService::StopSchedulerAndCleanLayerTree() {
  scheduler_->Stop();
  active_layer_tree_.reset();
  committing_layer_tree_.reset();
  pending_layer_tree_.reset();
#if OS_MAC
  vsync_waiter_.reset();
#endif
}

void RasterFrameService::NotifyUploadTaskRegistered() {
  scheduler_->NotifyUploadTaskRegistered();
}

}  // namespace clay
