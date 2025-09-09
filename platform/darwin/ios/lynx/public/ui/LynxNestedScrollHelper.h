// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxEventHandler;

typedef NS_ENUM(NSInteger, LynxNestedScrollMode) {
  LynxNestedScrollModeSelfOnly = 0,
  LynxNestedScrollModeSelfFirst,
  LynxNestedScrollModeParentFirst,
  LynxNestedScrollModeParallel,
};

@interface LynxNestedScrollHelper : NSObject <UIScrollViewDelegate>

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler;

- (void)resetScrollTargetChain;

- (void)generateScrollTargetChain;

@end

NS_ASSUME_NONNULL_END
