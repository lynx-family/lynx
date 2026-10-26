// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <unordered_map>

#include "base/include/log/logging.h"
#include "base/include/no_destructor.h"
#include "platform/embedder/lynx_vsync_monitor_priv.h"

LYNX_EXTERN_C lynx_vsync_monitor_t* lynx_vsync_monitor_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_vsync_monitor_t*, void*)) {
  lynx_vsync_monitor_t* monitor = new lynx_vsync_monitor_t;
  monitor->user_data = user_data;
  monitor->finalizer = finalizer;
  return monitor;
}

LYNX_EXTERN_C void* lynx_vsync_monitor_get_user_data(
    lynx_vsync_monitor_t* monitor) {
  return monitor->user_data;
}

LYNX_EXTERN_C void lynx_vsync_monitor_bind_request_vsync_func(
    lynx_vsync_monitor_t* monitor, lynx_vsync_monitor_request_vsync func) {
  monitor->request_vsync_func = func;
}

namespace lynx {
namespace embedder {
namespace {

// Host callbacks may outlive the view. Use an opaque, one-shot request token
// rather than exposing a pointer to the monitor to the host.
struct PendingRequests {
  std::mutex mutex;
  uintptr_t next_id = 0;
  std::unordered_map<uintptr_t, std::weak_ptr<LynxVSyncMonitor>> monitors;
};

PendingRequests& GetPendingRequests() {
  static base::NoDestructor<PendingRequests> requests;
  return *requests;
}

}  // namespace

LynxVSyncMonitor::~LynxVSyncMonitor() {
  auto& requests = GetPendingRequests();
  {
    std::lock_guard<std::mutex> lock(requests.mutex);
    requests.monitors.erase(pending_request_id_);
  }

  if (c_monitor_->finalizer) {
    c_monitor_->finalizer(c_monitor_, c_monitor_->user_data);
  }
  delete c_monitor_;
}

void LynxVSyncMonitor::RequestVSync(Callback callback) {
  if (!callback) {
    return;
  }
  bool request_pending;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    request_pending = !callbacks_.empty();
    callbacks_.emplace_back(std::move(callback));
  }
  if (request_pending) {
    return;
  }
  if (!c_monitor_ || !c_monitor_->request_vsync_func) {
    LOGI("request_vsync_func is null, skip RequestVSync");
    // Keep requests pending. An external monitor must never fall back to the
    // display clock when the host does not provide a signal.
    return;
  }
  auto& requests = GetPendingRequests();
  uintptr_t request_id;
  {
    std::lock_guard<std::mutex> lock(requests.mutex);
    do {
      request_id = ++requests.next_id;
    } while (request_id == 0 || requests.monitors.count(request_id));
    requests.monitors.emplace(request_id, weak_from_this());
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_request_id_ = request_id;
  }
  c_monitor_->request_vsync_func(c_monitor_, &OnVSync,
                                 reinterpret_cast<void*>(request_id));
}

void LynxVSyncMonitor::OnVSync(void* user_data, int64_t frame_start_time,
                               int64_t frame_target_time) {
  const auto request_id = reinterpret_cast<uintptr_t>(user_data);
  auto& requests = GetPendingRequests();
  std::shared_ptr<LynxVSyncMonitor> monitor;
  {
    std::lock_guard<std::mutex> lock(requests.mutex);
    auto it = requests.monitors.find(request_id);
    if (it == requests.monitors.end()) {
      return;
    }
    monitor = it->second.lock();
    requests.monitors.erase(it);
  }
  if (!monitor) {
    return;
  }
  std::vector<Callback> callbacks;
  {
    std::lock_guard<std::mutex> lock(monitor->mutex_);
    monitor->pending_request_id_ = 0;
    callbacks.swap(monitor->callbacks_);
  }
  // A callback may request the following VSync. Do not hold the monitor lock or
  // include those new requests in the signal currently being delivered.
  for (Callback& callback : callbacks) {
    callback(frame_start_time, frame_target_time);
  }
}

}  // namespace embedder
}  // namespace lynx
