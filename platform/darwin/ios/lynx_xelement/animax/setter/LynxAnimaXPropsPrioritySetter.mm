// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxAnimaXPropsPrioritySetter.h>

#include "src/base/log/log.h"

@interface AnimaXPrioritizedTask : NSObject
@property(nonatomic, copy) AnimaXPlayerTask task;
@property(nonatomic, assign) AnimaXExecutionPriority priority;
- (instancetype)initWithTask:(AnimaXPlayerTask)task priority:(AnimaXExecutionPriority)priority;
@end

@implementation AnimaXPrioritizedTask
- (instancetype)initWithTask:(AnimaXPlayerTask)task priority:(AnimaXExecutionPriority)priority {
  if ((self = [super init])) {
    _task = [task copy];
    _priority = priority;
  }
  return self;
}
@end

@interface LynxAnimaXPropsPrioritySetter ()
@property(nonatomic, weak) AnimaXPlayer *player;
@property(nonatomic, strong) NSMutableArray<AnimaXPrioritizedTask *> *taskQueue;
@end

@implementation LynxAnimaXPropsPrioritySetter
- (instancetype)init {
  if ((self = [super init])) {
    _taskQueue = [NSMutableArray array];
  }
  return self;
}
- (void)attachToPlayer:(AnimaXPlayer *)player {
  self.player = player;
}

- (void)enqueueTask:(AnimaXPlayerTask)task {
  [self enqueueTask:task priority:AnimaXDefault];
}

- (void)enqueueTask:(AnimaXPlayerTask)task priority:(AnimaXExecutionPriority)priority {
  if (!task) {
    return;
  }
  AnimaXPrioritizedTask *prioritized = [[AnimaXPrioritizedTask alloc] initWithTask:task
                                                                          priority:priority];
  [self.taskQueue addObject:prioritized];
}
- (void)flush {
  AnimaXPlayer *strongPlayer = self.player;
  if (!strongPlayer) {
    return;
  }
  NSArray<AnimaXPrioritizedTask *> *sorted =
      [self.taskQueue sortedArrayUsingComparator:^NSComparisonResult(AnimaXPrioritizedTask *a,
                                                                     AnimaXPrioritizedTask *b) {
        NSInteger av = a.priority;
        NSInteger bv = b.priority;
        if (av < bv) return NSOrderedAscending;
        if (av > bv) return NSOrderedDescending;
        return NSOrderedSame;
      }];
  for (AnimaXPrioritizedTask *pt in sorted) {
    @try {
      if (pt.task) {
        pt.task(strongPlayer);
      }
    } @catch (NSException *exception) {
      ANIMAX_LOGE("Failed to run pending task:" << exception.name
                                                << ", reason: " << exception.reason)
    }
  }
  // Clear the task queue after all tasks are executed
  [self.taskQueue removeAllObjects];
}
@end
