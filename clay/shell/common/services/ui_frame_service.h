// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_UI_FRAME_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_UI_FRAME_SERVICE_H_

#include <future>
#include <memory>

#include "base/include/closure.h"
#include "build/build_config.h"
#include "clay/common/service/service.h"
#include "clay/flow/frame_timings.h"
#include "clay/shell/common/services/sync_compositor_service.h"

namespace clay {

struct RasterFrameServicePuppet;

class UIFrameService : public clay::Service<UIFrameService, clay::Owner::kUI> {
 public:
  static std::shared_ptr<UIFrameService> Create();

  // Request a UI frame via Raster/vsync in async mode, or notify the
  // platform in sync mode.
  void RequestFrame();
  void SetVsyncSourceActive(bool active);

  // Indicates that the PageView has received meaningful layout, which means we
  // can build first frame now.
  void OnFirstMeaningfulLayout();

  // Queue the synchronous scheduler call on Raster and fulfill the promise
  // when it returns. UI preparation runs first on the calling UI thread.
  void DemandDrawHw(std::promise<bool> frame_promise);

#if defined(OS_LINUX)
  // Linux sync mode only. Process pending UI work without requesting a draw.
  // Returns whether UI is ready for synchronous composition.
  bool PrepareFrame();
#endif

  // Build UI with the supplied vsync timings and queue any resulting LayerTree
  // on Raster. Used by vsync scheduling and synchronous UI preparation.
  void BeginFrame(std::unique_ptr<clay::FrameTimingsRecorder> recorder);

  // Build UI and mark any resulting LayerTree commit for immediate
  // Raster drawing.
  void ForceBeginFrame(std::unique_ptr<clay::FrameTimingsRecorder> recorder,
                       fml::closure no_update_callback = nullptr);

  void PrepareForRecycle();
  void CleanForRecycle();

 private:
  void BeginFrame();
  void CommitWithNoUpdates();

  void OnInit(clay::ServiceManager& service_manager,
              const clay::UIServiceContext& ctx) override;
  void OnDestroy() override;

  Engine* engine_ = nullptr;
  std::unique_ptr<RasterFrameServicePuppet> raster_frame_service_;
  clay::Puppet<clay::Owner::kUI, SyncCompositorService>
      sync_compositor_service_;
  bool using_sync_compositor_ = false;
  bool vsync_source_active_ = true;
  bool frame_requested_ = false;
  bool inside_ui_frame_ = false;
};
}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SERVICES_UI_FRAME_SERVICE_H_
