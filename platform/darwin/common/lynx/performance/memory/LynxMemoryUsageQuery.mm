// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>

#import <Lynx/LynxLog.h>

#include <algorithm>
#include <string_view>

#include "base/include/memory/process_memory_info.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"

namespace {

constexpr int64_t kLynxGlobalMemoryUsageDefaultTimeoutMs = 2000;

int64_t LynxMemoryUsageNowMs() {
  return static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000);
}

NSString *LynxMemoryUsageString(std::string_view value) {
  return [[NSString alloc] initWithBytes:value.data()
                                  length:value.size()
                                encoding:NSUTF8StringEncoding]
             ?: @"";
}

LynxInstanceMemoryUsage *LynxBuildInstanceMemoryUsage(
    const lynx::tasm::performance::InstanceMemoryUsage &source) {
  LynxInstanceMemoryUsage *instance = [[LynxInstanceMemoryUsage alloc] init];
  instance.instanceId = source.instance_id;
  instance.pageId = LynxMemoryUsageString(source.page_id);
  instance.url = LynxMemoryUsageString(source.url);
  instance.btsRuntimeGroupId = LynxMemoryUsageString(source.bts_runtime_group_id);
  instance.btsRuntimeType = LynxMemoryUsageString(source.bts_runtime_type);
  instance.btsRuntimeTotalBytes = source.bts_heap_bytes;
  return instance;
}

LynxGlobalMemoryUsageResult *LynxBuildGlobalMemoryUsageResult(
    int64_t collectionStartMs, int64_t collectionTimeoutMs,
    const lynx::tasm::performance::GlobalMemoryUsage &usage) {
  NSMutableArray<LynxInstanceMemoryUsage *> *instances =
      [NSMutableArray arrayWithCapacity:usage.instances.size()];
  for (const auto &source : usage.instances) {
    LynxInstanceMemoryUsage *instance = LynxBuildInstanceMemoryUsage(source);
    instance.totalBytes = source.page.total_bytes;
    instance.elementBytes = source.page.element_bytes;
    instance.elementNodeCount = source.page.element_count;
    instance.viewBytes = source.page.ui_bytes;
    instance.viewDetail = @{};
    instance.mainThreadRuntimeBytes = source.page.mts_bytes;
    instance.backgroundThreadRuntimeBytes = source.page.bts_bytes;
    [instances addObject:instance];
  }
  [instances sortUsingComparator:^NSComparisonResult(LynxInstanceMemoryUsage *left,
                                                     LynxInstanceMemoryUsage *right) {
    if (left.totalBytes > right.totalBytes) return NSOrderedAscending;
    if (left.totalBytes < right.totalBytes) return NSOrderedDescending;
    return NSOrderedSame;
  }];

  LynxGlobalMemoryUsageResult *result = [[LynxGlobalMemoryUsageResult alloc] init];
  result.collectionStartMs = collectionStartMs;
  const bool completed = usage.status == lynx::tasm::performance::MemoryUsageQueryStatus::kOk;
  result.collectionStatus =
      completed ? LynxMemoryCollectionStatusCompleted : LynxMemoryCollectionStatusError;
  if (!completed) {
    result.errorMessage =
        LynxMemoryUsageString(lynx::tasm::performance::kMemoryMonitoringDisabledError);
  }
  result.collectionDurationMs = LynxMemoryUsageNowMs() - collectionStartMs;
  result.collectionTimeoutMs = collectionTimeoutMs;
  result.expectedInstanceCount = instances.count;
  result.completedInstanceCount = instances.count;
  result.totalBytes = usage.total.total_bytes;
  result.elementBytes = usage.total.element_bytes;
  result.elementNodeCount = usage.total.element_count;
  result.viewBytes = usage.total.ui_bytes;
  result.mainThreadRuntimeBytes = usage.total.mts_bytes;
  result.backgroundThreadRuntimeBytes = usage.total.bts_bytes;
  result.instances = instances;
  NSMutableArray<LynxInstanceMemoryUsage *> *destroyedInstances =
      [NSMutableArray arrayWithCapacity:usage.destroyed_instances.size()];
  for (const auto &source : usage.destroyed_instances) {
    LynxInstanceMemoryUsage *instance = LynxBuildInstanceMemoryUsage(source);
    instance.backgroundThreadRuntimeResidualBytes = source.bts_residual_bytes;
    [destroyedInstances addObject:instance];
  }
  result.destroyedInstances = destroyedInstances;
  result.appBytes = completed ? std::max<int64_t>(0, lynx::base::GetProcessPssBytes()) : 0;
  return result;
}

void LynxInvokeGlobalMemoryUsageCallback(LynxGlobalMemoryUsageCallback callback,
                                         LynxGlobalMemoryUsageResult *result) {
  @try {
    callback(result);
  } @catch (NSException *exception) {
    LLogError(@"LynxMemoryUsageQuery callback failed: %@", exception.reason);
  }
}

}  // namespace

@implementation LynxMemoryUsageQuery

+ (instancetype)sharedInstance {
  static LynxMemoryUsageQuery *query = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    query = [[LynxMemoryUsageQuery alloc] init];
  });
  return query;
}

- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback {
  [self queryLynxGlobalMemoryUsageAsync:callback timeoutMs:0];
}

- (void)queryLynxGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback
                              timeoutMs:(int64_t)timeoutMs {
  if (!callback) return;
  LynxGlobalMemoryUsageCallback callbackCopy = [callback copy];
  const int64_t collectionStartMs = LynxMemoryUsageNowMs();
  const int64_t collectionTimeoutMs =
      timeoutMs > 0 ? timeoutMs : kLynxGlobalMemoryUsageDefaultTimeoutMs;
  lynx::tasm::performance::GlobalMemoryMonitor::GetInstance().GetGlobalMemoryUsage(
      [callbackCopy, collectionStartMs,
       collectionTimeoutMs](lynx::tasm::performance::GlobalMemoryUsage usage) {
        LynxInvokeGlobalMemoryUsageCallback(
            callbackCopy,
            LynxBuildGlobalMemoryUsageResult(collectionStartMs, collectionTimeoutMs, usage));
      });
}

@end
