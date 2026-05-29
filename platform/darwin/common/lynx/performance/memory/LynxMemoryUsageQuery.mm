// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>

#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxMemoryUsageResult.h>

@implementation LynxMemoryUsageQuery

+ (instancetype)sharedInstance {
  static LynxMemoryUsageQuery *query = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    query = [[LynxMemoryUsageQuery alloc] init];
  });
  return query;
}

- (void)queryLynxGlobalMemoryUsageAsync:(LynxGlobalMemoryUsageCallback)callback {
  if (!callback) {
    return;
  }
  int64_t collectionStartMs = static_cast<int64_t>([[NSDate date] timeIntervalSince1970] * 1000);
  // This MR only introduces the public API facade. Keep the observable behavior
  // asynchronous and report-thread based, but return an empty completed result
  // until the native collector is wired in by the follow-up implementation.
  [LynxEventReporter
      delayRunOnReportThread:^{
        LynxGlobalMemoryUsageResult *result = [[LynxGlobalMemoryUsageResult alloc] init];
        result.collectionStartMs = collectionStartMs;
        result.collectionStatus = LynxMemoryCollectionStatusCompleted;
        callback(result);
      }
                     delayMs:0];
}

@end
