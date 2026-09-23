// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxMemoryUsageResult.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxGlobalMemoryUsageResult;

typedef void (^LynxGlobalMemoryUsageCallback)(LynxGlobalMemoryUsageResult *result);

/**
 * Process-level entry point for active Lynx memory queries.
 *
 * Hosts use this singleton when they want a one-shot snapshot of the current
 * Lynx-attributed memory usage. The implementation reads the native global
 * memory monitor directly.
 */
@interface LynxMemoryUsageQuery : NSObject

+ (instancetype)sharedInstance;

/**
 * Queries the current Lynx-attributed memory usage for all registered Lynx instances.
 *
 * The callback is invoked asynchronously on the report thread. Callers that
 * update UIKit must dispatch back to the main thread.
 *
 * The current implementation accepts nil as a no-op. When a callback is
 * provided, it receives a completed native snapshot. If no live instances
 * exist, the callback receives zero Lynx-attributed bytes.
 */
- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback;

/**
 * Queries memory usage while retaining the timeout value as result metadata.
 *
 * The native global query has no per-instance fan-out and does not wait for
 * this timeout. Values less than or equal to 0 use 2000ms.
 */
- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback
                              timeoutMs:(int64_t)timeoutMs;

@end

NS_ASSUME_NONNULL_END
