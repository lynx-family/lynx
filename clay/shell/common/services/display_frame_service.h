// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SERVICES_DISPLAY_FRAME_SERVICE_H_
#define CLAY_SHELL_COMMON_SERVICES_DISPLAY_FRAME_SERVICE_H_
#include <functional>
#include <memory>

#include "clay/common/service/service.h"
#include "clay/flow/frame_timings.h"
#include "clay/shell/common/vsync_waiter.h"
namespace clay {
// A shared VSync clock posts production work to Raster and schedules
// composition independently on Platform. A display opportunity never waits for
// that Raster tick to finish.
class DisplayFrameService
    : public Service<DisplayFrameService, Owner::kPlatform> {
 public:
  static std::shared_ptr<DisplayFrameService> Create();
  using Tick = std::function<void(std::unique_ptr<FrameTimingsRecorder>)>;
  void RequestRasterTick(Tick tick);
  void RequestDisplay();
  void SetCompose(std::function<bool(fml::TimePoint)> compose);
  void SetActive(bool active);

 private:
  void OnInit(ServiceManager&, const PlatformServiceContext&) override;
  void OnDestroy() override;
  void RequestVsync();
  void OnVsync(std::unique_ptr<FrameTimingsRecorder> recorder);
  void ScheduleCompose();
  fml::RefPtr<fml::TaskRunner> runner_;
  std::shared_ptr<VsyncWaiter> waiter_;
  Tick raster_tick_;
  std::function<bool(fml::TimePoint)> compose_;
  fml::TimePoint deadline_;
  double compose_cost_ms_ = 2.0;
  uint64_t generation_ = 0;
  bool active_ = true, vsync_pending_ = false, display_pending_ = false,
       compose_pending_ = false, opportunity_used_ = true;
};
}  // namespace clay
#endif  // CLAY_SHELL_COMMON_SERVICES_DISPLAY_FRAME_SERVICE_H_
