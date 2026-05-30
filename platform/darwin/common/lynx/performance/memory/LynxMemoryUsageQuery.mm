// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>

#import "LynxTraceEventDef.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

@interface LynxMemoryUsageCollector : NSObject

+ (instancetype)sharedCollector;
- (void)queryGlobalMemoryUsageAsync:(nullable LynxGlobalMemoryUsageCallback)callback;

@end

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
  // Keep this singleton as the public facade only. The internal collector logic owns weak fetcher
  // storage, report-thread serialization, timeout, and aggregation details.
  TRACE_EVENT(LYNX_TRACE_CATEGORY, MEMORY_USAGE_QUERY_PUBLIC_API);
  [[LynxMemoryUsageCollector sharedCollector] queryGlobalMemoryUsageAsync:callback];
}

@end
