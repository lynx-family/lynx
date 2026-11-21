// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_VSYNC_MONITOR_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_VSYNC_MONITOR_PRIV_H_

#include <memory>
#include <mutex>
#include <vector>

#include "core/public/vsync_monitor_platform_impl.h"
#include "platform/embedder/public/capi/lynx_vsync_monitor_capi.h"
#if defined(ENABLE_FRAME_TICK_FENCE)
#include "platform/embedder/frame_tick_fence.h"
#endif

struct lynx_vsync_monitor_t {
  void* user_data = nullptr;
  void (*finalizer)(lynx_vsync_monitor_t*, void*) = nullptr;
  lynx_vsync_monitor_request_vsync request_vsync_func = nullptr;
  lynx_vsync_monitor_frame_tick_complete frame_tick_complete_func = nullptr;
};

namespace lynx {
namespace embedder {

class LynxVSyncMonitor : public base::VSyncMonitorPlatformImpl,
                         public std::enable_shared_from_this<LynxVSyncMonitor> {
 public:
  explicit LynxVSyncMonitor(lynx_vsync_monitor_t* c_monitor);
  ~LynxVSyncMonitor() override;
  void RequestVSync(Callback callback) override;
  lynx_vsync_monitor_t* GetCMonitor() const { return c_monitor_; }

#if defined(ENABLE_FRAME_TICK_FENCE)
  std::shared_ptr<base::VSyncFrameTickWork> AcquireFrameTickWork(
      int64_t frame_start_time_ns) override;
#endif

 private:
  void RequestHostVSync();
  static void OnVSync(void* user_data, int64_t frame_start_time,
                      int64_t frame_target_time);
  lynx_vsync_monitor_t* c_monitor_;
  std::mutex mutex_;
  std::vector<Callback> callbacks_;
  uintptr_t pending_request_id_ = 0;
#if defined(ENABLE_FRAME_TICK_FENCE)
  void OnFrameTickComplete(uint64_t frame_id, int64_t frame_start_time);
  uint64_t frame_id_ = 0;
  bool frame_tick_pending_ = false;
  FrameTickFence frame_tick_fence_;
#endif
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_VSYNC_MONITOR_PRIV_H_
