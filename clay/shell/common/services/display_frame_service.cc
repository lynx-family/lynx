// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/services/display_frame_service.h"

#include <algorithm>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "clay/common/service/service_manager.h"
#include "clay/shell/common/services/vsync_waiter_service.h"
namespace clay {
std::shared_ptr<DisplayFrameService> DisplayFrameService::Create() {
  return std::make_shared<DisplayFrameService>();
}
void DisplayFrameService::OnInit(ServiceManager& manager,
                                 const PlatformServiceContext&) {
  runner_ = manager.GetTaskRunners()->SelectTaskRunner<Owner::kPlatform>();
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  Puppet<Owner::kPlatform, VsyncWaiterService> service =
      manager.GetService<VsyncWaiterService>();
  waiter_ = service->CreateVsyncWaiter(runner_);
}
void DisplayFrameService::OnDestroy() {
  SetActive(false);
  compose_ = nullptr;
  raster_tick_ = nullptr;
}
void DisplayFrameService::SetCompose(
    std::function<bool(fml::TimePoint)> compose) {
  compose_ = std::move(compose);
}
void DisplayFrameService::SetActive(bool active) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (active_ == active) return;
  active_ = active;
  ++generation_;
  compose_pending_ = false;
  vsync_pending_ = false;
  waiter_->ResetPendingCallbacks();
  waiter_->SetEngineIsActive(active);
  if (active && (display_pending_ || raster_tick_)) RequestVsync();
}
void DisplayFrameService::RequestRasterTick(Tick tick) {
  raster_tick_ = std::move(tick);
  RequestVsync();
}
void DisplayFrameService::RequestDisplay() {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  display_pending_ = true;
  if (active_ && !opportunity_used_ && fml::TimePoint::Now() < deadline_)
    ScheduleCompose();
  else
    RequestVsync();
}
void DisplayFrameService::RequestVsync() {
  if (!active_ || vsync_pending_) return;
  vsync_pending_ = true;
  waiter_->AsyncWaitForVsync(
      [weak = GetWeakPtr(), generation = generation_](auto recorder) {
        if (weak && weak->active_ && generation == weak->generation_)
          weak->OnVsync(std::move(recorder));
      });
}
void DisplayFrameService::OnVsync(
    std::unique_ptr<FrameTimingsRecorder> recorder) {
  vsync_pending_ = false;
  opportunity_used_ = false;
  // Start with a two millisecond estimate; retain one millisecond of jitter
  // margin and bound the measured cost so a slow native present cannot starve
  // us.
  deadline_ = recorder->GetVsyncTargetTime() -
              fml::TimeDelta::FromMillisecondsF(compose_cost_ms_ + 1.0);
  if (raster_tick_) {
    auto tick = std::exchange(raster_tick_, nullptr);
    tick(std::move(recorder));
  }
  if (display_pending_) ScheduleCompose();
}
void DisplayFrameService::ScheduleCompose() {
  if (compose_pending_ || !active_ || !compose_) return;
  compose_pending_ = true;
  auto now = fml::TimePoint::Now();
  if (now >= deadline_ || opportunity_used_) {
    compose_pending_ = false;
    RequestVsync();
    return;
  }
  runner_->PostTask([weak = GetWeakPtr(), generation = generation_] {
    if (!weak || generation != weak->generation_) return;
    weak->compose_pending_ = false;
    if (!weak->active_ || !weak->display_pending_ || !weak->compose_) return;
    weak->display_pending_ = false;
    // A task that arrives after the submit budget skips this cycle.
    TRACE_EVENT("clay", "DisplayFrameService::Compose");
    auto start = fml::TimePoint::Now();
    bool again = weak->compose_(weak->deadline_);
    if (!again)
      weak->compose_cost_ms_ = std::clamp(
          weak->compose_cost_ms_ * 0.8 +
              (fml::TimePoint::Now() - start).ToMillisecondsF() * 0.2,
          0.5, 4.0);
    if (!again) weak->opportunity_used_ = true;
    if (again) {
      weak->display_pending_ = true;
      weak->RequestVsync();
    }
  });
}
}  // namespace clay
