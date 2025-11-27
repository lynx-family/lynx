// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/log/logging.h"
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

LynxVSyncMonitor::~LynxVSyncMonitor() {
  if (c_monitor_->finalizer) {
    c_monitor_->finalizer(c_monitor_, c_monitor_->user_data);
  }
  delete c_monitor_;
}

void LynxVSyncMonitor::RequestVSync(Callback callback) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    callbacks_.emplace_back(std::move(callback));
  }
  if (!c_monitor_ || !c_monitor_->request_vsync_func) {
    LOGE("request_vsync_func is null, skip RequestVSync");
    callbacks_.clear();
    return;
  }
  c_monitor_->request_vsync_func(c_monitor_, &OnVSync, this);
}

void LynxVSyncMonitor::OnVSync(void* user_data, int64_t frame_start_time,
                               int64_t frame_target_time) {
  auto* monitor = reinterpret_cast<LynxVSyncMonitor*>(user_data);
  std::lock_guard<std::mutex> lock(monitor->mutex_);
  for (Callback& callback : monitor->callbacks_) {
    callback(frame_start_time, frame_target_time);
  }
  monitor->callbacks_.clear();
}

}  // namespace embedder
}  // namespace lynx
