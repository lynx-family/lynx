// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_VSYNC_MONITOR_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_VSYNC_MONITOR_PRIV_H_

#include <mutex>
#include <vector>

#include "core/public/vsync_monitor_platform_impl.h"
#include "platform/embedder/public/capi/lynx_vsync_monitor_capi.h"

struct lynx_vsync_monitor_t {
  void* user_data = nullptr;
  void (*finalizer)(lynx_vsync_monitor_t*, void*) = nullptr;
  lynx_vsync_monitor_request_vsync request_vsync_func = nullptr;
};

namespace lynx {
namespace embedder {

class LynxVSyncMonitor : public base::VSyncMonitorPlatformImpl {
 public:
  explicit LynxVSyncMonitor(lynx_vsync_monitor_t* c_monitor)
      : c_monitor_(c_monitor) {}
  ~LynxVSyncMonitor() override;
  void RequestVSync(Callback callback) override;

 private:
  static void OnVSync(void* user_data, int64_t frame_start_time,
                      int64_t frame_target_time);
  lynx_vsync_monitor_t* c_monitor_;
  std::mutex mutex_;
  std::vector<Callback> callbacks_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_VSYNC_MONITOR_PRIV_H_
