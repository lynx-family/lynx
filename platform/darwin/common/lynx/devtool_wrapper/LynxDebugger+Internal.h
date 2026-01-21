// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDebugger.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxDebugger ()

+ (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data
                instanceId:(int32_t)instanceId;
@end

NS_ASSUME_NONNULL_END
