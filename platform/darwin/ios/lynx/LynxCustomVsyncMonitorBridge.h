// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_LYNXCUSTOMVSYNCMONITORBRIDGE_H_
#define PLATFORM_DARWIN_IOS_LYNX_LYNXCUSTOMVSYNCMONITORBRIDGE_H_

#import <Lynx/LynxCustomVsyncMonitor.h>

#if defined(__cplusplus)

#include <memory>

#include "core/public/vsync_monitor_platform_impl.h"

namespace lynx {
namespace platform {

std::shared_ptr<base::VSyncMonitorPlatformImpl> CreateVSyncMonitorPlatformImpl(
    id<LynxCustomVsyncMonitor> _Nullable monitor);

}  // namespace platform
}  // namespace lynx

#endif

#endif  // PLATFORM_DARWIN_IOS_LYNX_LYNXCUSTOMVSYNCMONITORBRIDGE_H_
