// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/lynx_basic_animator/basic_animator_frame_callback_provider.h"

#include <atomic>
#include <utility>

#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace animation {
namespace basic {
namespace {

// Keep generated IDs distinct from aligned object addresses used by the
// other subscribers to a page monitor.
std::atomic<uintptr_t> g_next_frame_callback_id{1};

}  // namespace

BasicAnimatorFrameCallbackProvider::BasicAnimatorFrameCallbackProvider(
    const std::shared_ptr<base::VSyncMonitor>& vsync_monitor)
    : vsync_monitor_(vsync_monitor),
      frame_callback_id_(
          g_next_frame_callback_id.fetch_add(2, std::memory_order_relaxed)) {}

std::shared_ptr<base::VSyncMonitor>
BasicAnimatorFrameCallbackProvider::GetVSyncMonitor() {
  thread_local std::shared_ptr<base::VSyncMonitor> local_vsync_monitor_;
  if (!vsync_monitor_) {
    local_vsync_monitor_ = base::VSyncMonitor::Create(
        tasm::LynxEnv::GetInstance().EnableAnimationVsyncOnUIThread());
    local_vsync_monitor_->BindToCurrentThread();
    local_vsync_monitor_->Init();
    vsync_monitor_ = local_vsync_monitor_;
  }
  return vsync_monitor_;
}

void BasicAnimatorFrameCallbackProvider::RequestNextFrame(
    base::MoveOnlyClosure<void, const fml::TimePoint&> callback) {
  GetVSyncMonitor()->ScheduleVSyncSecondaryCallback(
      frame_callback_id_,
      [callback = std::move(callback)](int64_t frame_start, int64_t frame_end) {
        callback(fml::TimePoint::FromTicks((frame_start)));
      });
}

}  // namespace basic
}  // namespace animation
}  // namespace lynx
