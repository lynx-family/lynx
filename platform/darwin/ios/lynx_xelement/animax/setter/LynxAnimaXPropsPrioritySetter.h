// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class AnimaXPlayer;

typedef void (^AnimaXPlayerTask)(AnimaXPlayer* _Nonnull player);

typedef NS_ENUM(NSInteger, AnimaXExecutionPriority) {
  AnimaXHigh = -100,
  AnimaXDefault = 0,
  AnimaXLow = 100,
};

NS_ASSUME_NONNULL_BEGIN
@interface LynxAnimaXPropsPrioritySetter : NSObject
- (instancetype)init;

- (void)attachToPlayer:(AnimaXPlayer*)player;
- (void)enqueueTask:(AnimaXPlayerTask)task;

- (void)enqueueTask:(AnimaXPlayerTask)task priority:(AnimaXExecutionPriority)priority;

- (void)flush;
@end
NS_ASSUME_NONNULL_END
