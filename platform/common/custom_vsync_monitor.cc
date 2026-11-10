// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/common/custom_vsync_monitor.h"

#include <utility>

namespace lynx {
namespace platform {

CustomVSyncMonitor::CustomVSyncMonitor(RequestCallback request_callback)
    : request_callback_(std::move(request_callback)) {}

void CustomVSyncMonitor::RequestVSync(Callback callback) {
  if (!callback || !request_callback_) {
    return;
  }
  auto keep_alive = shared_from_this();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    waiting_callbacks_.emplace_back(std::move(callback));
    if (in_flight_request_ || buffered_batch_ || request_loop_running_) {
      return;
    }
    request_loop_running_ = true;
  }
  DispatchHostRequests();
}

void CustomVSyncMonitor::DispatchHostRequests() {
  while (auto request_token = PrepareNextRequest()) {
    // Keep host requests serial, including synchronous replies that enqueue
    // another request. Consumer delivery has its own serial loop.
    request_callback_([weak_self = weak_from_this(),
                       request_token = std::move(request_token)](
                          int64_t frame_start_time_ns,
                          int64_t frame_target_time_ns) {
      if (auto self = weak_self.lock()) {
        self->OnVSync(request_token, frame_start_time_ns, frame_target_time_ns);
      }
    });
  }
}

std::shared_ptr<CustomVSyncMonitor::RequestToken>
CustomVSyncMonitor::PrepareNextRequest() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (in_flight_request_ || buffered_batch_ || waiting_callbacks_.empty()) {
    request_loop_running_ = false;
    return nullptr;
  }
  in_flight_request_ = std::make_shared<RequestToken>();
  return in_flight_request_;
}

void CustomVSyncMonitor::OnVSync(
    const std::shared_ptr<RequestToken>& request_token,
    int64_t frame_start_time_ns, int64_t frame_target_time_ns) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (request_token != in_flight_request_) {
      return;
    }
    in_flight_request_.reset();
    buffered_batch_.emplace();
    buffered_batch_->frame_start_time_ns = frame_start_time_ns;
    buffered_batch_->frame_target_time_ns = frame_target_time_ns;
    buffered_batch_->callbacks.swap(waiting_callbacks_);
    if (delivery_loop_running_) {
      return;
    }
    delivery_loop_running_ = true;
  }
  DeliverCallbacks();
}

void CustomVSyncMonitor::DeliverCallbacks() {
  CallbackBatch batch;
  bool start_request_loop = false;
  while (TakeBufferedBatch(batch, start_request_loop)) {
    if (start_request_loop) {
      DispatchHostRequests();
    }
    for (Callback& callback : batch.callbacks) {
      callback(batch.frame_start_time_ns, batch.frame_target_time_ns);
    }
    // Capture destruction may also enqueue requests. Keep delivery serial
    // until those captures have been released outside the mutex.
    batch.callbacks.clear();
  }
}

bool CustomVSyncMonitor::TakeBufferedBatch(CallbackBatch& batch,
                                           bool& start_request_loop) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!buffered_batch_) {
    delivery_loop_running_ = false;
    return false;
  }
  std::swap(batch, *buffered_batch_);
  buffered_batch_.reset();
  // Consumers registered after this reply need a later signal. Freeing
  // the bounded reply slot lets their request proceed before delivery.
  start_request_loop = !waiting_callbacks_.empty() && !request_loop_running_;
  if (start_request_loop) {
    request_loop_running_ = true;
  }
  return true;
}

}  // namespace platform
}  // namespace lynx
