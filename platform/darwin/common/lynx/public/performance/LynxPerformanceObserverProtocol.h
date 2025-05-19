// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxPerformanceEntry.h>

NS_ASSUME_NONNULL_BEGIN
@protocol LynxPerformanceObserverProtocol <NSObject>

/**
 * Notify the client that a performance event has been sent. It will be called every time a
 * performance event is generated, including but not limited to container initialization, engine
 * rendering, rendering metrics update, etc.
 *
 * Note: This method is for performance events and will be executed on the reporter thread, so do
 * not execute complex logic or UI modification logic in this method.
 *
 * @param entry the LynxPerformanceEntry about the performance event
 *
 */
@optional
- (void)onPerformanceEvent:(nonnull LynxPerformanceEntry *)entry;

@end

NS_ASSUME_NONNULL_END
