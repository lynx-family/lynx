// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/vsync_waiter.h"

#include <utility>
#include <vector>

#include "base/include/fml/message_loop_task_queues.h"
#include "base/include/fml/task_queue_id.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_point.h"
#include "base/trace/native/trace_event.h"
#include "clay/flow/frame_timings.h"
#include "clay/fml/logging.h"

namespace clay {

static constexpr const char* kVsyncTraceName = "VsyncProcessCallback";

VsyncWaiter::VsyncWaiter(fml::RefPtr<fml::TaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {}

VsyncWaiter::~VsyncWaiter() = default;

// Public method invoked by the animator.
void VsyncWaiter::AsyncWaitForVsync(const Callback& callback) {
  if (!callback) {
    return;
  }

  TRACE_EVENT("clay", "AsyncWaitForVsync");

  {
    std::scoped_lock lock(callback_mutex_);
    if (callback_) {
      // The animator may request a frame more than once within a frame
      // interval. Multiple calls to request frame must result in a single
      // callback per frame interval.
      TRACE_EVENT_INSTANT("clay", "MultipleCallsToVsyncInFrameInterval");
      return;
    }
    callback_ = callback;
    if (!secondary_callbacks_.empty()) {
      // Return directly as `AwaitVSync` is already called by
      // `ScheduleSecondaryCallback`.
      return;
    }
  }
  if (await_vsync_callback_) {
    // Invoke the customized vsync callback.
    await_vsync_callback_(shared_from_this(), task_runner_);
  } else {
    AwaitVSync();
  }
}

void VsyncWaiter::ScheduleSecondaryCallback(uintptr_t id,
                                            const fml::closure& callback) {
  FML_DCHECK(task_runner_->RunsTasksOnCurrentThread());

  if (!callback) {
    return;
  }

  TRACE_EVENT("clay", "ScheduleSecondaryCallback");

  {
    std::scoped_lock lock(callback_mutex_);
    bool secondary_callbacks_originally_empty = secondary_callbacks_.empty();
    auto [_, inserted] =  // NOLINT
        secondary_callbacks_.emplace(id, callback);
    if (!inserted) {
      // Multiple schedules must result in a single callback per frame interval.
      TRACE_EVENT_INSTANT("clay",
                          "MultipleCallsToSecondaryVsyncInFrameInterval");
      return;
    }
    if (callback_) {
      // Return directly as `AwaitVSync` is already called by
      // `AsyncWaitForVsync`.
      return;
    }
    if (!secondary_callbacks_originally_empty) {
      // Return directly as `AwaitVSync` is already called by
      // `ScheduleSecondaryCallback`.
      return;
    }
  }
  AwaitVSyncForSecondaryCallback();
}

void VsyncWaiter::FireCallback(fml::TimePoint frame_start_time,
                               fml::TimePoint frame_target_time) {
  // We assume that the FireCallback is triggered on the task_runner thread.
  // Because on most platforms, vsync callbacks are triggered on the registered
  // thread.
  // If it does not behave as expected on some platforms, manual dispatch should
  // be used.
  FML_DCHECK(task_runner_->RunsTasksOnCurrentThread());

  Callback callback;
  callback.swap(callback_);
  std::vector<lynx::base::closure> secondary_callbacks;
  secondary_callbacks.reserve(secondary_callbacks_.size());

  for (auto& pair : secondary_callbacks_) {
    secondary_callbacks.push_back(std::move(pair.second));
  }
  secondary_callbacks_.clear();

  if (!callback && secondary_callbacks.empty()) {
    // This means that the vsync waiter implementation fired a callback for a
    // request we did not make. This is a paranoid check but we still want to
    // make sure we catch misbehaving vsync implementations.

    // Also may because of raster animation. Just ignore it.
    // TRACE_EVENT_INSTANT("clay", "MismatchedFrameCallback");
    return;
  }
  inside_vsync_ = true;

  if (callback) {
    TRACE_EVENT("clay", kVsyncTraceName, "StartTime",
                frame_start_time.ToEpochDelta().ToMilliseconds(), "TargetTime",
                frame_target_time.ToEpochDelta().ToMilliseconds());
    std::unique_ptr<FrameTimingsRecorder> frame_timings_recorder =
        std::make_unique<FrameTimingsRecorder>();
    frame_timings_recorder->RecordVsync(frame_start_time, frame_target_time);
    callback(std::move(frame_timings_recorder));
  }

  for (auto& secondary_callback : secondary_callbacks) {
    task_runner_->PostTask(std::move(secondary_callback));
  }

  inside_vsync_ = false;
}

void VsyncWaiter::Hook(AWaitVSyncCallback callback) {
  await_vsync_callback_ = callback;
}

}  // namespace clay
