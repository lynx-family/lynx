// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>

#import <Lynx/LynxLog.h>

#include <algorithm>
#include <utility>

#include "base/include/memory/process_memory_info.h"
#include "core/services/performance/memory_monitor/global_memory_monitor.h"

namespace {

constexpr int64_t kLynxGlobalMemoryUsageDefaultTimeoutMs = 2000;

int64_t LynxMemoryUsageNowMs() {
  return static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000);
}

NSString *LynxMemoryUsageString(const std::string &value) {
  return [[NSString alloc] initWithBytes:value.data()
                                  length:value.size()
                                encoding:NSUTF8StringEncoding]
             ?: @"";
}

LynxGlobalMemoryUsageResult *LynxBuildGlobalMemoryUsageResult(
    int64_t collectionStartMs, int64_t collectionTimeoutMs,
    lynx::tasm::performance::GlobalMemoryUsage usage) {
  NSMutableArray<LynxInstanceMemoryUsage *> *instances =
      [NSMutableArray arrayWithCapacity:usage.instances.size()];
  for (const auto &source : usage.instances) {
    LynxInstanceMemoryUsage *instance = [[LynxInstanceMemoryUsage alloc] init];
    instance.instanceId = source.instance_id;
    instance.pageId = LynxMemoryUsageString(source.page_id);
    instance.url = LynxMemoryUsageString(source.url);
    instance.totalBytes = source.page.total_bytes;
    instance.elementBytes = source.page.element_bytes;
    instance.elementNodeCount = source.page.element_count;
    instance.viewBytes = source.page.ui_bytes;
    instance.viewDetail = @{};
    instance.mainThreadRuntimeBytes = source.page.mts_bytes;
    instance.backgroundThreadRuntimeBytes = source.page.bts_bytes;
    instance.btsRuntimeGroupId = LynxMemoryUsageString(source.bts_runtime_group_id);
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
  result.collectionStatus = LynxMemoryCollectionStatusCompleted;
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
  result.appBytes = std::max<int64_t>(0, lynx::base::GetProcessPssBytes());
  result.ratioToApp =
      result.appBytes > 0 ? static_cast<double>(result.totalBytes) / result.appBytes : 0;
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
            callbackCopy, LynxBuildGlobalMemoryUsageResult(collectionStartMs, collectionTimeoutMs,
                                                           std::move(usage)));
      });
}

@end
