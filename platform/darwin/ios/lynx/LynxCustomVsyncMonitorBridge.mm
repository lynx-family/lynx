// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCustomVsyncMonitorBridge.h"

#include <utility>

#include "platform/common/custom_vsync_monitor.h"

namespace lynx {
namespace platform {

std::shared_ptr<base::VSyncMonitorPlatformImpl> CreateVSyncMonitorPlatformImpl(
    id<LynxCustomVsyncMonitor> monitor) {
  if (!monitor) {
    return nullptr;
  }
  return std::make_shared<CustomVSyncMonitor>(
      [monitor](base::VSyncMonitorPlatformImpl::Callback callback) {
        auto reply =
            std::make_shared<base::VSyncMonitorPlatformImpl::Callback>(std::move(callback));
        [monitor requestVsync:^(int64_t frameStartTimeNanos, int64_t frameTargetTimeNanos) {
          (*reply)(frameStartTimeNanos, frameTargetTimeNanos);
        }];
      });
}

}  // namespace platform
}  // namespace lynx
