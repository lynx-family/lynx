// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/ui_frame_service.h"

#include <utility>

#include "clay/fml/logging.h"
#include "clay/shell/common/engine.h"
#include "clay/shell/common/services/platform_const_service.h"
#include "clay/shell/common/services/raster_frame_service.h"

namespace clay {

struct RasterFrameServicePuppet {
  explicit RasterFrameServicePuppet(
      clay::Puppet<clay::Owner::kUI, RasterFrameService> puppet)
      : puppet_(std::move(puppet)) {}
  clay::Puppet<clay::Owner::kUI, RasterFrameService> puppet_;
};

std::shared_ptr<UIFrameService> UIFrameService::Create() {
  return std::make_shared<UIFrameService>();
}

void UIFrameService::RequestFrame() {
  if (frame_requested_) {
    return;
  }
  frame_requested_ = true;
  if (using_sync_compositor_) {
    sync_compositor_service_.Act([](auto& impl) { impl.Invalidate(false); });
  } else {
    raster_frame_service_->puppet_.Act(
        [inside_ui_frame = inside_ui_frame_](auto& impl) {
          impl.RequestUIFrame(inside_ui_frame);
        });
  }
}

void UIFrameService::OnFirstMeaningfulLayout() {
  raster_frame_service_->puppet_.Act(
      [](auto& impl) { impl.SetMeaningfulLayout(true); });

  if (using_sync_compositor_) {
    // Do not early build LayerTree in sync mode, it may cause duplicate frame
    // when the ClayView is wrap content which is high probability.
    sync_compositor_service_.Act(
        [](auto& impl) { impl.OnFirstMeaningfulLayout(); });
  } else {
    std::unique_ptr<FrameTimingsRecorder> recorder =
        std::make_unique<FrameTimingsRecorder>();
    const fml::TimePoint placeholder_time = fml::TimePoint::Now();
    recorder->RecordVsync(placeholder_time, placeholder_time);
    // Build and push LayerTree in async mode to improve first frame
    // performance.
    ForceBeginFrame(std::move(recorder));
  }
}

void UIFrameService::DemandDrawHw(std::promise<bool> frame_promise) {
  std::unique_ptr<FrameTimingsRecorder> recorder =
      std::make_unique<FrameTimingsRecorder>();
  const fml::TimePoint placeholder_time = fml::TimePoint::Now();
  recorder->RecordVsync(placeholder_time, placeholder_time);
  // Call BeginFrame synchronously first to build and push LayerTree to
  // Scheduler, and will be scheduled with state machine in DemandDrawHw bellow.
  BeginFrame(std::move(recorder));

  raster_frame_service_->puppet_.Act(
      [frame_promise = std::move(frame_promise)](auto& impl) mutable {
        bool result = impl.DemandDrawHw();
        frame_promise.set_value(result);
      });
}

void UIFrameService::BeginFrame(
    std::unique_ptr<clay::FrameTimingsRecorder> recorder) {
  FML_DCHECK(engine_);
  frame_requested_ = false;
  recorder->RecordBuildStart(fml::TimePoint::Now());
  TRACE_EVENT_WITH_FRAME_NUMBER(recorder, "clay", "UIFrameService::BeginFrame");
  inside_ui_frame_ = true;
  if (!engine_->BeginFrame(std::move(recorder))) {
    // Commit with no updates if failed to build LayerTree.
    CommitWithNoUpdates();
  }
  inside_ui_frame_ = false;
}

void UIFrameService::ForceBeginFrame(
    std::unique_ptr<clay::FrameTimingsRecorder> recorder) {
  raster_frame_service_->puppet_.Act(
      [](auto& impl) { impl.ForceBeginFrame(); });
  recorder->RecordBuildStart(fml::TimePoint::Now());
  FML_DCHECK(engine_);
  inside_ui_frame_ = true;
  if (!engine_->BeginFrame(std::move(recorder))) {
    // Commit with no updates if failed to build LayerTree.
    CommitWithNoUpdates();
  }
  inside_ui_frame_ = false;
}

void UIFrameService::CommitWithNoUpdates() {
  raster_frame_service_->puppet_.Act(
      [](auto& impl) { impl.CommitWithNoUpdates(); });
}

void UIFrameService::OnInit(clay::ServiceManager& service_manager,
                            const clay::UIServiceContext& ctx) {
  engine_ = ctx.engine;
  clay::Puppet<clay::Owner::kUI, RasterFrameService> puppet =
      service_manager.GetService<RasterFrameService>();
  raster_frame_service_ = std::make_unique<RasterFrameServicePuppet>(puppet);
  clay::Puppet<clay::Owner::kUI, PlatformConstService> platform_const_service =
      service_manager.GetService<PlatformConstService>();
  using_sync_compositor_ =
      platform_const_service->GetSettings().enable_sync_compositor;
  if (using_sync_compositor_) {
    sync_compositor_service_ =
        service_manager.GetService<SyncCompositorService>();
  }
}

void UIFrameService::OnDestroy() { raster_frame_service_.reset(); }

}  // namespace clay
