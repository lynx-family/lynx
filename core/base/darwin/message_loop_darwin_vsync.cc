// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/darwin/message_loop_darwin_vsync.h"

#include <utility>

#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace base {
namespace darwin {
// TODO(huangweiwu): this class will merge with android vsync loop.
MessageLoopDarwinVSync::MessageLoopDarwinVSync() {
  vsync_monitor_ = std::make_shared<shell::VSyncMonitorIOS>(true, false);
  // Will be removed after refactoring VSyncMonitor.
  vsync_monitor_->BindToCurrentThread();
  vsync_monitor_->Init();
}

void MessageLoopDarwinVSync::WakeUp(fml::TimePoint time_point) {
  if (fml::TimePoint::Now() < time_point) {
    MessageLoopDarwin::WakeUp(time_point);
  } else if (!HasPendingVSyncRequest()) {
    // No pending VSync request, a new VSync request should be sent.
    request_vsync_time_millis_ = base::CurrentSystemTimeMilliseconds();
    vsync_monitor_->RequestVSyncOnUIThread(
        [this](int64_t frame_start_time_ns, int64_t frame_target_time_ns) {
          request_vsync_time_millis_ = 0;
          max_execute_time_ms_ = static_cast<uint64_t>(
              (frame_target_time_ns - frame_start_time_ns) * kNSecPerMSec);
          SetRestrictionDuration(
              fml::TimeDelta::FromMilliseconds(max_execute_time_ms_));
          RunExpiredTasksNow();
        });
  }
}

bool MessageLoopDarwinVSync::HasPendingVSyncRequest() {
  return request_vsync_time_millis_ > 0;
}

bool MessageLoopDarwinVSync::CanRunNow() {
  // TODO(heshan): For now, a workaround is in place.
  // Currently, there are two message loops on the UI thread,
  // so special handling is required when making calls from the UI thread.
  // This code will be removed once the MessageLoopVsync is used as default.
  if (UIThread::GetRunner()->GetLoop() ==
      fml::MessageLoop::GetCurrent().GetLoopImpl()) {
    return UIThread::GetRunner(true)->GetLoop().get() == this;
  }

  return MessageLoopImpl::CanRunNow();
}

}  // namespace darwin
}  // namespace base
}  // namespace lynx
