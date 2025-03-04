// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "UIScrollView+LynxGesture.h"

@implementation LynxGestureConsumer

- (instancetype)init {
  if (self = [super init]) {
    _gestureConsumeStatus = LynxGestureConsumeStatusUndefined;
  }
  return self;
}

- (void)consumeGesture:(BOOL)consume {
  _gestureConsumeStatus = consume ? LynxGestureConsumeStatusAllow : LynxGestureConsumeStatusBlock;
}

@end

@interface UIScrollView (LynxGestureDummy)

- (LynxGestureConsumer *)gestureConsumer;

@end

@implementation UIScrollView (LynxGesture)

- (BOOL)respondToScrollViewDidScroll:(LynxGestureConsumer *)gestureConsumer {
  if (!gestureConsumer) {
    return YES;
  }

  CGPoint adjustContentOffset = self.contentOffset;

  if (gestureConsumer.adjustingScrollOffset) {
    // The gestureConsumer is adjusting contentOffset, avoid re-entrance
    return NO;
  } else if (gestureConsumer.gestureConsumeStatus == LynxGestureConsumeStatusUndefined) {
    // Do nothing
    gestureConsumer.previousScrollOffset = adjustContentOffset;
    return YES;
  } else if (gestureConsumer.gestureConsumeStatus == LynxGestureConsumeStatusBlock) {
    // Reset the content offset
    adjustContentOffset = gestureConsumer.previousScrollOffset;
    if (!CGPointEqualToPoint(adjustContentOffset, self.contentOffset)) {
      gestureConsumer.adjustingScrollOffset = YES;
      [self setContentOffset:adjustContentOffset];
      gestureConsumer.adjustingScrollOffset = NO;
      return NO;
    }
  } else if (gestureConsumer.gestureConsumeStatus == LynxGestureConsumeStatusAllow) {
    // Do nothing
    gestureConsumer.previousScrollOffset = adjustContentOffset;
    return YES;
  }

  return YES;
}

- (void)disableGesturesRecursivelyIfNecessary:(LynxGestureConsumer *)gestureConsumer {
  if (gestureConsumer && gestureConsumer.gestureConsumeStatus == LynxGestureConsumeStatusBlock) {
    for (UIView *subview in self.subviews) {
      [self disableGesturesRecursively:subview];
    }
  }
}

- (void)disableGesturesRecursively:(UIView *)view {
  // Disable all gesture recognizers for the current view
  for (UIGestureRecognizer *gestureRecognizer in view.gestureRecognizers) {
    gestureRecognizer.state = UIGestureRecognizerStateFailed;
  }
  for (UIView *subview in view.subviews) {
    [self disableGesturesRecursively:subview];
  }
}

- (BOOL)stopDeceleratingIfNecessaryWithTargetContentOffset:(inout CGPoint *)targetContentOffset {
  LynxGestureConsumer *gestureConsumer = [self tryGetGestureConsumer];
  if (gestureConsumer && gestureConsumer.gestureConsumeStatus == LynxGestureConsumeStatusBlock) {
    targetContentOffset->x = self.contentOffset.x;
    targetContentOffset->y = self.contentOffset.y;
    return YES;
  }
  return NO;
}

- (LynxGestureConsumer *)tryGetGestureConsumer {
  if ([self respondsToSelector:@selector(gestureConsumer)]) {
    LynxGestureConsumer *gestureConsumer = [self gestureConsumer];
    if ([gestureConsumer isKindOfClass:LynxGestureConsumer.class]) {
      return gestureConsumer;
    }
  }
  return nil;
}

@end
