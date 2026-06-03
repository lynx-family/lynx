// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxMemoryUsageQuery.h>

#import "LynxGlobalMemoryUsageCollectionContext.h"
#import "LynxMemoryUsageFetcher.h"
#import "LynxTraceEventDef.h"

#include <mach/mach.h>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

#include <algorithm>
#include <atomic>
#include <memory>

namespace {

constexpr int64_t kLynxGlobalMemoryUsageDefaultTimeoutMs = 2000;

// LynxGroup uses "-1" for a single, non-shared background runtime. Such
// runtimes are charged per instance and must not be deduplicated as shared BTS
// runtime samples.
static NSString *const kLynxMemoryUsageNonSharedBTSRuntimeGroupId = @"-1";

int64_t LynxMemoryUsageNowMs() {
  return static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000);
}

int64_t LynxMemoryUsageAppBytes() {
  // phys_footprint is the same app-level denominator used by iOS memory diagnostics. It is sampled
  // only when the result is built so the ratio reflects the end of the request window.
  task_vm_info_data_t info;
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
  kern_return_t result =
      task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count);
  if (result != KERN_SUCCESS) {
    return 0;
  }
  return static_cast<int64_t>(info.phys_footprint);
}

int64_t LynxNormalizeGlobalMemoryUsageTimeoutMs(int64_t timeoutMs) {
  return timeoutMs > 0 ? timeoutMs : kLynxGlobalMemoryUsageDefaultTimeoutMs;
}

LynxMemoryUsageFetcherCallback LynxCreateSingleShotMemoryUsageFetcherCallback(
    LynxMemoryUsageFetcherCallback callback) {
  auto didCall = std::make_shared<std::atomic_bool>(false);
  LynxMemoryUsageFetcherCallback callbackCopy = [callback copy];
  return [^(LynxInstanceMemoryUsage *_Nullable result) {
    bool expected = false;
    if (didCall->compare_exchange_strong(expected, true)) {
      callbackCopy(result);
    }
  } copy];
}

void LynxInvokeGlobalMemoryUsageCallbackSafely(LynxGlobalMemoryUsageCallback callback,
                                               LynxGlobalMemoryUsageResult *result) {
  @try {
    callback(result);
  } @catch (NSException *exception) {
    LLogError(@"LynxMemoryUsageCollector failed to invoke global memory usage callback: %@",
              exception.reason);
  }
}

}  // namespace

// Report-thread coordinator for global memory queries. It serializes request setup, timeout,
// per-fetcher completion, aggregation, and callback delivery while the weak fetcher registry lives
// in LynxMemoryUsageFetchers.mm behind the LynxMemoryUsageFetchers registry facade.
@interface LynxMemoryUsageCollector : NSObject

@property(nonatomic, strong, nullable)
    LynxGlobalMemoryUsageCollectionContext *activeCollectionContext;
@property(nonatomic, assign) uint64_t nextCollectionId;

+ (instancetype)sharedCollector;
- (void)queryGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback;
- (void)queryGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback
                          timeoutMs:(int64_t)timeoutMs;
- (void)finishCollection:(uint64_t)collectionId status:(LynxMemoryCollectionStatus)status;
- (void)receiveInstanceResult:(LynxInstanceMemoryUsage *_Nullable)result
                 collectionId:(uint64_t)collectionId;

@end

@implementation LynxMemoryUsageCollector

+ (instancetype)sharedCollector {
  static LynxMemoryUsageCollector *collector = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    collector = [[LynxMemoryUsageCollector alloc] init];
  });
  return collector;
}

- (void)queryGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback {
  [self queryGlobalMemoryUsageAsync:callback timeoutMs:kLynxGlobalMemoryUsageDefaultTimeoutMs];
}

- (void)queryGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback
                          timeoutMs:(int64_t)timeoutMs {
  if (!callback) {
    return;
  }
  int64_t collectionTimeoutMs = LynxNormalizeGlobalMemoryUsageTimeoutMs(timeoutMs);
  // The block crosses an async report-thread hop, so copy it before the caller's stack frame exits.
  LynxGlobalMemoryUsageCallback callbackCopy = [callback copy];
  // Public callers may arrive from any thread. The collector logic posts the entire request setup
  // onto the report thread so collection state, fetcher snapshots, timeout completion,
  // aggregation, and callback delivery share one execution lane.
  [LynxEventReporter
      delayRunOnReportThread:^{
        LynxGlobalMemoryUsageCollectionContext *collectionContext = self.activeCollectionContext;
        if (collectionContext) {
          // Coalescing preserves the active request's snapshot and timeout. The new caller observes
          // the same result as the original caller, which avoids fan-out storms when multiple
          // diagnostics poll at once.
          [collectionContext addCallback:callbackCopy];
          return;
        }

        int64_t collectionStartMs = LynxMemoryUsageNowMs();
        TRACE_EVENT(LYNX_TRACE_CATEGORY, GLOBAL_MEMORY_USAGE_COLLECTOR_QUERY);
        // The weak table is converted to a strong NSArray for this one request.
        // expectedInstanceCount is derived from that fixed snapshot, and every object in it is
        // invoked once below even if lifecycle unregister happens while the request is pending.
        NSArray<id<LynxMemoryUsageFetcher>> *fetchers =
            [LynxMemoryUsageFetchers fetchersForCurrentQuery];
        uint64_t collectionId = ++self.nextCollectionId;
        collectionContext =
            [[LynxGlobalMemoryUsageCollectionContext alloc] initWithCollectionId:collectionId
                                                               collectionStartMs:collectionStartMs
                                                             collectionTimeoutMs:collectionTimeoutMs
                                                           expectedInstanceCount:fetchers.count
                                                                        callback:callbackCopy];
        self.activeCollectionContext = collectionContext;
        if (fetchers.count == 0) {
          // Preserve the public API contract for empty apps: the query still completes
          // asynchronously on the report thread with a normal COMPLETED snapshot.
          [self finishCollection:collectionId status:LynxMemoryCollectionStatusCompleted];
          return;
        }

        // The timeout races with fetcher completions on the same report thread. It captures the
        // collection id from request start, so a stale timeout cannot finish a later request.
        [LynxEventReporter
            delayRunOnReportThread:^{
              [self finishCollection:collectionId status:LynxMemoryCollectionStatusTimeout];
            }
                           delayMs:collectionTimeoutMs];

        for (id<LynxMemoryUsageFetcher> fetcher in fetchers) {
          // Fetchers may complete on arbitrary platform/native threads. Normalize every result back
          // to the report thread before touching request state. Late results after timeout are
          // ignored by collection id, so they cannot mutate a newer query. The single-shot wrapper
          // makes a buggy fetcher harmless if it invokes the callback more than once.
          LynxMemoryUsageFetcherCallback singleShotCallback =
              LynxCreateSingleShotMemoryUsageFetcherCallback(
                  ^(LynxInstanceMemoryUsage *_Nullable result) {
                    [LynxEventReporter
                        delayRunOnReportThread:^{
                          [self receiveInstanceResult:result collectionId:collectionId];
                        }
                                       delayMs:0];
                  });
          @try {
            [fetcher queryMemoryUsageAsync:singleShotCallback];
          } @catch (NSException *exception) {
            LLogError(@"LynxMemoryUsageCollector failed to query memory usage fetcher: %@",
                      exception.reason);
            singleShotCallback(nil);
          }
        }
      }
                     delayMs:0];
}

- (void)finishCollection:(uint64_t)collectionId status:(LynxMemoryCollectionStatus)status {
  LynxGlobalMemoryUsageCollectionContext *collectionContext = self.activeCollectionContext;
  if (![collectionContext matchesCollectionId:collectionId]) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, GLOBAL_MEMORY_USAGE_COLLECTOR_FINISH);

  // Build the public result once, after completion or timeout wins the request. The instance list
  // keeps the completed fetcher samples, while global totals apply the BTS sharing rule below so
  // shared background runtimes are not counted once for every LynxView that reports the same
  // runtime group.
  int64_t elementBytes = 0;
  int32_t elementNodeCount = 0;
  int64_t viewBytes = 0;
  int64_t mainThreadRuntimeBytes = 0;
  int64_t nonSharedBackgroundRuntimeBytes = 0;
  NSMutableDictionary<NSString *, NSNumber *> *sharedBackgroundRuntimeBytesByGroup =
      [NSMutableDictionary dictionary];

  for (LynxInstanceMemoryUsage *instance in collectionContext.instances) {
    elementBytes += instance.elementBytes;
    elementNodeCount += instance.elementNodeCount;
    viewBytes += instance.viewBytes;
    mainThreadRuntimeBytes += instance.mainThreadRuntimeBytes;
    NSString *groupId = instance.btsRuntimeGroupId;
    if (groupId.length > 0 &&
        ![groupId isEqualToString:kLynxMemoryUsageNonSharedBTSRuntimeGroupId]) {
      // Shared BTS runtimes can be sampled by multiple instances. Count one sample per runtime
      // group in the global total. Use the largest sample so weak-table enumeration order cannot
      // change the aggregate, while the instance list still exposes each raw sample for diagnosis.
      int64_t currentBytes = [sharedBackgroundRuntimeBytesByGroup[groupId] longLongValue];
      int64_t maxBytes = std::max(currentBytes, instance.backgroundThreadRuntimeBytes);
      sharedBackgroundRuntimeBytesByGroup[groupId] = @(maxBytes);
    } else {
      nonSharedBackgroundRuntimeBytes += instance.backgroundThreadRuntimeBytes;
    }
  }
  int64_t backgroundThreadRuntimeBytes = nonSharedBackgroundRuntimeBytes;
  for (NSNumber *sharedBackgroundRuntimeBytes in sharedBackgroundRuntimeBytesByGroup.allValues) {
    backgroundThreadRuntimeBytes += sharedBackgroundRuntimeBytes.longLongValue;
  }
  // The public result contract sorts completed instance samples by descending totalBytes so callers
  // can inspect the largest Lynx instances first without reordering the snapshot themselves.
  NSArray<LynxInstanceMemoryUsage *> *sortedInstances = [collectionContext.instances
      sortedArrayUsingComparator:^NSComparisonResult(LynxInstanceMemoryUsage *left,
                                                     LynxInstanceMemoryUsage *right) {
        if (left.totalBytes > right.totalBytes) {
          return NSOrderedAscending;
        }
        if (left.totalBytes < right.totalBytes) {
          return NSOrderedDescending;
        }
        return NSOrderedSame;
      }];

  LynxGlobalMemoryUsageResult *result = [[LynxGlobalMemoryUsageResult alloc] init];
  result.collectionStartMs = collectionContext.collectionStartMs;
  result.collectionStatus = status;
  result.collectionDurationMs = LynxMemoryUsageNowMs() - collectionContext.collectionStartMs;
  result.collectionTimeoutMs = collectionContext.collectionTimeoutMs;
  result.expectedInstanceCount = collectionContext.expectedInstanceCount;
  result.completedInstanceCount = sortedInstances.count;
  result.elementBytes = elementBytes;
  result.elementNodeCount = elementNodeCount;
  result.viewBytes = viewBytes;
  result.mainThreadRuntimeBytes = mainThreadRuntimeBytes;
  result.backgroundThreadRuntimeBytes = backgroundThreadRuntimeBytes;
  result.totalBytes =
      elementBytes + viewBytes + mainThreadRuntimeBytes + backgroundThreadRuntimeBytes;
  int64_t appBytes = LynxMemoryUsageAppBytes();
  result.appBytes = appBytes;
  result.ratioToApp =
      appBytes > 0 ? static_cast<double>(result.totalBytes) / static_cast<double>(appBytes) : 0;
  result.instances = sortedInstances;

  // Detach callbacks and clear pending state before invoking user code. This lets a callback
  // re-enter the API and start a fresh request instead of mutating the already-finished request.
  NSArray<id> *callbacks = [collectionContext copyCallbacksForCompletion];
  self.activeCollectionContext = nil;
  for (id callbackObject in callbacks) {
    LynxGlobalMemoryUsageCallback callback = callbackObject;
    LynxInvokeGlobalMemoryUsageCallbackSafely(callback, result);
  }
}

- (void)receiveInstanceResult:(LynxInstanceMemoryUsage *_Nullable)result
                 collectionId:(uint64_t)collectionId {
  LynxGlobalMemoryUsageCollectionContext *collectionContext = self.activeCollectionContext;
  if (![collectionContext matchesCollectionId:collectionId]) {
    return;
  }
  [collectionContext recordInstanceResult:result];
  // A nil result is still a completed fetcher response. It is excluded from the instance list, but
  // it must advance the received count so a defensive fetcher can fail closed without stalling the
  // whole global request.
  if ([collectionContext hasReceivedAllFetchResults]) {
    [self finishCollection:collectionId status:LynxMemoryCollectionStatusCompleted];
  }
}

@end
