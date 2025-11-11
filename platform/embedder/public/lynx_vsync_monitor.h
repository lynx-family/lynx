// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_VSYNC_MONITOR_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_VSYNC_MONITOR_H_

#include <functional>
#include <memory>

#include "capi/lynx_vsync_monitor_capi.h"

namespace lynx {
namespace pub {
class LynxVSyncMonitor : public std::enable_shared_from_this<LynxVSyncMonitor> {
 public:
  explicit LynxVSyncMonitor(lynx_vsync_monitor_t* c_monitor)
      : c_monitor_(c_monitor) {}
  virtual ~LynxVSyncMonitor() = default;
  /**
   * @apidoc
   * @brief Synchronize a Vsync signal
   * @param callback OnVsync signal callback.
   */
  virtual void RequestVSync(std::function<void(int64_t, int64_t)> callback) = 0;

  lynx_vsync_monitor_t* GetCMonitor() { return c_monitor_; }

  void InitIfNeed() {
    if (c_monitor_) {
      return;
    }
    c_monitor_ = lynx_vsync_monitor_create_with_finalizer(
        new std::weak_ptr<LynxVSyncMonitor>(weak_from_this()),
        [](lynx_vsync_monitor_t* monitor, void* user_data) {
          std::weak_ptr<LynxVSyncMonitor>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxVSyncMonitor>*>(user_data);
          delete weak_ptr;
        });
    BindFunction();
  }

 private:
  void BindFunction() {
    lynx_vsync_monitor_bind_request_vsync_func(
        c_monitor_,
        [](lynx_vsync_monitor_t* c_monitor,
           lynx_vsync_monitor_on_vsync_callback callback, void* user_data) {
          auto* weak_monitor =
              reinterpret_cast<std::weak_ptr<LynxVSyncMonitor>*>(
                  lynx_vsync_monitor_get_user_data(c_monitor));
          std::shared_ptr<LynxVSyncMonitor> shared_monitor =
              weak_monitor ? weak_monitor->lock() : nullptr;
          if (!shared_monitor) {
            return;
          }
          shared_monitor->RequestVSync(
              [callback, user_data](int64_t frame_start_time,
                                    int64_t frame_target_time) {
                callback(user_data, frame_start_time, frame_target_time);
              });
        });
  }

  lynx_vsync_monitor_t* c_monitor_ = nullptr;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_VSYNC_MONITOR_H_
