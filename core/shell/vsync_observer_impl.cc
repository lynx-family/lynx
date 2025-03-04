// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/vsync_observer_impl.h"

#include "core/shell/common/vsync_monitor.h"

namespace lynx {
namespace shell {

void VSyncObserverImpl::RequestVSync() {
  if (has_pending_vsync_request_) {
    return;
  }
  has_pending_vsync_request_ = true;
  vsync_monitor_->AsyncRequestVSync(
      [this](int64_t frame_start_time, int64_t frame_end_time) {
        vsync_monitor_->runtime_actor()->Act(
            [this, frame_start_time, frame_end_time](auto& runtime) {
              has_pending_vsync_request_ = false;
              DoFrame(frame_start_time, frame_end_time);
            });
      });
}

inline void VSyncObserverImpl::SwapVSyncCallbacks(
    std::vector<VSyncCallback>& swap_callbacks) {
  // first run flush callback; then JS RAF callback
  for (auto& cb_pair : before_animation_frame_callbacks_) {
    swap_callbacks.push_back(std::move(cb_pair.second));
  }
  before_animation_frame_callbacks_.clear();

  for (auto& cb_pair : vsync_callbacks_) {
    swap_callbacks.push_back(std::move(cb_pair.second));
  }
  vsync_callbacks_.clear();
}

inline void VSyncObserverImpl::DoFrame(int64_t frame_start_time,
                                       int64_t frame_end_time) {
  std::vector<VSyncCallback> callbacks;
  SwapVSyncCallbacks(callbacks);
  for (auto& cb : callbacks) {
    cb(frame_start_time, frame_end_time);
  }

  for (auto& cb : after_animation_frame_callbacks_) {
    cb(frame_start_time, frame_end_time);
  }
}

void VSyncObserverImpl::RequestAnimationFrame(
    uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) {
  // make sure to run on Runtime thread
  DCHECK(js_runner_->RunsTasksOnCurrentThread());

  vsync_callbacks_.emplace(id, std::move(callback));
  RequestVSync();
}

void VSyncObserverImpl::RequestBeforeAnimationFrame(
    uintptr_t id, base::MoveOnlyClosure<void, int64_t, int64_t> callback) {
  // make sure to run on Runtime thread
  DCHECK(js_runner_->RunsTasksOnCurrentThread());

  before_animation_frame_callbacks_.emplace(id, std::move(callback));
  RequestVSync();
}

void VSyncObserverImpl::RegisterAfterAnimationFrameListener(
    base::MoveOnlyClosure<void, int64_t, int64_t> callback) {
  after_animation_frame_callbacks_.emplace_back(std::move(callback));
}

}  // namespace shell
}  // namespace lynx
