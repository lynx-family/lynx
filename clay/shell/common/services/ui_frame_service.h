// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_UI_FRAME_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_UI_FRAME_SERVICE_H_

#include <future>
#include <memory>

#include "clay/common/service/service.h"
#include "clay/flow/frame_timings.h"
#include "clay/shell/common/services/sync_compositor_service.h"

namespace clay {

struct RasterFrameServicePuppet;

class UIFrameService : public clay::Service<UIFrameService, clay::Owner::kUI> {
 public:
  static std::shared_ptr<UIFrameService> Create();

  // Send a UI frame request to RasterFrameService, and it will trigger the
  // scheduler to transfer the state machine. The scheduler will schedule to
  // call BeginFrame with appropriate timing.
  void RequestFrame();

  // Indicates that the PageView has received meaningful layout, which means we
  // can build first frame now.
  void OnFirstMeaningfulLayout();

  void DemandDrawHw(std::promise<bool> frame_future);

  // Begin a new UI frame, which will try to build a new LayerTree, and than
  // commit to RasterFrameService, it's called from RasterFrameService which is
  // scheduled by the state machine.
  void BeginFrame(std::unique_ptr<clay::FrameTimingsRecorder> recorder);

  // Force begin a new UI frame, which will ignore the state machine and commit
  // the layer tree immediately.
  // It's used for BeginFrameImmediately in Engine.
  void ForceBeginFrame(std::unique_ptr<clay::FrameTimingsRecorder> recorder);

 private:
  void CommitWithNoUpdates();

  void OnInit(clay::ServiceManager& service_manager,
              const clay::UIServiceContext& ctx) override;
  void OnDestroy() override;

  Engine* engine_ = nullptr;
  std::unique_ptr<RasterFrameServicePuppet> raster_frame_service_;
  clay::Puppet<clay::Owner::kUI, SyncCompositorService>
      sync_compositor_service_;
  bool using_sync_compositor_ = false;
  bool frame_requested_ = false;
  bool inside_ui_frame_ = false;
};
}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_UI_FRAME_SERVICE_H_
