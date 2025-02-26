// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/base/android/message_loop_android_vsync.h"

#include <utility>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace base {
namespace android {
MessageLoopAndroidVSync::MessageLoopAndroidVSync() {
  vsync_monitor_ = std::make_shared<shell::VSyncMonitorAndroid>();
  vsync_monitor_->BindToCurrentThread();
  vsync_monitor_->Init();
}

void MessageLoopAndroidVSync::WakeUp(fml::TimePoint time_point) {
  if (fml::TimePoint::Now() < time_point || WaitForVSyncTimeOut()) {
    // Scenario 1: The execution time of the task has not yet arrived. Use the
    // epoll to wake up the looper at the specified time.

    // Scenario 2: When app goes into the background, the platform layer may no
    // longer provides VSync callbacks to the application. In this case, we need
    // to use epoll to wake up the looper to flush tasks.
    MessageLoopAndroid::WakeUp(time_point);
  } else if (!HasPendingVSyncRequest()) {
    // No pending VSync request, a new VSync request should be sent.
    request_vsync_time_millis_ = base::CurrentSystemTimeMilliseconds();
    vsync_monitor_->RequestVSyncOnUIThread(
        [this](int64_t frame_start_time_ns, int64_t frame_target_time_ns) {
          request_vsync_time_millis_ = 0;
          max_execute_time_ms_ = static_cast<uint64_t>(
              (frame_target_time_ns - frame_start_time_ns) *
              kTraversalProportion / kNSecPerMSec);
          RunExpiredTasksNow();
        });
  }
}

bool MessageLoopAndroidVSync::WaitForVSyncTimeOut() {
  auto now = base::CurrentSystemTimeMilliseconds();
  // There is a pending VSync request, and the waiting time has already exceeded
  // the threshold.
  return HasPendingVSyncRequest() &&
         (now - request_vsync_time_millis_ >= kWaitingVSyncTimeoutMillis);
}

bool MessageLoopAndroidVSync::HasPendingVSyncRequest() {
  return request_vsync_time_millis_ > 0;
}

void MessageLoopAndroidVSync::FlushTasks(fml::FlushType type) {
  TRACE_EVENT("lynx", "MessageLoopAndroidVSync::FlushTasks");
  const auto now = fml::TimePoint::Now();
  auto begin = base::CurrentSystemTimeMilliseconds();
  base::closure invocation;
  do {
    auto task = task_queue_->GetNextTaskToRun(queue_ids_, now);
    if (!task || !task.has_value()) {
      break;
    }

    invocation = std::move(task.value().task);
    if (!invocation) {
      break;
    }
    invocation();

    auto observers = task_queue_->GetObserversToNotify(task->task_queue_id);
    for (const auto& observer : observers) {
      (*observer)();
    }
    if (type == fml::FlushType::kSingle) {
      break;
    }
    if (base::CurrentSystemTimeMilliseconds() - begin > max_execute_time_ms_) {
      // The maximum execution time has been reached, no further execution.
      return;
    }
  } while (invocation);
}

bool MessageLoopAndroidVSync::CanRunNow() {
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

}  // namespace android
}  // namespace base
}  // namespace lynx
