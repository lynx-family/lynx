// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_COMMON_CUSTOM_VSYNC_MONITOR_H_
#define PLATFORM_COMMON_CUSTOM_VSYNC_MONITOR_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "core/public/vsync_monitor_platform_impl.h"

namespace lynx {
namespace platform {

// Coalesces consumers of one host VSync source. The host can reply from any
// thread; each reply is accepted at most once and cannot keep the source alive.
// Replies freeze their subscribers; callback batches are delivered serially.
// A buffered batch is delivered by the thread already draining callbacks.
class CustomVSyncMonitor final
    : public base::VSyncMonitorPlatformImpl,
      public std::enable_shared_from_this<CustomVSyncMonitor> {
 public:
  // Register the next signal and return promptly; replies may be synchronous.
  using RequestCallback = std::function<void(Callback)>;

  explicit CustomVSyncMonitor(RequestCallback request_callback);
  void RequestVSync(Callback callback) override;

 private:
  struct RequestToken {};
  struct CallbackBatch {
    int64_t frame_start_time_ns = 0;
    int64_t frame_target_time_ns = 0;
    std::vector<Callback> callbacks;
  };

  void DispatchHostRequests();
  std::shared_ptr<RequestToken> PrepareNextRequest();
  void DeliverCallbacks();
  bool TakeBufferedBatch(CallbackBatch& batch, bool& start_request_loop);
  void OnVSync(const std::shared_ptr<RequestToken>& request_token,
               int64_t frame_start_time_ns, int64_t frame_target_time_ns);

  const RequestCallback request_callback_;
  std::mutex mutex_;
  std::vector<Callback> waiting_callbacks_;
  // Identifies the outstanding host request. Mutually exclusive with
  // buffered_batch_, which holds an already accepted reply.
  std::shared_ptr<RequestToken> in_flight_request_;
  // One completed batch may wait behind the batch being delivered. While it
  // occupies this slot, new consumers queue without another host request.
  std::optional<CallbackBatch> buffered_batch_;
  // Each loop has one owning thread; the two loops may overlap.
  bool request_loop_running_ = false;
  bool delivery_loop_running_ = false;
};

}  // namespace platform
}  // namespace lynx

#endif  // PLATFORM_COMMON_CUSTOM_VSYNC_MONITOR_H_
