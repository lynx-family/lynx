// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/lynx_basic_animator/basic_animator_frame_callback_provider.h"

#include <utility>

#include "core/shell/common/vsync_monitor.h"

namespace lynx {
namespace animation {
namespace basic {

std::shared_ptr<shell::VSyncMonitor>
BasicAnimatorFrameCallbackProvider::GetVSyncMonitor() {
  thread_local std::shared_ptr<shell::VSyncMonitor> local_vsync_monitor_;
  if (!vsync_monitor_) {
    local_vsync_monitor_ = shell::VSyncMonitor::Create();
    local_vsync_monitor_->BindToCurrentThread();
    local_vsync_monitor_->Init();
    vsync_monitor_ = local_vsync_monitor_;
  }
  return vsync_monitor_;
}

void BasicAnimatorFrameCallbackProvider::RequestNextFrame(
    base::MoveOnlyClosure<void, const fml::TimePoint&> callback) {
  GetVSyncMonitor()->ScheduleVSyncSecondaryCallback(
      reinterpret_cast<uintptr_t>(this),
      [callback = std::move(callback)](int64_t frame_start, int64_t frame_end) {
        callback(fml::TimePoint::FromTicks((frame_start)));
      });
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
