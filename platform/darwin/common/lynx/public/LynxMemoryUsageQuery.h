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
 *
 * The current API-only rollout is a no-op facade. It preserves the final
 * asynchronous callback shape but always returns an empty COMPLETED result until
 * the collector-backed implementation is wired in.
 */
@interface LynxMemoryUsageQuery : NSObject

+ (instancetype)sharedInstance;

/**
 * Queries the current Lynx-attributed memory usage for all registered Lynx instances.
 *
 * The callback is invoked asynchronously on the report thread. Callers that
 * update UIKit must dispatch back to the main thread.
 *
 * The current implementation accepts nil as a no-op. When a callback is provided,
 * it receives an empty result with zero instance counts and byte fields, and
 * collectionStatus set to LynxMemoryCollectionStatusCompleted. A later collector
 * implementation will populate the instance list and aggregate memory fields.
 */
- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback;

@end

NS_ASSUME_NONNULL_END
