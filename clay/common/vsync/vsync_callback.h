// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_VSYNC_VSYNC_CALLBACK_H_
#define CLAY_COMMON_VSYNC_VSYNC_CALLBACK_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <utility>

#include "base/include/vsync_frame_tick_work.h"

namespace clay {

// Both timestamps are in nanoseconds and use fml::TimePoint::Now()'s clock
// origin.
using VsyncSignalCallback = std::function<void(int64_t frame_start_time_ns,
                                               int64_t frame_target_time_ns)>;
// Requests arrive on the raster thread. The host may reply on any thread.
class VsyncRequestCallback {
 public:
  using Request = std::function<void(VsyncSignalCallback)>;
  using AcquireWork =
      std::function<std::shared_ptr<lynx::base::VSyncFrameTickWork>(
          int64_t frame_start_time_ns)>;
  using FrameTickCallback = std::function<void(
      int64_t, int64_t, std::shared_ptr<lynx::base::VSyncFrameTickWork>)>;

  VsyncRequestCallback() = default;

  VsyncRequestCallback(Request request, AcquireWork acquire = {})
      : request_(std::move(request)), acquire_(std::move(acquire)) {}

  explicit operator bool() const { return static_cast<bool>(request_); }

  void operator()(VsyncSignalCallback callback) const {
    request_(std::move(callback));
  }

  // Acquire before returning from the source callback, while its tick is open.
  void RequestWithFrameTickWork(FrameTickCallback callback) const {
    request_([acquire = acquire_, callback = std::move(callback)](
                 int64_t start, int64_t target) {
      callback(start, target, acquire ? acquire(start) : nullptr);
    });
  }

 private:
  Request request_;
  AcquireWork acquire_;
};

}  // namespace clay

#endif  // CLAY_COMMON_VSYNC_VSYNC_CALLBACK_H_
