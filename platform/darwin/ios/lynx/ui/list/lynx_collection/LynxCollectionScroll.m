// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionScroll.h"
#import "LynxCollectionInvalidationContext.h"
#import "LynxCollectionScroller.h"
#import "LynxCollectionViewLayout.h"
#import "LynxUIMethodProcessor.h"

CGFloat const kLynxCollectionScrollDefaultSafeDisappearPercentage = 0.01;
CGFloat const kLynxCollectionScrollDefaultSafeDisappearOffset =
    kLynxCollectionScrollDefaultSafeDisappearPercentage * 896.;
CGFloat const kLynxCollectionScrollDefaultAnimationTimeForOnePage = 0.005;
CGFloat const kLynxCollectionScrollDefaultAnimationSpeed =
    896. / kLynxCollectionScrollDefaultAnimationTimeForOnePage;

NSInteger const LynxCollectionScrollInitialSrollInvalidIndex = -1;

typedef NS_ENUM(NSInteger, LynxUICollectionInitialScrollIndexState) {
  LynxUICollectionInitialScrollIndexStateInvalid,
  LynxUICollectionInitialScrollIndexStateUnset,
  LynxUICollectionInitialScrollIndexStateSet,
  LynxUICollectionInitialScrollIndexStateDidScroll
};

@interface LynxCollectionScroll () <LynxCollectionScrollerHolderDelegate>
@property(nonatomic, nullable) NSIndexPath* lastIndexPathWithValidLayoutAttributes;
@property(nonatomic) BOOL hasEstimatedHeights;
@property(nonatomic) CGFloat animationSpeed;
@property(nonatomic) CGFloat safeDisappearOffset;
@property(nonatomic) LynxUICollectionInitialScrollIndexState initialScrollIndexState;
@property(nonatomic, strong) LynxCollectionScroller* scroller;
@end

@implementation LynxCollectionScroll

- (instancetype)init {
  self = [super init];
  if (self) {
    _animationSpeed = kLynxCollectionScrollDefaultAnimationSpeed;
    _safeDisappearOffset = kLynxCollectionScrollDefaultSafeDisappearOffset;
    _initialScrollIndexState = LynxUICollectionInitialScrollIndexStateUnset;
    _initialScrollIndex = LynxCollectionScrollInitialSrollInvalidIndex;
    _hasEstimatedHeights = NO;
  }
  return self;
}

+ (UICollectionViewScrollPosition)scrollPositionWithNSString:(NSString*)string {
  UICollectionViewScrollPosition position = UICollectionViewScrollPositionNone;
  if (string) {
    if ([string isEqualToString:@"top"]) {
      position = UICollectionViewScrollPositionTop;
    } else if ([string isEqualToString:@"bottom"]) {
      position = UICollectionViewScrollPositionBottom;
    } else if ([string isEqualToString:@"left"]) {
      position = UICollectionViewScrollPositionLeft;
    } else if ([string isEqualToString:@"right"]) {
      position = UICollectionViewScrollPositionRight;
    } else if ([string isEqualToString:@"middle"]) {
      position = UICollectionViewScrollPositionCenteredVertically;
    }
  }
  return position;
}

BOOL LynxCollectionScrollApproximatelyEqualCGFloat(CGFloat a, CGFloat b) {
#if !defined(LYNX_CGFLOAT_EPSILON)
#if CGFLOAT_IS_DOUBLE
#define LYNX_CGFLOAT_EPSILON DBL_EPSILON
#else
#define LYNX_CGFLOAT_EPSILON FLT_EPSILON
#endif
#endif  // !defined(CGFLOAT_EPSILON)
  CGFloat epsilon = 0.;
  if (fabs(a) < fabs(b)) {
    epsilon = fabs(b) * LYNX_CGFLOAT_EPSILON;
  } else {
    epsilon = fabs(a) * LYNX_CGFLOAT_EPSILON;
  }
  return fabs(a - b) <= epsilon;
}

BOOL LynxCollectionScrollShouldApplyOffset(CGFloat offset,
                                           UICollectionViewScrollPosition scrollPosition) {
  return !(LynxCollectionScrollApproximatelyEqualCGFloat(offset, 0.)) &&
         scrollPosition != UICollectionViewScrollPositionNone;
}

- (BOOL)scrollWithValidLayoutAttributesOnCollectionView:(UICollectionView*)collectionView
                                              indexPath:(NSIndexPath*)targetIndexPath
                                                 offset:(CGFloat)offset
                                                alignTo:
                                                    (UICollectionViewScrollPosition)scrollPosition
                                                 smooth:(BOOL)smooth {
  // Assumption: if indexPath [m] has valid layoutAttributes (cellForItemAtIndexPath: has been
  // called) then for all indexPaths [n], such that [n] < [m], [n] has valid layoutAttributes
  if ((targetIndexPath.row <= self.lastIndexPathWithValidLayoutAttributes.row)) {
    if (LynxCollectionScrollShouldApplyOffset(offset, scrollPosition)) {
      UICollectionViewLayoutAttributes* targetAttribute =
          [collectionView layoutAttributesForItemAtIndexPath:targetIndexPath];

      CGFloat cellOffset = targetAttribute.frame.origin.y;
      CGFloat cellHeight = targetAttribute.frame.size.height;

      CGFloat collectionViewHeight = collectionView.frame.size.height;
      CGFloat collectionViewContentHeight = collectionView.contentSize.height;

      if (targetAttribute) {
        CGFloat targetOffset = -offset;
        if (scrollPosition == UICollectionViewScrollPositionTop) {
          targetOffset += cellOffset;
        } else {
          // UICollectionViewScrollPositionBottom
          targetOffset += cellOffset - collectionViewHeight + cellHeight;
        }

        targetOffset = MAX(0, targetOffset);
        targetOffset = MIN(collectionViewContentHeight, targetOffset);

        [collectionView setContentOffset:CGPointMake(0, targetOffset) animated:smooth];
        return YES;
      }
    } else {
      [collectionView scrollToItemAtIndexPath:targetIndexPath
                             atScrollPosition:scrollPosition
                                     animated:smooth];
      return YES;
    }
  }
  return NO;
}

- (void)scrollHeuristicallyOnCollectionView:(UICollectionView*)collectionView
                                  indexPath:(NSIndexPath*)targetIndexPath
                                     offset:(CGFloat)offset
                                    alignTo:(UICollectionViewScrollPosition)scrollPosition
                                     smooth:(BOOL)smooth
                                   callback:(LynxUIMethodCallbackBlock)callback {
  typedef void (^ScrollToPositionBlockType)(void);
  ScrollToPositionBlockType scroller;
  __weak UICollectionView* weakCollectionView = collectionView;
  __block __weak ScrollToPositionBlockType weakScroller;
  __block NSTimeInterval animationSpeed = _animationSpeed;
  __weak LynxCollectionScroll* weakSelf = self;
  __block CGFloat safeDisappearOffset = _safeDisappearOffset;
  weakScroller = scroller = [^(void) {
    UICollectionView* strongCollectionView = weakCollectionView;
    if (strongCollectionView == nil) {
      callback(kUIMethodUnknown, @"CollectionView is nil");
      return;
    }

    // find all current visibleItems and check whether our targetIndexPath is among them.
    NSArray<NSIndexPath*>* indexPathsForVisibleItems =
        [strongCollectionView indexPathsForVisibleItems];
    if (![indexPathsForVisibleItems containsObject:targetIndexPath]) {
      // our targetIndexPath is not visible now. scroll to a position where the collectionView
      // could be trigged to load a new cell. This position could not be too far (far enough to
      // scroll a current visible cell out of visible area). The cell being scrolled out of the
      // visible will have been recycled when the animation starts. This makes the scrolling
      // animation werid.
      // clang-format off
      //                        contentOffsetY
      //                        +-+------+---+---------------------+
      //                          ^      ^   |                     |
      //                          |      |   |                     | --> content
      //                          |      |   |                     |
      //                          |      |   |                     |
      //                          |      |   |                     |
      //                          |      |   |                     |
      //                          |      |   | +-------+ +-------+ |
      //                          v      |   | |       | |       | |maxTargetContentOffsetDeltaY
      //                        +-+---------*************************----------+--    ^
      //                                 |  *| |   ^   | |   ^   | |*          ^      |
      //                                 |  *| |saf|DisapperD|lta| |*          v  +---+
      //                                 |  *| +---v-----+----------*----------+
      //                                 |  *| +-------+ |   |   | |*
      //                                 |  *|           |   v   | |*
      //                                 |  *| +-------+ +---+---+ |*
      //                                 |  *| |       |           |*
      //                                 |  *| |       | +-------+ |*
      //                                 |  *| |       | |       | |*
      //                                 |  *| |       | |       | |*
      //                visibleRectBottom|  *| |       | |       | |*  visibleRect
      //                                 |  *| |       | |       | |*
      //                                 |  *| |       | |       | |*
      //                                 v  *| |       | |       | |*
      //                      +-+--------+--*************************
      //                        ^            | |   ^   | |   ^   | |
      //                        |            | |   |   | |   |   | |
      //                        |            | |   |   | |   |   | |
      //                 +---+  |            | | delta | |   |   | |
      //                 |      |            | |   |   | |   |   | |
      //                 v      v            | |   v   | | delta | |
      //                      +-+--------------+---+---+ |   |   | |
      //            targetContentOffsetDeltaY|           |   |   | |
      //                                     |           |   v   | |
      //                                     |           +---+---+ |
      //                                     |                     |
      //                                     +---------------------+
      // clang-format on
      CGPoint currentOffset = [strongCollectionView contentOffset];
      // compute the step Adaptively.
      __block NSNumber* targetContentOffsetDeltaY = nil;
      __block NSNumber* maxTargetContentOffsetDeltaY = nil;
      [indexPathsForVisibleItems enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull indexPath,
                                                              NSUInteger idx, BOOL* _Nonnull stop) {
        UICollectionViewLayoutAttributes* attribute =
            [strongCollectionView layoutAttributesForItemAtIndexPath:indexPath];
        if (!attribute) {
          // layoutAttributes for a visibleItem cannot be nil.
          return;
        }
        CGPoint contentOffset = [strongCollectionView contentOffset];
        CGFloat cellSafeDisappearOffset = contentOffset.y + safeDisappearOffset;
        CGFloat cellBottom = attribute.frame.origin.y + attribute.frame.size.height;
        CGFloat visibleRectBottom = contentOffset.y + strongCollectionView.frame.size.height;
        CGFloat delta = cellBottom - visibleRectBottom;
        CGFloat safeDisappearDelta = cellBottom - contentOffset.y;

        // find the cell has the smallest cellBottom, set maxTargetContentOffsetDeltaY to its bottom
        // - 1 this ensures that this cell will not be scroll out of the visible area if the bottom
        // of the cell is in the 'cellSafeDisappearOffset' area, ignore it because its disappearance
        // (being recycled) is not obvious and hard to be aware of.
        if (cellBottom > cellSafeDisappearOffset) {
          if (maxTargetContentOffsetDeltaY == nil ||
              maxTargetContentOffsetDeltaY.floatValue > safeDisappearDelta) {
            maxTargetContentOffsetDeltaY = [NSNumber numberWithFloat:(safeDisappearDelta - 1)];
          }
        }

        // find the cell has the smallest bottom among the cells with bottom larger
        // than the bottom of the visible area
        if (delta > 0.) {
          if (targetContentOffsetDeltaY == nil || targetContentOffsetDeltaY.floatValue > delta) {
            targetContentOffsetDeltaY = [NSNumber numberWithFloat:delta];
          }
        }
      }];

      CGFloat contentOffsetDelta = 0.;
      if (targetContentOffsetDeltaY != nil) {
        contentOffsetDelta = contentOffsetDelta + targetContentOffsetDeltaY.floatValue;
      }

      // clamp scroll offset by maxTargetContentOffsetDeltaY, for reason, see
      // aforementioned comment of maxTargetContentOffsetDeltaY
      if (maxTargetContentOffsetDeltaY != nil) {
        contentOffsetDelta = MIN(maxTargetContentOffsetDeltaY.floatValue, contentOffsetDelta);
      }

      // at least we have to move 1 more than targetContentOffsetDeltaY to trigger the
      // collectionView loading new cells
      contentOffsetDelta = MAX(1., contentOffsetDelta);

      CGFloat targetContentOffsetY = currentOffset.y + contentOffsetDelta;

      // you cannot scroll out of the scroll'able area
      CGFloat maxTargetContentOffsetY =
          [strongCollectionView contentSize].height - strongCollectionView.frame.size.height;
      maxTargetContentOffsetY = MAX(0, maxTargetContentOffsetY);
      if (targetContentOffsetY > maxTargetContentOffsetY) {
        callback(kUIMethodParamInvalid, @"can not scroll when come to border");
        return;
      }

      CGPoint targetContentOffset = CGPointMake(currentOffset.x, targetContentOffsetY);
      ScrollToPositionBlockType strongScroller = [weakScroller copy];

      // you cannot animate the contentOffset, but set the contentOffset of a scrollView
      // is the same as set the bounds of the scrollView, thus we animate the bounds instead.
      // see
      // https://stackoverflow.com/questions/6535450/can-i-animate-the-uiscrollview-contentoffset-property-via-its-layer
      // for explanation
      CGRect bounds = strongCollectionView.bounds;
      [CATransaction begin];
      CABasicAnimation* animation = [CABasicAnimation animationWithKeyPath:@"bounds"];
      animation.duration = (targetContentOffset.y - currentOffset.y) / animationSpeed;
      animationSpeed += 0.2 * animationSpeed;
      animation.timingFunction =
          [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
      bounds.origin.y += targetContentOffset.y - currentOffset.y;
      animation.toValue = [NSValue valueWithCGRect:bounds];
      [CATransaction setCompletionBlock:^{
        strongCollectionView.bounds = bounds;
        // on the scrolling being done, do this procedure recursively.
        strongScroller();
      }];
      if (smooth) {
        [strongCollectionView.layer addAnimation:animation forKey:@"bounds"];
      }
      [CATransaction commit];
    } else {
      // our targetIndexPath is now VISIBLE, its layoutAttribute must be VALID now.
      LynxCollectionScroll* strongSelf = weakSelf;
      bool result = NO;
      if (strongSelf) {
        result = [strongSelf scrollWithValidLayoutAttributesOnCollectionView:strongCollectionView
                                                                   indexPath:targetIndexPath
                                                                      offset:offset
                                                                     alignTo:scrollPosition
                                                                      smooth:smooth];
      }
      if (result) {
        callback(kUIMethodSuccess, nil);
      } else {
        callback(kUIMethodParamInvalid, @"can not scroll when come to border");
      }
    }
  } copy];
  scroller();
}

#pragma mark - LynxCollectionScrollerHolderDelegate
- (void)removeScroller {
  self.scroller = nil;
}

#pragma mark - LynxCollectionScrollListener
- (void)collectionViewDidScroll:(UICollectionView*)collectionView {
  [self.scroller collectionViewDidScroll:collectionView];
}
- (void)collectionViewDidEndScrollingAnimation:(UICollectionView*)collectionView {
  [self.scroller collectionViewDidEndScrollingAnimation:collectionView];
}
- (void)collectionViewWillBeginDragging:(UICollectionView*)collectionView {
  [self.scroller collectionViewWillBeginDragging:collectionView];
}

- (void)scrollCollectionView:(UICollectionView*)collectionView
                    position:(NSInteger)position
                      offset:(CGFloat)offset
                     alignTo:(NSString*)alignTo
                      smooth:(BOOL)smooth
                 useScroller:(BOOL)useScroller
                    callback:(LynxUIMethodCallbackBlock)callback {
  NSIndexPath* targetIndexPath = [NSIndexPath indexPathForRow:position inSection:0];
  UICollectionViewScrollPosition scrollPosition =
      [LynxCollectionScroll scrollPositionWithNSString:alignTo];
  if (useScroller) {
    [self scrollByScroller:collectionView
                 indexPath:targetIndexPath
                    offset:offset
            scrollPosition:scrollPosition
                    smooth:smooth
                  callback:callback];
    return;
  }

  if ([self scrollWithValidLayoutAttributesOnCollectionView:collectionView
                                                  indexPath:targetIndexPath
                                                     offset:offset
                                                    alignTo:scrollPosition
                                                     smooth:smooth]) {
    callback(kUIMethodSuccess, nil);
    return;
  }

  [self scrollHeuristicallyOnCollectionView:collectionView
                                  indexPath:targetIndexPath
                                     offset:offset
                                    alignTo:scrollPosition
                                     smooth:smooth
                                   callback:callback];
}

- (void)scrollByScroller:(UICollectionView*)collectionView
               indexPath:(NSIndexPath*)targetIndexPath
                  offset:(CGFloat)offset
          scrollPosition:(UICollectionViewScrollPosition)scrollPosition
                  smooth:(BOOL)smooth
                callback:(LynxUIMethodCallbackBlock)callback {
  NSArray<NSIndexPath*>* indexPathsForVisibleItems = [collectionView indexPathsForVisibleItems];

  BOOL willScrollDown = YES;
  BOOL willScrollToInvisibleRect = NO;

  if ([indexPathsForVisibleItems containsObject:targetIndexPath]) {
    willScrollToInvisibleRect = NO;

  } else {
    willScrollToInvisibleRect = YES;
    willScrollDown = indexPathsForVisibleItems.firstObject.item < targetIndexPath.item;
  }

  // test sticky
  LynxCollectionViewLayout* layout = (LynxCollectionViewLayout*)collectionView.collectionViewLayout;
  BOOL isSticky = [layout isStickyItem:targetIndexPath];
  if (isSticky) {
    UICollectionViewLayoutAttributes* attributes =
        [layout layoutAttributesForStickItemAtIndexPath:targetIndexPath];
    willScrollDown = attributes.frame.origin.y > collectionView.contentOffset.y;
    willScrollToInvisibleRect =
        CGRectGetMaxY(attributes.frame) < collectionView.contentOffset.y ||
        CGRectGetMinY(attributes.frame) >
            collectionView.contentOffset.y + collectionView.bounds.size.height;
  }

  [self.scroller stopScroll];

  self.scroller = [[LynxCollectionScroller alloc]
      initWithTargetIndexPath:targetIndexPath
                   scrollDown:willScrollDown
        scrollToInvisibleRect:willScrollToInvisibleRect
               scrollPosition:scrollPosition
                       offset:offset
                       sticky:isSticky
                     delegate:self
                   completion:^(BOOL success) {
                     if (success) {
                       callback(kUIMethodSuccess, nil);
                     } else {
                       callback(kUIMethodParamInvalid, @"scroll stoped");
                     }
                   }];
  self.scroller.horizontalLayout = self.horizontalLayout;
  [self.scroller collectionViewStartScroll:collectionView animated:smooth];
}

- (NSIndexPath*)lastIndexPathWithValidLayoutAttributes {
  if (_hasEstimatedHeights) {
    return [NSIndexPath indexPathForRow:NSIntegerMax inSection:0];
  } else {
    return _lastIndexPathWithValidLayoutAttributes;
  }
}

- (void)updateLastIndexPathWithValidLayoutAttributes:(LynxCollectionInvalidationContext*)context {
  NSInteger targetIndex = self.lastIndexPathWithValidLayoutAttributes.item;

  for (NSIndexPath* indexPath in context.removals) {
    targetIndex = MIN(targetIndex, indexPath.item - 1);
  }

  for (NSIndexPath* indexPath in context.insertions) {
    targetIndex = MIN(targetIndex, indexPath.item - 1);
  }

  if (!context.isSelfSizing) {
    for (NSIndexPath* indexPath in context.updates) {
      targetIndex = MIN(targetIndex, indexPath.item - 1);
    }
  }

  for (NSIndexPath* indexPath in context.moveFrom) {
    targetIndex = MIN(targetIndex, indexPath.item - 1);
  }

  for (NSIndexPath* indexPath in context.moveTo) {
    targetIndex = MIN(targetIndex, indexPath.item - 1);
  }

  if (targetIndex > 0) {
    self.lastIndexPathWithValidLayoutAttributes =
        [NSIndexPath indexPathForItem:targetIndex
                            inSection:self.lastIndexPathWithValidLayoutAttributes.section];
  } else {
    self.lastIndexPathWithValidLayoutAttributes = nil;
  }
}

- (void)updateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.estimatedHeights) {
    _hasEstimatedHeights = YES;
    return;
  }

  if (self.lastIndexPathWithValidLayoutAttributes == nil) {
    return;
  }
  // updates from diff or onComponentLayoutChanged
  // updates before lastIndexPathWithValidLayoutAttributes will trigger cellForItemAtIndexPath only
  // when it is visible. updates before lastIndexPathWithValidLayoutAttributes && not visible will
  // make list jumpy when scrolling back

  // update the animationSpeed and safeDisappearOffset when bounds changed
  if (!CGRectIsNull(context.bounds)) {
    CGRect bounds = context.bounds;
    _animationSpeed = bounds.size.height / kLynxCollectionScrollDefaultAnimationTimeForOnePage;
    _safeDisappearOffset = bounds.size.height * kLynxCollectionScrollDefaultSafeDisappearPercentage;
    // make sure `_safeDisappearOffset` is not too small
    _safeDisappearOffset = MAX(1., _safeDisappearOffset);
  }
}

- (void)updateWithCellLoadedAtIndexPath:(NSIndexPath*)indexPath {
  if (self.lastIndexPathWithValidLayoutAttributes == nil ||
      self.lastIndexPathWithValidLayoutAttributes.row < indexPath.row) {
    self.lastIndexPathWithValidLayoutAttributes = indexPath;
  }
}

#pragma mark - Initial Scroll

- (void)initialScrollCollectionView:(LynxCollectionViewLayout*)layout {
  NSInteger initialScrollIndex = self.initialScrollIndex;
  NSIndexPath* initialScrollIndexPath = [NSIndexPath indexPathForRow:initialScrollIndex
                                                           inSection:0];
  [layout prepareLayout];
  UICollectionViewLayoutAttributes* attributes =
      [layout layoutAttributesForItemAtIndexPath:initialScrollIndexPath];
  if (attributes == nil) {
    return;
  }
  if (_horizontalLayout) {
    [layout.collectionView setContentOffset:CGPointMake(attributes.frame.origin.x, 0)];
  } else {
    [layout.collectionView setContentOffset:CGPointMake(0, attributes.frame.origin.y)];
  }

  _initialScrollIndexState = LynxUICollectionInitialScrollIndexStateDidScroll;
}

- (void)initialScrollCollectionViewIfNeeded:(LynxCollectionViewLayout*)layout {
  // only try to scroll the list while it has not been initial-scrolled
  // We have to circumvent a bug in layout (on first layoutDidFinished, the list may have 0 height):
  // 1. the list should have a valid height, which must be greater than 0.
  // 2. the scrolling must be in the next layout cycle.
  // Otherwise, while changing the height of the list from 0 to a valid length, the contentOffset of
  // the list will be reset as 0.
  if (_initialScrollIndexState == LynxUICollectionInitialScrollIndexStateSet &&
      _initialScrollIndex > 0 && layout.collectionView.frame.size.height > 0) {
    __weak typeof(self) weakSelf = self;
    __weak typeof(layout) weakLayout = layout;
    dispatch_async(dispatch_get_main_queue(), ^{
      typeof(weakSelf) strongSelf = weakSelf;
      typeof(weakLayout) strongLayout = weakLayout;
      if (strongSelf && strongLayout) {
        [strongSelf initialScrollCollectionView:strongLayout];
      }
    });
  }
}

- (void)setInitialScrollIndex:(NSInteger)initialScrollIndex {
  if (_initialScrollIndexState == LynxUICollectionInitialScrollIndexStateUnset &&
      initialScrollIndex >= 0) {
    _initialScrollIndex = initialScrollIndex;
    _initialScrollIndexState = LynxUICollectionInitialScrollIndexStateSet;
  }
}

@end
