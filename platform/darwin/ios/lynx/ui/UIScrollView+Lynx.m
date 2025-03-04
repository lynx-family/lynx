// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "LynxWeakProxy.h"
#import "UIScrollView+Lynx.h"

@interface LynxCustomScroll : NSObject

@property(readonly, nonatomic, weak) UIScrollView *scrollView;
@property(readonly, nonatomic, strong) CADisplayLink *displayLink;
@property(readonly, nonatomic, assign) NSTimeInterval duration;
@property(readonly, nonatomic, assign) CGPoint start;
@property(readonly, nonatomic, assign) CGPoint dist;
@property(readonly, nonatomic, assign) NSTimeInterval beginTime;
@property(readonly, nonatomic, copy) UIScrollViewLynxCompletion completeBlock;
@property(readonly, nonatomic, copy) UIScrollViewLynxProgressInterception interception;
@property(readonly, nonatomic, copy) UIScrollViewLynxTimingFunction timingFunction;
@property(nonatomic, assign) BOOL originalScrollEnabled;
@property(nonatomic, assign) CGFloat autoScrollRate;
@property(nonatomic, assign) BOOL isAutoScrollVertical;
@property(nonatomic, assign) BOOL isAutoScrollAutoStop;
@property(nonatomic, assign) LynxScrollViewTouchBehavior scrollBehavior;
@end

@implementation LynxCustomScroll

- (instancetype)initWithScrollView:(UIScrollView *)scrollView {
  if (self = [super init]) {
    _scrollView = scrollView;
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(tick:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    _displayLink.paused = YES;
  }
  return self;
}

- (void)startAutoScroll:(NSTimeInterval)interval
               behavior:(LynxScrollViewTouchBehavior)behavior
                   rate:(CGFloat)rate
               autoStop:(BOOL)autoStop
               vertical:(BOOL)isVertical
               complete:(_Nullable UIScrollViewLynxCompletion)callback {
  if (!_displayLink.paused) {
    // if last scroll is not finished, reset scrollEnabled to original value
    self.scrollView.scrollEnabled = _originalScrollEnabled;
  }
  _originalScrollEnabled = self.scrollView.scrollEnabled;
  self.autoScrollRate = rate;
  self.isAutoScrollVertical = isVertical;
  self.isAutoScrollAutoStop = autoStop;
  self.scrollBehavior = behavior;
  _completeBlock = callback;
  if (behavior == LynxScrollViewTouchBehaviorForbid) {
    self.scrollView.scrollEnabled = NO;
  }
  if (@available(iOS 10.0, *)) {
    _displayLink.preferredFramesPerSecond = interval ? (1.0 / interval) : 0;
  } else if (@available(iOS 3.1, *)) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    _displayLink.frameInterval = interval ? (interval / 0.016f) : 1;
#pragma clang diagnostic pop
  }
  _displayLink.paused = NO;
}

- (void)startWithDuration:(NSTimeInterval)duration
                 behavior:(LynxScrollViewTouchBehavior)behavior
                 interval:(NSTimeInterval)interval
                    start:(CGPoint)start
                      end:(CGPoint)end
           timingFunction:(_Nullable UIScrollViewLynxTimingFunction)timingFunction
                 progress:(_Nullable UIScrollViewLynxProgressInterception)interception
                 complete:(_Nullable UIScrollViewLynxCompletion)callback {
  if (!_displayLink.paused) {
    // if last scroll is not finished, reset scrollEnabled to original value
    self.scrollView.scrollEnabled = _originalScrollEnabled;
  }
  if (!CGPointEqualToPoint(start, end)) {
    _originalScrollEnabled = self.scrollView.scrollEnabled;

    switch (behavior) {
      case LynxScrollViewTouchBehaviorForbid:
        self.scrollView.scrollEnabled = NO;
        break;
      default:
        break;
    }
    _completeBlock = callback;
    _interception = interception;
    _timingFunction = timingFunction;
    _duration = duration;
    _beginTime = CFAbsoluteTimeGetCurrent();
    _start = start;
    _dist = CGPointMake(end.x - start.x, end.y - start.y);
    _scrollBehavior = behavior;
    if (@available(iOS 10.0, *)) {
      _displayLink.preferredFramesPerSecond = interval ? (1.0 / interval) : 0;
    } else if (@available(iOS 3.1, *)) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
      _displayLink.frameInterval = interval ? (interval / 0.016f) : 1;
#pragma clang diagnostic pop
    }

    _displayLink.paused = NO;
  } else if (callback) {
    // current offset == target offset, call callback immediately
    callback(self.scrollView.scrollEnabled, YES);
  }
}

- (void)tick:(CADisplayLink *)displayLink {
  if (self.autoScrollRate) {
    [self autoScrollTick];
    return;
  }

  switch (self.scrollBehavior) {
    case LynxScrollViewTouchBehaviorForbid:
      break;
    default:
      if (self.scrollView.tracking) {
        [self stop:NO];
        return;
      }
  }

  double timeProgress = MIN(1.0, (CFAbsoluteTimeGetCurrent() - _beginTime) / _duration);

  double progress =
      _timingFunction ? _timingFunction(timeProgress) : [self easeInEaseOut:timeProgress];

  CGFloat x = _start.x + _dist.x * progress;
  CGFloat y = _start.y + _dist.y * progress;

  CGPoint targetOffset = CGPointMake(x, y);

  if (_interception) {
    targetOffset = _interception(timeProgress, progress, targetOffset);
  }

  [self.scrollView setContentOffset:targetOffset];

  if (progress >= 1.0) {
    [self stop:YES];
  }
}

- (void)autoScrollTick {
  switch (self.scrollBehavior) {
    case LynxScrollViewTouchBehaviorPause:
      if (self.scrollView.tracking) {
        return;
      }
      break;
    case LynxScrollViewTouchBehaviorStop:
      if (self.scrollView.tracking) {
        [self stop:NO];
        return;
      }
      break;
    case LynxScrollViewTouchBehaviorNone:
    case LynxScrollViewTouchBehaviorForbid:
      break;
  }

  CGPoint targetContentOffset = self.scrollView.contentOffset;
  BOOL reachToTheBounds = [self autoScrollWillReachToTheBounds:&targetContentOffset];

  [self.scrollView setContentOffset:targetContentOffset];

  if (reachToTheBounds && self.isAutoScrollAutoStop) {
    [self stop:YES];
  }
}

- (BOOL)autoScrollWillReachToTheBounds:(inout CGPoint *)targetContentOffset {
  if (self.isAutoScrollVertical) {
    targetContentOffset->y += self.autoScrollRate;

    // don not scroll beyond bounce
    CGFloat bottomMost = self.scrollView.contentSize.height - self.scrollView.frame.size.height +
                         self.scrollView.contentInset.bottom;
    CGFloat topMost = -self.scrollView.contentInset.top;
    CGFloat lower = MAX(topMost, bottomMost);
    CGFloat upper = topMost;

    if (targetContentOffset->y <= upper) {
      targetContentOffset->y = upper;
      return YES;
    } else if (targetContentOffset->y >= lower) {
      targetContentOffset->y = lower;
      return YES;
    }
  } else {
    targetContentOffset->x += self.autoScrollRate;
    // don not scroll beyond bounce
    CGFloat rightMost = self.scrollView.contentSize.width - self.scrollView.frame.size.width +
                        self.scrollView.contentInset.right;
    CGFloat leftMost = -self.scrollView.contentInset.left;
    CGFloat lower = MAX(leftMost, rightMost);
    CGFloat upper = leftMost;

    if (targetContentOffset->x <= upper) {
      targetContentOffset->x = upper;
      return YES;
    } else if (targetContentOffset->x >= lower) {
      targetContentOffset->x = lower;
      return YES;
    }
  }
  return NO;
}

- (void)stop:(BOOL)completed {
  if (!_displayLink.paused) {
    BOOL willScrollEnabled = _originalScrollEnabled;
    if (_completeBlock) {
      willScrollEnabled = _completeBlock(willScrollEnabled, completed);
    }
    _completeBlock = nil;
    _interception = nil;
    _timingFunction = nil;
    switch (_scrollBehavior) {
      case LynxScrollViewTouchBehaviorForbid:
        self.scrollView.scrollEnabled = willScrollEnabled;
        break;
      default:
        break;
    }
    _displayLink.paused = YES;
    [_displayLink invalidate];
  }
}

- (double)easeInEaseOut:(double)p {
  if (p < 0.2) {
    return 2 * p * p;
  } else if (p > 0.8) {
    return (-2 * p * p) + (4 * p) - 1;
  } else {
    return 0.08 + (p - 0.2) * 1.4;
  }
}

- (void)dealloc {
  // If customScroll dealloced, mark it as completed.
  [self stop:YES];
}

@end

@implementation UIScrollView (Lynx)

- (void)setLynxListAdjustingContentOffset:(BOOL)value {
}

- (BOOL)isLynxListAdjustingContentOffset {
  return NO;
}

- (LynxCustomScroll *)lynxCustomScroll {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setLynxCustomScroll:(LynxCustomScroll *)customScroll {
  objc_setAssociatedObject(self, @selector(lynxCustomScroll), customScroll,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)consumeDeltaOffset:(CGPoint)delta vertical:(BOOL)vertical {
  CGPoint offset = self.contentOffset;
  if (vertical) {
    if (delta.y > 0) {  // scroll to lower
      if (offset.y >= self.contentSize.height - self.bounds.size.height + self.contentInset.bottom -
                          1.0f) {  // 1.0f is epsilon to handle height / bottom is a float number
        return NO;
      }
    } else if (delta.y < 0) {  // scroll to upper
      if (offset.y <= -self.contentInset.top) {
        return NO;
      }
    }
  } else {
    if (delta.x > 0) {  // scroll to right
      if (offset.x >= self.contentSize.width - self.bounds.size.width + self.contentInset.right -
                          1.0f) {  // 1.0f is epsilon to handle height / bottom is a float number
        return NO;
      }
    } else if (delta.x < 0) {
      if (offset.x <= -self.contentInset.left) {
        return NO;
      }
    }
  }
  return YES;
}

- (CGPoint)updateContentOffset:(CGPoint)contentOffset vertical:(BOOL)vertical {
  if (vertical) {
    contentOffset.y =
        MIN(MAX(contentOffset.y, -self.contentInset.top),
            MAX(0, self.contentSize.height - self.bounds.size.height + self.contentInset.bottom));
    contentOffset.x = self.contentOffset.x;
  } else {
    contentOffset.x =
        MIN(MAX(contentOffset.x, -self.contentInset.left),
            MAX(0, self.contentSize.width - self.bounds.size.width + self.contentInset.right));
    contentOffset.y = self.contentOffset.y;
  }

  [self setContentOffset:contentOffset];

  return contentOffset;
}

- (void)setContentOffset:(CGPoint)contentOffset
                behavior:(LynxScrollViewTouchBehavior)behavior
                duration:(NSTimeInterval)duration
                interval:(NSTimeInterval)interval
                progress:(_Nullable UIScrollViewLynxProgressInterception)interception
                complete:(_Nullable UIScrollViewLynxCompletion)callback {
  [[self lynxCustomScroll] stop:NO];

  [self setLynxCustomScroll:[[LynxCustomScroll alloc] initWithScrollView:self]];

  [[self lynxCustomScroll] startWithDuration:duration
                                    behavior:behavior
                                    interval:interval
                                       start:self.contentOffset
                                         end:contentOffset
                              timingFunction:nil
                                    progress:interception
                                    complete:callback];
}

- (void)autoScrollWithRate:(CGFloat)rate
                  behavior:(LynxScrollViewTouchBehavior)behavior
                  interval:(NSTimeInterval)interval
                  autoStop:(BOOL)autoStop
                  vertical:(BOOL)isVertical
                  complete:(_Nullable UIScrollViewLynxCompletion)callback {
  [[self lynxCustomScroll] stop:NO];

  [self setLynxCustomScroll:[[LynxCustomScroll alloc] initWithScrollView:self]];

  [[self lynxCustomScroll] startAutoScroll:interval
                                  behavior:behavior
                                      rate:rate
                                  autoStop:autoStop
                                  vertical:isVertical
                                  complete:callback];
}

- (BOOL)autoScrollWillReachToTheBounds {
  CGPoint currentContentOffset = self.contentOffset;
  return [[self lynxCustomScroll] autoScrollWillReachToTheBounds:&currentContentOffset];
}

- (void)scrollToTargetContentOffset:(CGPoint)contentOffset
                           behavior:(LynxScrollViewTouchBehavior)behavior
                           duration:(NSTimeInterval)duration
                           interval:(NSTimeInterval)interval
                           complete:(_Nullable UIScrollViewLynxCompletion)callback {
  [[self lynxCustomScroll] stop:NO];

  [self setLynxCustomScroll:[[LynxCustomScroll alloc] initWithScrollView:self]];

  [[self lynxCustomScroll] startWithDuration:duration
                                    behavior:behavior
                                    interval:interval
                                       start:self.contentOffset
                                         end:contentOffset
                              timingFunction:^double(double input) {
                                // ease out
                                return 1 - pow(1 - input, 4);
                              }
                                    progress:nil
                                    complete:callback];
}

- (void)stopScroll {
  // called when autoScroll stopped by UIMethod param 'start' = NO
  [[self lynxCustomScroll] stop:YES];
}

- (void)setScrollEnableFromLynx:(BOOL)value {
  objc_setAssociatedObject(self, @selector(scrollEnableFromLynx), @(value),
                           OBJC_ASSOCIATION_ASSIGN);
}

- (BOOL)scrollEnableFromLynx {
  return [objc_getAssociatedObject(self, _cmd) boolValue];
}

- (CGPoint)distanceToItem:(CGRect)itemFrame
                     size:(CGSize)scrollViewSize
            contentOffset:(CGPoint)contentOffset
                 vertical:(BOOL)vertical
                   factor:(CGFloat)factor
                   offset:(CGFloat)offset {
  if (vertical) {
    return CGPointMake(0, CGRectGetMinY(itemFrame) + CGRectGetHeight(itemFrame) * factor -
                              (contentOffset.y + scrollViewSize.height * factor) + offset);
  } else {
    return CGPointMake(CGRectGetMinX(itemFrame) + CGRectGetWidth(itemFrame) * factor -
                           (contentOffset.x + scrollViewSize.width * factor) + offset,
                       0);
  }
}

- (CGPoint)targetContentOffset:(CGPoint)proposedContentOffset
         withScrollingVelocity:(CGPoint)velocity
              withVisibleItems:(NSArray<UIView *> *)visibleItems
              getIndexFromView:(UIScrollViewGetIndexFromView)getIndexFromView
            getViewRectAtIndex:(UIScrollViewGetViewRectAtIndex)getViewRectAtIndex
                      vertical:(BOOL)vertical
                           rtl:(BOOL)rtl
                        factor:(CGFloat)factor
                        offset:(CGFloat)offset
                      callback:(UIScrollViewWillSnapToCallback)callback {
  BOOL forward = vertical ? velocity.y >= 0 : velocity.x >= 0;
  BOOL hasVelocity = vertical ? velocity.y != 0 : velocity.x != 0;

  // A child that is exactly in the position is eligible for both before and after
  __block UIView *closestBeforePosition = nil;
  __block UIView *closestAfterPosition = nil;
  __block CGFloat distanceBefore = -CGFLOAT_MAX;
  __block CGFloat distanceAfter = CGFLOAT_MAX;

  CGRect scrollviewRect = self.frame;
  CGPoint contentOffset = self.contentOffset;

  // Find the first view before the position, and the first view after the position
  [visibleItems enumerateObjectsUsingBlock:^(__kindof UIView *_Nonnull obj, NSUInteger idx,
                                             BOOL *_Nonnull stop) {
    CGPoint distancePoint = [self distanceToItem:obj.frame
                                            size:scrollviewRect.size
                                   contentOffset:contentOffset
                                        vertical:vertical
                                          factor:factor
                                          offset:offset];
    CGFloat distance = vertical ? distancePoint.y : distancePoint.x;
    if (distance <= 0 && distance > distanceBefore) {
      // Child is before the position and closer then the previous best
      distanceBefore = distance;
      closestBeforePosition = obj;
    }
    if (distance >= 0 && distance < distanceAfter) {
      // Child is after the position and closer then the previous best
      distanceAfter = distance;
      closestAfterPosition = obj;
    }
  }];

  NSInteger targetPosition = -1;

  CGRect targetFrame = CGRectNull;

  UIView *targetView = nil;

  if (!hasVelocity) {
    if (closestAfterPosition && closestBeforePosition) {
      if (ABS(distanceAfter) < ABS(distanceBefore)) {
        targetView = closestAfterPosition;
      } else {
        targetView = closestBeforePosition;
      }
    } else if (closestAfterPosition) {
      targetView = closestAfterPosition;

    } else if (closestBeforePosition) {
      targetView = closestBeforePosition;
    }
  } else {
    // Return the position of the first child from the position, in the direction of the fling
    if (forward && closestAfterPosition) {
      targetView = closestAfterPosition;
    } else if (!forward && closestBeforePosition) {
      targetView = closestBeforePosition;
    }
  }

  if (targetView) {
    if (targetView == closestAfterPosition) {
      targetFrame = closestAfterPosition.frame;
      targetPosition = getIndexFromView(closestAfterPosition);
    } else if (targetView == closestBeforePosition) {
      targetFrame = closestBeforePosition.frame;
      targetPosition = getIndexFromView(closestBeforePosition);
    }
  }

  // There is no child in the direction of the fling. Either it doesn't exist (start/end of the
  // list), or it is not yet attached (very rare case when children are larger then the viewport).
  // Extrapolate from the child that is visible to get the position of the view to snap to.
  if (!CGRectIsNull(targetFrame)) {
    CGPoint targetOffset = CGPointZero;
    if (vertical) {
      targetOffset = CGPointMake(
          contentOffset.x,
          CGRectGetMinY(targetFrame) -
              (CGRectGetHeight(scrollviewRect) - CGRectGetHeight(targetFrame)) * factor + offset);
    } else {
      targetOffset = CGPointMake(
          CGRectGetMinX(targetFrame) -
              (CGRectGetWidth(scrollviewRect) - CGRectGetWidth(targetFrame)) * factor + offset,
          contentOffset.y);
    }

    callback(targetPosition, targetOffset);

    return targetOffset;
    ;
  }

  UIView *visibleView = forward ? closestBeforePosition : closestAfterPosition;

  if (!visibleView) {
    callback(-1, proposedContentOffset);
    return proposedContentOffset;
  }

  BOOL forwardWithRTL = vertical ? forward : (rtl ? !forward : forward);

  targetPosition = getIndexFromView(visibleView) + (forwardWithRTL ? 1 : -1);

  if (targetPosition < 0) {
    targetPosition = 0;
  }

  targetFrame = getViewRectAtIndex(targetPosition);

  if (CGRectIsNull(targetFrame)) {
    callback(-1, proposedContentOffset);
    return proposedContentOffset;
  }

  CGPoint targetOffset = CGPointZero;
  if (vertical) {
    targetOffset = CGPointMake(
        contentOffset.x,
        CGRectGetMinY(targetFrame) -
            (CGRectGetHeight(scrollviewRect) - CGRectGetHeight(targetFrame)) * factor + offset);
  } else {
    targetOffset = CGPointMake(
        CGRectGetMinX(targetFrame) -
            (CGRectGetWidth(scrollviewRect) - CGRectGetWidth(targetFrame)) * factor + offset,
        contentOffset.y);
  }

  callback(targetPosition, targetOffset);
  return targetOffset;
}

@end
