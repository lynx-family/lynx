// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Used to let lynx native gesture block the scrolling of UIScrollView

typedef NS_ENUM(NSInteger, LynxGestureConsumeStatus) {
  LynxGestureConsumeStatusUndefined = -1,
  LynxGestureConsumeStatusBlock = 0,
  LynxGestureConsumeStatusAllow = 1,
};

@interface LynxGestureConsumer : NSObject
@property(nonatomic, assign) BOOL adjustingScrollOffset;
@property(nonatomic, assign, readonly) LynxGestureConsumeStatus gestureConsumeStatus;
@property(nonatomic, assign) CGPoint previousScrollOffset;

- (void)consumeGesture:(BOOL)consume;

@end

@interface UIScrollView (LynxGesture)

- (BOOL)respondToScrollViewDidScroll:(LynxGestureConsumer *)gestureConsumer;

- (void)disableGesturesRecursivelyIfNecessary:(LynxGestureConsumer *)gestureConsumer;

- (BOOL)stopDeceleratingIfNecessaryWithTargetContentOffset:(inout CGPoint *)targetContentOffset;

@end

NS_ASSUME_NONNULL_END
