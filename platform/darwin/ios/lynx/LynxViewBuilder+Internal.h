// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxViewBuilder.h>

#if defined(__cplusplus)
#include <memory>

#include "core/public/vsync_monitor_platform_impl.h"
#endif

@interface LynxViewBuilder ()

- (NSMutableDictionary<NSString *, id> *)getModuleWrapper;

#if defined(__cplusplus)
- (std::shared_ptr<lynx::base::VSyncMonitorPlatformImpl>)vsyncMonitorPlatformImpl;
// Share the configured source when copying View configuration.
- (void)setVSyncMonitorPlatformImpl:
    (const std::shared_ptr<lynx::base::VSyncMonitorPlatformImpl> &)monitor;
#endif

@end
