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
  raster_runner_ = manager.GetTaskRunners()->SelectTaskRunner<Owner::kRaster>();
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
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  ++compose_generation_;
  display_pending_ = false;
  compose_ = std::move(compose);
}
void DisplayFrameService::SetActive(bool active) {
  FML_DCHECK(runner_->RunsTasksOnCurrentThread());
  if (active_ == active) return;
  active_ = active;
  ++generation_;
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
  ++opportunity_;
  opportunity_used_ = false;
  // Start with a two millisecond estimate; retain one millisecond of jitter
  // margin and bound the measured cost so a slow native present cannot starve
  // us.
  deadline_ = recorder->GetVsyncTargetTime() -
              fml::TimeDelta::FromMillisecondsF(compose_cost_ms_ + 1.0);
  // Both operations share a runner. Give a completed output its display
  // opportunity before enqueuing potentially expensive production work.
  if (display_pending_) ScheduleCompose();
  if (raster_tick_) {
    auto tick = std::exchange(raster_tick_, nullptr);
    tick(std::move(recorder));
  }
}
void DisplayFrameService::ScheduleCompose() {
  if (compose_pending_ || !active_ || !display_pending_ || !compose_) return;
  if (fml::TimePoint::Now() >= deadline_ || opportunity_used_) {
    RequestVsync();
    return;
  }
  compose_pending_ = true;
  display_pending_ = false;
  raster_runner_->PostTask([weak = GetWeakPtr(), platform = runner_,
                            compose = compose_, generation = generation_,
                            compose_generation = compose_generation_,
                            opportunity = opportunity_, deadline = deadline_] {
    TRACE_EVENT("clay", "DisplayFrameService::Compose");
    auto start = fml::TimePoint::Now();
    bool again = start >= deadline || compose(deadline);
    double compose_ms = (fml::TimePoint::Now() - start).ToMillisecondsF();
    platform->PostTask(
        [weak, generation, compose_generation, opportunity, again, compose_ms] {
          if (weak) {
            weak->OnComposeComplete(generation, compose_generation, opportunity,
                                    again, compose_ms);
          }
        });
  });
}
void DisplayFrameService::OnComposeComplete(uint64_t generation,
                                            uint64_t compose_generation,
                                            uint64_t opportunity, bool again,
                                            double compose_ms) {
  compose_pending_ = false;
  if (active_ && generation == generation_ &&
      compose_generation == compose_generation_) {
    if (!again) {
      compose_cost_ms_ =
          std::clamp(compose_cost_ms_ * 0.8 + compose_ms * 0.2, 0.5, 4.0);
      if (opportunity == opportunity_) opportunity_used_ = true;
    } else {
      display_pending_ = true;
      // Keep the next VSync armed; an explicit target retry may still fit
      // within the current deadline.
      if (opportunity == opportunity_) {
        RequestVsync();
        return;
      }
    }
  }
  ScheduleCompose();
}
}  // namespace clay
