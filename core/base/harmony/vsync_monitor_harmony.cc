// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/harmony/vsync_monitor_harmony.h"

#include <algorithm>
#include <atomic>
#include <memory>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/harmony_vsync_manager.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace base {

namespace {

// nano seconds
static std::atomic_uint g_refresh_rate_ = 16666700;

void DispatchVSync(const std::weak_ptr<VSyncMonitor>& weak_monitor,
                   int64_t timestamp) {
  auto monitor = weak_monitor.lock();
  if (!monitor) {
    return;
  }
  monitor->OnVSync(timestamp, timestamp + g_refresh_rate_.load());
}

}  // namespace

std::shared_ptr<VSyncMonitor> VSyncMonitor::Create(bool is_on_ui_thread) {
  return std::make_shared<lynx::base::VSyncMonitorHarmony>(is_on_ui_thread);
}

void VSyncMonitorHarmony::OnUpdateRefreshRate(int64_t refresh_rate) {
  DCHECK(refresh_rate > 0);
  g_refresh_rate_ = (1.0f / refresh_rate) * (1000.0 * 1000.0 * 1000.0);
}

void VSyncMonitorHarmony::RequestVSync() {
  // Request the vsync signal from global vsync manager.
  base::HarmonyVsyncManager::GetInstance().RequestVSync(
      [weak_this = weak_from_this()](long long timestamp) {
        // Keep the existing NativeVSync clock correction on the legacy path.
        const auto now = fml::TimePoint::Now().ToEpochDelta().ToNanoseconds();
        DispatchVSync(weak_this,
                      std::min(static_cast<int64_t>(timestamp), now));
      });
}

void VSyncMonitorHarmony::RequestVSyncOnUIThread() {
  if (!ui_vsync_requester_) {
    // Standalone monitors (for example scrolling and synthetic gestures) have
    // no page UIContext. Preserve their existing NativeVSync source. Page
    // animation monitors receive the UI requester from the shell builder.
    RequestVSync();
    return;
  }
  ui_vsync_requester_([weak_this = weak_from_this()](int64_t timestamp) {
    DCHECK(UIThread::GetRunner()->RunsTasksOnCurrentThread());
    DispatchVSync(weak_this, timestamp);
  });
}

}  // namespace base
}  // namespace lynx
