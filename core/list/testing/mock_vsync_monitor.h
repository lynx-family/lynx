// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_MOCK_VSYNC_MONITOR_H_
#define CORE_LIST_TESTING_MOCK_VSYNC_MONITOR_H_

#include <cstdint>

#include "core/base/threading/vsync_monitor.h"

namespace lynx {
namespace list {

// A manually advanced VSyncMonitor for List animation tests. VSyncMonitor uses
// nanosecond timestamps, while this helper accepts milliseconds to match the
// animation-duration API.
class MockVSyncMonitor final : public base::VSyncMonitor {
 public:
  MockVSyncMonitor() = default;
  ~MockVSyncMonitor() override = default;

  void TriggerFrameAtMilliseconds(int64_t frame_start_ms) {
    constexpr int64_t kNanosecondsPerMillisecond = 1000 * 1000;
    constexpr int64_t kFrameDurationNanoseconds =
        16 * kNanosecondsPerMillisecond;
    const int64_t frame_start = frame_start_ms * kNanosecondsPerMillisecond;
    OnVSync(frame_start, frame_start + kFrameDurationNanoseconds);
  }

 protected:
  void RequestVSync() override {}
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_MOCK_VSYNC_MONITOR_H_
