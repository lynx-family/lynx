// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Horizontal.h>
#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/LynxBaseScrollView+Vertical.h>
#import <Lynx/UIScrollView+Lynx.h>

typedef NS_ENUM(NSUInteger, LynxNestedScrollDirection) {
  LynxNestedScrollDirectionNone = 0,
  LynxNestedScrollDirectionForwards,
  LynxNestedScrollDirectionBackwards,
};

@implementation LynxBaseScrollView (Nested)

- (NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)getHitTestChainForNestedScrollViews {
  return (NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)
      [self.scrollDelegate getHitTestChainForNestedScrollViews];
}

- (BOOL)triggerLayoutForNestedScrollViews {
  NSArray<UIScrollView<LynxNestedScrollProtocol> *> *nestedScrollViews =
      [self getHitTestChainForNestedScrollViews];
  __block BOOL hasNestedScrollBehaviors = NO;
  [nestedScrollViews
      enumerateObjectsUsingBlock:^(UIScrollView<LynxNestedScrollProtocol> *_Nonnull obj,
                                   NSUInteger idx, BOOL *_Nonnull stop) {
        if ([obj forwardsNestedScrollMode] != NestedScrollModeSelfOnly ||
            [obj backwardsNestedScrollMode] != NestedScrollModeSelfOnly) {
          hasNestedScrollBehaviors = YES;
          *stop = YES;
        }
      }];
  if (hasNestedScrollBehaviors) {
    [nestedScrollViews
        enumerateObjectsUsingBlock:^(UIScrollView<LynxNestedScrollProtocol> *_Nonnull obj,
                                     NSUInteger idx, BOOL *_Nonnull stop) {
          [obj setNeedsLayout];
        }];
  }
  return hasNestedScrollBehaviors;
}

- (void)tryToHandleNestedScroll {
  NSArray<UIScrollView<LynxNestedScrollProtocol> *> *hitTestChain =
      [self getHitTestChainForNestedScrollViews];
  if (self == hitTestChain.firstObject) {
    [LynxBaseScrollView handleNestedScroll:self with:hitTestChain];
    [hitTestChain enumerateObjectsUsingBlock:^(UIScrollView<LynxNestedScrollProtocol> *_Nonnull obj,
                                               NSUInteger idx, BOOL *_Nonnull stop) {
      if (!CGPointEqualToPoint(obj.previousScrollOffset, obj.contentOffset)) {
        obj.previousScrollOffset = obj.contentOffset;
        [obj scrollOffsetUpdated:obj];
      }
    }];
  }
}

+ (NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)generateNestedScrollChainWithHitTestTarget:
    (UIView *)view {
  NSMutableArray<UIScrollView<LynxNestedScrollProtocol> *> *result = [NSMutableArray array];

  while (view) {
    if ([view isKindOfClass:UIScrollView.class] &&
        [view conformsToProtocol:@protocol(LynxNestedScrollProtocol)]) {
      [result addObject:(UIScrollView<LynxNestedScrollProtocol> *)view];
    }
    view = view.superview;
  }

  return [result copy];
}

+ (void)rollBackParentsScrollOffset:(UIScrollView<LynxNestedScrollProtocol> *)scrollView
                              toTop:(NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)
                                        hittestChain {
  NSEnumerator *enumerator = [hittestChain objectEnumerator];
  UIScrollView<LynxNestedScrollProtocol> *item;

  while (item = [enumerator nextObject]) {
    // Find scrollView itself
    if (item == scrollView) {
      break;
    }
  }

  while (item = [enumerator nextObject]) {
    [LynxBaseScrollView rollBackScrollOffset:item];
  }
}

+ (void)rollBackScrollOffset:(UIScrollView<LynxNestedScrollProtocol> *)scrollView {
  if (!CGPointEqualToPoint(scrollView.contentOffset, [scrollView previousScrollOffset])) {
    scrollView.contentOffset = [scrollView previousScrollOffset];
  }
}

+ (CGPoint)getDeltaScrollOffset:(UIScrollView<LynxNestedScrollProtocol> *)scrollView {
  return CGPointMake(scrollView.contentOffset.x - [scrollView previousScrollOffset].x,
                     scrollView.contentOffset.y - [scrollView previousScrollOffset].y);
}

+ (BOOL)isBouncing:(LynxBaseScrollView *)scrollView {
  CGFloat scrollRange[2] = {0, 0};
  CGFloat currentOffset = scrollView.vertical ? [scrollView getScrollOffsetVertically]
                                              : [scrollView getScrollOffsetHorizontally];
  if (scrollView.vertical) {
    [scrollView getScrollRangeVertically:&scrollRange];
  } else {
    [scrollView getScrollRangeHorizontally:&scrollRange];
  }
  CGFloat epsilon = 1.0 / UIScreen.mainScreen.scale;
  return currentOffset < scrollRange[0] - epsilon || currentOffset > scrollRange[1] + epsilon;
}

+ (LynxNestedScrollDirection)directionFromAxisDelta:(CGFloat)delta {
  CGFloat epsilon = 1.0 / UIScreen.mainScreen.scale;
  if (delta > epsilon) {
    return LynxNestedScrollDirectionForwards;
  }
  if (delta < -epsilon) {
    return LynxNestedScrollDirectionBackwards;
  }
  return LynxNestedScrollDirectionNone;
}

+ (LynxNestedScrollDirection)directionFromVelocity:(CGPoint)velocity vertical:(BOOL)vertical {
  // UIPanGestureRecognizer velocity follows the finger, so content moves in the opposite
  // direction.
  return [LynxBaseScrollView directionFromAxisDelta:-(vertical ? velocity.y : velocity.x)];
}

+ (LynxNestedScrollDirection)currentScrollDirectionOf:(LynxBaseScrollView *)scrollView {
  BOOL vertical = scrollView.vertical;
  CGPoint delta = CGPointZero;
  delta = scrollView.lynxLastScrollDelta;
  if (CGPointEqualToPoint(delta, CGPointZero)) {
    delta = [LynxBaseScrollView getDeltaScrollOffset:scrollView];
  }

  CGFloat axisDelta = vertical ? delta.y : delta.x;
  LynxNestedScrollDirection direction = [LynxBaseScrollView directionFromAxisDelta:axisDelta];
  if (direction != LynxNestedScrollDirectionNone) {
    return direction;
  }

  if ([LynxBaseScrollView isBouncing:scrollView]) {
    return LynxNestedScrollDirectionNone;
  }

  return [LynxBaseScrollView
      directionFromVelocity:[scrollView.panGestureRecognizer velocityInView:scrollView]
                   vertical:vertical];
}

+ (LynxNestedScrollDirection)releaseScrollDirectionOf:(LynxBaseScrollView *)scrollView
                                  targetContentOffset:(CGPoint)targetContentOffset
                                             velocity:(CGPoint)velocity {
  BOOL vertical = scrollView.vertical;

  if ([LynxBaseScrollView isBouncing:scrollView]) {
    return [LynxBaseScrollView directionFromVelocity:velocity vertical:vertical];
  }

  CGFloat currentOffset =
      vertical ? [scrollView getScrollOffsetVertically] : [scrollView getScrollOffsetHorizontally];
  CGFloat targetOffset = vertical ? targetContentOffset.y : targetContentOffset.x;
  CGFloat projectedDelta = targetOffset - currentOffset;
  LynxNestedScrollDirection direction = [LynxBaseScrollView directionFromAxisDelta:projectedDelta];
  if (direction != LynxNestedScrollDirectionNone) {
    return direction;
  }

  CGFloat lastScrollAxisDelta =
      vertical ? scrollView.lynxLastScrollDelta.y : scrollView.lynxLastScrollDelta.x;
  direction = [LynxBaseScrollView directionFromAxisDelta:lastScrollAxisDelta];
  if (direction != LynxNestedScrollDirectionNone) {
    return direction;
  }

  return [LynxBaseScrollView directionFromVelocity:velocity vertical:vertical];
}

- (void)stopSameDirectionParentFlingWithTargetContentOffset:(CGPoint)targetContentOffset
                                                   velocity:(CGPoint)velocity {
  LynxNestedScrollDirection direction =
      [LynxBaseScrollView releaseScrollDirectionOf:self
                               targetContentOffset:targetContentOffset
                                          velocity:velocity];
  if (direction == LynxNestedScrollDirectionNone) {
    return;
  }

  NestedScrollMode mode = direction == LynxNestedScrollDirectionForwards
                              ? self.forwardsNestedScrollMode
                              : self.backwardsNestedScrollMode;
  if (mode != NestedScrollModeSelfOnly) {
    return;
  }

  BOOL foundSelf = NO;
  for (LynxBaseScrollView *parent in (
           NSArray<LynxBaseScrollView *> *)[self getHitTestChainForNestedScrollViews]) {
    if (!foundSelf) {
      foundSelf = parent == self;
      continue;
    }
    if (parent.vertical != self.vertical) {
      continue;
    }
    [parent setContentOffset:parent.contentOffset animated:NO];
    [parent tryToUpdateScrollState:LynxBaseScrollViewScrollStateIdle];
    [parent stopScroll];
  }
}

+ (UIScrollView<LynxNestedScrollProtocol> *)
    findAParentHasScroll:(UIScrollView<LynxNestedScrollProtocol> *)scrollView
                    with:(NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)hittestChain {
  NSEnumerator *enumerator = [hittestChain objectEnumerator];

  UIScrollView<LynxNestedScrollProtocol> *item;

  while (item = [enumerator nextObject]) {
    // Find scrollView itself
    if (item == scrollView) {
      break;
    }
  }

  while (item = [enumerator nextObject]) {
    CGPoint delta = [LynxBaseScrollView getDeltaScrollOffset:item];
    if (!CGPointEqualToPoint(delta, CGPointZero)) {
      return item;
    }
  }

  return nil;
}

+ (UIScrollView<LynxNestedScrollProtocol> *)
    getParentScrollViewOf:(UIScrollView<LynxNestedScrollProtocol> *)scrollView
                     with:(NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)hittestChain {
  NSEnumerator *enumerator = [hittestChain objectEnumerator];

  UIScrollView<LynxNestedScrollProtocol> *item;

  while (item = [enumerator nextObject]) {
    // Find scrollView itself
    if (item == scrollView) {
      break;
    }
  }

  return [enumerator nextObject];
}

+ (BOOL)handleNestedScroll:(UIScrollView<LynxNestedScrollProtocol> *)scrollView
                      with:(NSArray<UIScrollView<LynxNestedScrollProtocol> *> *)hittestChain {
  // Return if the scrollView has consumed some scroll

  BOOL vertical = scrollView.vertical;
  // Find the scrollable view, get its direction.
  CGPoint delta = [LynxBaseScrollView getDeltaScrollOffset:scrollView];
  BOOL forwards = vertical ? delta.y > 0 : delta.x > 0;
  BOOL backwards = vertical ? delta.y < 0 : delta.x < 0;

  if (!forwards && !backwards) {
    UIScrollView<LynxNestedScrollProtocol> *parent =
        [LynxBaseScrollView findAParentHasScroll:scrollView with:hittestChain];
    if (parent) {
      delta = [LynxBaseScrollView getDeltaScrollOffset:parent];
      vertical = parent.vertical;
      forwards = vertical ? delta.y > 0 : delta.x > 0;
      backwards = vertical ? delta.y < 0 : delta.x < 0;
      switch (forwards ? scrollView.forwardsNestedScrollMode
                       : scrollView.backwardsNestedScrollMode) {
        case NestedScrollModeSelfOnly:
          [LynxBaseScrollView rollBackScrollOffset:parent];
          break;
        case NestedScrollModeSelfFirst:
        case NestedScrollModeParentFirst:
        case NestedScrollModeParallel:
          break;
        default:
          break;
      }
      switch (forwards ? parent.forwardsNestedScrollMode : parent.backwardsNestedScrollMode) {
        case NestedScrollModeSelfOnly:
          [LynxBaseScrollView rollBackParentsScrollOffset:parent toTop:hittestChain];
          return NO;
        case NestedScrollModeSelfFirst:
        case NestedScrollModeParentFirst:
        case NestedScrollModeParallel:
          return [LynxBaseScrollView handleNestedScroll:parent with:hittestChain];
        default:
          return NO;
      }
    }
  }

  if (!forwards && !backwards) {
    // The scrollView does not scroll, and can not find any parent scrollView who scrolls,.
    return NO;
  }

  // The scrollView is scrolled.

  switch (forwards ? scrollView.forwardsNestedScrollMode : scrollView.backwardsNestedScrollMode) {
    case NestedScrollModeSelfOnly:
      [LynxBaseScrollView rollBackParentsScrollOffset:scrollView toTop:hittestChain];
      break;
    case NestedScrollModeSelfFirst:
      [LynxBaseScrollView rollBackParentsScrollOffset:scrollView toTop:hittestChain];
      break;
    case NestedScrollModeParentFirst: {
      BOOL parentScrollViewDoHaveScroll = [LynxBaseScrollView
          handleNestedScroll:[LynxBaseScrollView getParentScrollViewOf:scrollView with:hittestChain]
                        with:hittestChain];
      if (parentScrollViewDoHaveScroll) {
        [LynxBaseScrollView rollBackScrollOffset:scrollView];
      }
    } break;

    case NestedScrollModeParallel:
      break;
    default:
      break;
  }

  return YES;
}

- (void)scrollOffsetUpdated:(UIScrollView *)scrollView {
  [self.scrollDelegate scrollViewDidScroll:scrollView];
}

@end
