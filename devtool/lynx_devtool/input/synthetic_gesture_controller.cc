// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/input/synthetic_gesture_controller.h"

#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "core/base/threading/vsync_monitor.h"
#include "devtool/lynx_devtool/input/input_event_target.h"

namespace lynx {
namespace devtool {
namespace input {
namespace {

constexpr int64_t kVSyncFallbackTimeoutMs = 50;
constexpr int64_t kInputAckTimeoutMs = 10 * 1000;

int64_t NowUs() {
  return fml::TimePoint::Now().ToEpochDelta().ToMicroseconds();
}

}  // namespace

std::shared_ptr<SyntheticGestureController> SyntheticGestureController::Create(
    std::shared_ptr<InputEventTarget> target,
    const fml::RefPtr<fml::TaskRunner>& task_runner) {
  return std::shared_ptr<SyntheticGestureController>(
      new SyntheticGestureController(std::move(target), task_runner));
}

void SyntheticGestureController::QueueSyntheticGesture(
    std::unique_ptr<SyntheticGesture> gesture, CompletionCallback callback) {
  if (!gesture) {
    if (callback) {
      callback(SyntheticGestureResult::kFailed);
    }
    return;
  }
  pending_gestures_.push_back(
      PendingGesture{std::move(gesture), std::move(callback)});
  StartNextGesture();
}

SyntheticGestureController::SyntheticGestureController(
    std::shared_ptr<InputEventTarget> target,
    const fml::RefPtr<fml::TaskRunner>& task_runner)
    : target_(std::move(target)), task_runner_(task_runner) {}

SyntheticGestureController::~SyntheticGestureController() {
  if (active_gesture_ && target_) {
    active_gesture_->Cancel(NowUs(), target_.get());
  }
  if (active_callback_) {
    auto callback = std::exchange(active_callback_, nullptr);
    callback(SyntheticGestureResult::kFailed);
  }
  while (!pending_gestures_.empty()) {
    auto pending = std::move(pending_gestures_.front());
    pending_gestures_.pop_front();
    if (pending.callback) {
      pending.callback(SyntheticGestureResult::kFailed);
    }
  }
}

void SyntheticGestureController::StartNextGesture() {
  if (active_gesture_ || waiting_for_input_processed_ ||
      pending_gestures_.empty()) {
    return;
  }
  auto pending = std::move(pending_gestures_.front());
  pending_gestures_.pop_front();
  active_gesture_ = std::move(pending.gesture);
  active_callback_ = std::move(pending.callback);
  RequestNextFrame();
}

void SyntheticGestureController::RequestNextFrame() {
  if (!task_runner_) {
    Complete(SyntheticGestureResult::kFailed);
    return;
  }

  if (!vsync_monitor_) {
    vsync_monitor_ = base::VSyncMonitor::Create(true);
    if (vsync_monitor_) {
      vsync_monitor_->BindTaskRunner(task_runner_);
      vsync_monitor_->Init();
    }
  }

  const uint64_t request_id = ++frame_request_id_;
  pending_frame_request_id_ = request_id;
  if (vsync_monitor_) {
    auto weak_self = weak_from_this();
    vsync_monitor_->AsyncRequestVSync(
        [weak_self, request_id](int64_t, int64_t) {
          if (auto self = weak_self.lock()) {
            self->OnFrameRequested(request_id);
          }
        });
  }

  auto weak_self = weak_from_this();
  task_runner_->PostDelayedTask(
      [weak_self, request_id]() {
        if (auto self = weak_self.lock()) {
          self->OnFrameRequested(request_id);
        }
      },
      fml::TimeDelta::FromMilliseconds(kVSyncFallbackTimeoutMs));
}

void SyntheticGestureController::OnFrameRequested(uint64_t request_id) {
  if (request_id != pending_frame_request_id_) {
    return;
  }
  pending_frame_request_id_ = 0;
  OnFrame(NowUs());
}

void SyntheticGestureController::OnFrame(int64_t frame_time_us) {
  if (!active_gesture_ || !target_) {
    Complete(SyntheticGestureResult::kFailed);
    return;
  }

  const auto result =
      active_gesture_->ForwardInputEvents(frame_time_us, target_.get());
  if (result == SyntheticGestureResult::kRunning) {
    RequestNextFrame();
    return;
  }
  if (result == SyntheticGestureResult::kFailed) {
    Complete(result);
    return;
  }

  waiting_for_input_processed_ = true;
  const uint64_t ack_id = ++input_ack_id_;
  auto weak_self = weak_from_this();
  task_runner_->PostDelayedTask(
      [weak_self, ack_id]() {
        if (auto self = weak_self.lock()) {
          self->OnInputProcessed(ack_id, false);
        }
      },
      fml::TimeDelta::FromMilliseconds(kInputAckTimeoutMs));
  target_->WaitForInputProcessed([weak_self, ack_id](bool success) {
    if (auto self = weak_self.lock()) {
      self->OnInputProcessed(ack_id, success);
    }
  });
}

void SyntheticGestureController::OnInputProcessed(uint64_t ack_id,
                                                  bool success) {
  if (!waiting_for_input_processed_ || ack_id != input_ack_id_) {
    return;
  }
  waiting_for_input_processed_ = false;
  Complete(success ? SyntheticGestureResult::kDone
                   : SyntheticGestureResult::kFailed);
}

void SyntheticGestureController::Complete(SyntheticGestureResult result) {
  auto self = shared_from_this();
  waiting_for_input_processed_ = false;
  pending_frame_request_id_ = 0;
  active_gesture_.reset();
  if (active_callback_) {
    auto callback = std::exchange(active_callback_, nullptr);
    callback(result);
  }
  self->StartNextGesture();
}

}  // namespace input
}  // namespace devtool
}  // namespace lynx
