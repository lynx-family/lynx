// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/vsync_monitor_fallback.h"

#include <algorithm>
#include <memory>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "base/include/fml/message_loop.h"

namespace lynx {
namespace base {

static fml::TimePoint SnapToNextTick(fml::TimePoint value,
                                     fml::TimePoint tick_phase,
                                     fml::TimeDelta tick_interval) {
  fml::TimeDelta offset = (tick_phase - value) % tick_interval;
  if (offset != fml::TimeDelta::Zero()) {
    offset = offset + tick_interval;
  }
  return value + offset;
}

#ifndef __APPLE__
std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  int rate = 60;
#ifdef _WIN32
  HDC hdc = ::GetDC(NULL);
  if (hdc != NULL) {
    rate = std::max(rate, ::GetDeviceCaps(hdc, VREFRESH));
    ::ReleaseDC(NULL, hdc);
  }
#endif
  return std::make_shared<lynx::base::VSyncMonitorFallback>(
      fml::TimeDelta::FromSecondsF(1.0 / rate));
}
#endif

VSyncMonitorFallback::VSyncMonitorFallback(fml::TimeDelta interval)
    : interval_(interval) {}

void VSyncMonitorFallback::Init() { phase_ = fml::TimePoint::Now(); }

VSyncMonitorFallback::~VSyncMonitorFallback() {}

void VSyncMonitorFallback::RequestVSync() {
  if (!runner_) {
    return;
  }
  auto frame_start_time =
      SnapToNextTick(fml::TimePoint::Now(), phase_, interval_);
  auto frame_target_time = frame_start_time + interval_;

  std::weak_ptr<VSyncMonitorFallback> weak_this =
      std::static_pointer_cast<VSyncMonitorFallback>(shared_from_this());
  runner_->PostTaskForTime(
      [frame_start_time, frame_target_time, weak_this]() {
        auto vsync_waiter = weak_this.lock();
        if (vsync_waiter) {
          vsync_waiter->OnVSync(
              frame_start_time.ToEpochDelta().ToNanoseconds(),
              frame_target_time.ToEpochDelta().ToNanoseconds());
        }
      },
      frame_start_time);
}

}  // namespace base
}  // namespace lynx
