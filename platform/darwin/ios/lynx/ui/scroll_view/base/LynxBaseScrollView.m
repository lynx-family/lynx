// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBaseScrollView.h>
#import <Lynx/UIScrollView+Lynx.h>

#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/LynxBaseScrollView+Public.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxView+Internal.h>

@interface LynxBaseScrollView () <UIScrollViewDelegate>
@property(nonatomic, strong)
    LynxBaseScrollViewScrollFinishedCallback programmaticallyScrollFinishedCallback;
@property(nonatomic, assign) LynxBaseScrollViewScrollState scrollState;
@property(nonatomic, assign) NestedScrollMode forwardsNestedScrollMode;
@property(nonatomic, assign) NestedScrollMode backwardsNestedScrollMode;
@property(nonatomic, assign) CGPoint previousScrollOffset;
@end

@implementation LynxBaseScrollView

- (instancetype)initWithVertical:(BOOL)vertical layoutFromEnd:(BOOL)layoutFromEnd {
  if (self = [super init]) {
    self.autoresizesSubviews = NO;
    self.showsVerticalScrollIndicator = NO;
    self.showsHorizontalScrollIndicator = NO;
    self.delegate = self;
    if (@available(iOS 11.0, *)) {
      self.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
    }
    _vertical = vertical;
    _layoutFromEnd = layoutFromEnd;
    _scrollState = LynxBaseScrollViewScrollStateIdle;
  }
  return self;
}

#pragma mark - Unified API

- (void)autoScrollWithRate:(CGFloat)rate
                  interval:(NSTimeInterval)interval
                  complete:(LynxBaseScrollViewScrollFinishedCallback)complete {
  [self updateProgrammaticallyScrollFinishedCallback:complete];
  __weak __typeof(self) weakSelf = self;
  [self autoScrollWithRate:rate
                  behavior:LynxScrollViewTouchBehaviorStop
                  interval:interval
                  autoStop:YES
                  vertical:self.vertical
                  complete:^BOOL(BOOL scrollEnabledAtStart, BOOL completed) {
                    if (completed) {
                      [weakSelf tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
                    }
                    return scrollEnabledAtStart;
                  }];

  if (![self autoScrollWillReachToTheBounds]) {
    [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateAnimating];
  }
}

#pragma mark - UIScrollViewDelegate
- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
  if (![self triggerLayoutForNestedScrollViews]) {
    [self scrollOffsetUpdated:scrollView];
  }
}

- (void)scrollViewWillBeginDragging:(UIScrollView *)scrollView {
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateDragging];
}

- (void)scrollViewDidEndDragging:(UIScrollView *)scrollView willDecelerate:(BOOL)decelerate {
  if (decelerate) {
    [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateFling];
  } else {
    [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
  }
}

- (void)willMoveToWindow:(UIWindow *)newWindow {
  [super willMoveToWindow:newWindow];
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
}

- (void)scrollViewDidEndScrollingAnimation:(UIScrollView *)scrollView {
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
}

- (void)scrollViewDidEndDecelerating:(UIScrollView *)scrollView {
  [self tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
}

#pragma mark - Internal

- (void)tryToUpdateScrollState:(LynxBaseScrollViewScrollState)newState {
  LynxBaseScrollViewScrollState oldState = _scrollState;
  if (oldState != newState) {
    [self.scrollDelegate scrollStateChangedFrom:oldState to:newState];
    _scrollState = newState;
  }
  if (newState != LynxBaseScrollViewScrollStateAnimating) {
    [self updateProgrammaticallyScrollFinishedCallback:nil];
  }
}

- (void)updateProgrammaticallyScrollFinishedCallback:
    (LynxBaseScrollViewScrollFinishedCallback)callback {
  if (self.programmaticallyScrollFinishedCallback != callback) {
    if (self.programmaticallyScrollFinishedCallback) {
      // if last programmatically scroll was not finished, a new callback is comming, set NO to the
      // previous callback
      self.programmaticallyScrollFinishedCallback(callback ? NO : YES);
    }
    self.programmaticallyScrollFinishedCallback = callback;
  }
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self tryToHandleNestedScroll];
}

- (BOOL)gestureRecognizer:(UIPanGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  if ([otherGestureRecognizer.view isKindOfClass:LynxBaseScrollView.class]) {
    if (self.vertical == ((LynxBaseScrollView *)otherGestureRecognizer.view).vertical) {
      return YES;
    }
  }
  return NO;
}
@end
