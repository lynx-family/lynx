// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_PUBLIC_VSYNC_MONITOR_PLATFORM_IMPL_H_
#define CORE_PUBLIC_VSYNC_MONITOR_PLATFORM_IMPL_H_

#include <cstdint>
#include <memory>

#include "base/include/closure.h"
#include "base/include/vsync_frame_tick_work.h"

namespace lynx {
namespace base {

class VSyncMonitorPlatformImpl {
 public:
  using Callback = base::MoveOnlyClosure<void, int64_t, int64_t>;
  virtual ~VSyncMonitorPlatformImpl() {}

  virtual void RequestVSync(Callback callback) = 0;
  virtual std::shared_ptr<VSyncFrameTickWork> AcquireFrameTickWork(
      int64_t frame_start_time_ns) {
    return nullptr;
  }
};

}  // namespace base
}  // namespace lynx
#endif  // CORE_PUBLIC_VSYNC_MONITOR_PLATFORM_IMPL_H_
