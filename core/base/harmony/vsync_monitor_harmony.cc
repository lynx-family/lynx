// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/harmony/vsync_monitor_harmony.h"

#include <memory>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"

namespace lynx {
namespace base {

namespace {

// nano seconds
static std::atomic_uint g_refresh_rate_ = 16666700;

}  // namespace

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create() {
  return std::make_shared<lynx::base::VSyncMonitorHarmony>();
}

void VSyncMonitorHarmony::OnUpdateRefreshRate(int64_t refresh_rate) {
  DCHECK(refresh_rate > 0);
  g_refresh_rate_ = (1.0f / refresh_rate) * (1000.0 * 1000.0 * 1000.0);
}

void VSyncMonitorHarmony::RequestVSync() {
  // Request the vsync signal from global vsync manager.
  base::HarmonyVsyncManager::GetInstance().RequestVSync(
      [weak_this = weak_from_this()](long long timestamp) {
        auto shared_this = weak_this.lock();
        if (shared_this) {
          int64_t frame_nanos = static_cast<int64_t>(timestamp);
          auto frame_time = fml::TimePoint::FromEpochDelta(
              fml::TimeDelta::FromNanoseconds(frame_nanos));
          auto now = fml::TimePoint::Now();
          if (frame_time > now) {
            frame_time = now;
          }
          auto target_time =
              frame_time + fml::TimeDelta::FromNanoseconds(g_refresh_rate_);
          auto frame_start_time = frame_time.ToEpochDelta().ToNanoseconds();
          auto frame_target_time = target_time.ToEpochDelta().ToNanoseconds();
          shared_this->OnVSync(frame_start_time, frame_target_time);
        }
      });
}

}  // namespace base
}  // namespace lynx
