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
 * Lynx-attributed memory usage. Instance registration, timeout handling, and
 * aggregation stay inside the internal collector.
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
 * provided, the collector snapshots the live Lynx instance fetchers at request
 * start, queries them asynchronously, and returns either a completed result or a
 * timeout result containing the partial instance list collected before the
 * timeout fired. If no live instance fetchers exist, the callback still receives
 * an asynchronous completed result with zero Lynx-attributed bytes.
 */
- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback;

/**
 * Queries the current Lynx-attributed memory usage with a custom collection timeout.
 *
 * When timeoutMs is less than or equal to 0, the collector uses the default timeout of 2000ms.
 */
- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback
                              timeoutMs:(int64_t)timeoutMs;

@end

NS_ASSUME_NONNULL_END
