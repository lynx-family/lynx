// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Internal.h>
#import <Lynx/LynxBaseScrollView+Nested.h>
#import <Lynx/UIScrollView+Lynx.h>

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
