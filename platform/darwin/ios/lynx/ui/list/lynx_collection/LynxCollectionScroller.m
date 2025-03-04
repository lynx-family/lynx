// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionScroller.h"
#import "LynxCollectionViewLayout.h"

typedef void (^LynxCollectionScrollerCompletionBlock)(BOOL success);

//  During scrolling, cell's estimated height will be changed to a real height, which make
//  collectionView's contentSize changed constantly. So, firstly, we start scroll to a dummy
//  contentOffset. While scrolling, listen to scrollViewDidScroll, make sure currentConentOffset
//  will not be too exagegerated. After scroll finished, set contentOffset to a real position, at
//  this time, the contentSize is stable.

@interface LynxCollectionScroller ()
@property(nonatomic, assign) BOOL willScrollDown;
@property(nonatomic, assign) BOOL willScrollToInvisibleRect;
@property(nonatomic, assign) UICollectionViewScrollPosition scrollPosition;
@property(nonatomic, assign) CGFloat offset;
@property(nonatomic, assign) BOOL sticky;
@property(nonatomic, strong) NSIndexPath *targetIndexPath;

@property(nonatomic, strong, nonnull) LynxCollectionScrollerCompletionBlock completion;
@property(nonatomic, weak) id<LynxCollectionScrollerHolderDelegate> delegate;

@end

@implementation LynxCollectionScroller

- (instancetype)initWithTargetIndexPath:(NSIndexPath *)targetIndexPath
                             scrollDown:(BOOL)willScrollDown
                  scrollToInvisibleRect:(BOOL)willScrollToInvisibleRect
                         scrollPosition:(UICollectionViewScrollPosition)scrollPosition
                                 offset:(CGFloat)offset
                                 sticky:(BOOL)sticky
                               delegate:(id<LynxCollectionScrollerHolderDelegate>)delegate
                             completion:(nonnull void (^)(BOOL success))completion {
  if (self = [super init]) {
    _targetIndexPath = targetIndexPath;
    _willScrollDown = willScrollDown;
    _willScrollToInvisibleRect = willScrollToInvisibleRect;
    _scrollPosition = scrollPosition;
    _offset = offset;
    _sticky = sticky;
    _delegate = delegate;
    _completion = completion;
  }
  return self;
}

- (void)collectionViewStartScroll:(UICollectionView *)collectionView animated:(BOOL)animated {
  CGPoint idealTargetOffset = [self targetContentOffset:collectionView
                                              indexPath:self.targetIndexPath
                                                 offset:self.offset
                                                 sticky:self.sticky
                                         scrollPosition:self.scrollPosition];

  // if it is a smooth-scroll, and the target is equal to current offset, the anim will not begin,
  // we have to invoke the callback manually.
  if (animated && CGPointEqualToPoint(idealTargetOffset, collectionView.contentOffset)) {
    [self collectionViewAdjustTargetContentOffsetAtNextRunloop:collectionView];
  }

  // for simplicity, we setContentOffset to trigger scroll anim
  [collectionView setContentOffset:idealTargetOffset animated:animated];

  if (!animated) {
    // set contentOffset may make contentSize changed,
    // make sure contentOffset is correct at next runloop
    [self collectionViewAdjustTargetContentOffsetAtNextRunloop:collectionView];
  }
}

- (void)stopScroll {
  _completion(NO);
  [self.delegate removeScroller];
}

- (void)collectionViewDidScroll:(UICollectionView *)collectionView {
  if (self.willScrollToInvisibleRect) {
    CGPoint idealTargetOffset = [self targetContentOffset:collectionView
                                                indexPath:self.targetIndexPath
                                                   offset:self.offset
                                                   sticky:self.sticky
                                           scrollPosition:self.scrollPosition];
    if (!_horizontalLayout) {
      if (self.willScrollDown && collectionView.contentOffset.y > idealTargetOffset.y) {
        [collectionView
            setContentOffset:CGPointMake(collectionView.contentOffset.x, idealTargetOffset.y)];

      } else if (!self.willScrollDown && collectionView.contentOffset.y < idealTargetOffset.y) {
        [collectionView
            setContentOffset:CGPointMake(collectionView.contentOffset.x, idealTargetOffset.y)];
      }
    } else {
      if (self.willScrollDown && collectionView.contentOffset.x > idealTargetOffset.x) {
        [collectionView
            setContentOffset:CGPointMake(idealTargetOffset.x, collectionView.contentOffset.y)];

      } else if (!self.willScrollDown && collectionView.contentOffset.x < idealTargetOffset.x) {
        [collectionView
            setContentOffset:CGPointMake(idealTargetOffset.x, collectionView.contentOffset.y)];
      }
    }
  }
}

- (void)collectionViewDidEndScrollingAnimation:(UICollectionView *)collectionView {
  // adjust contentOffset at collectionViewDidScroll, which may make contentSize changed
  // make sure contentOffset is correct at next runloop

  [self collectionViewAdjustTargetContentOffsetAtNextRunloop:collectionView];
}

- (void)collectionViewAdjustTargetContentOffsetAtNextRunloop:(UICollectionView *)collectionView {
  __weak __typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      [strongSelf.delegate removeScroller];
      CGPoint idealTargetOffset = [strongSelf targetContentOffset:collectionView
                                                        indexPath:strongSelf.targetIndexPath
                                                           offset:strongSelf.offset
                                                           sticky:strongSelf.sticky
                                                   scrollPosition:strongSelf.scrollPosition];

      if (!strongSelf.horizontalLayout && collectionView.contentOffset.y != idealTargetOffset.y) {
        [collectionView
            setContentOffset:CGPointMake(collectionView.contentOffset.x, idealTargetOffset.y)
                    animated:NO];
      } else if (strongSelf.horizontalLayout &&
                 collectionView.contentOffset.x != idealTargetOffset.x) {
        [collectionView
            setContentOffset:CGPointMake(idealTargetOffset.x, collectionView.contentOffset.y)
                    animated:NO];
      }
      if (strongSelf.completion) {
        strongSelf.completion(YES);
      }
    }
  });
}

- (void)collectionViewWillBeginDragging:(UICollectionView *)collectionView {
  [self stopScroll];
}

// compute target contentCoffset at this moment

- (CGPoint)targetContentOffset:(UICollectionView *)collectionView
                     indexPath:(NSIndexPath *)targetIndexPath
                        offset:(CGFloat)offset
                        sticky:(BOOL)sticky
                scrollPosition:(UICollectionViewScrollPosition)scrollPosition {
  UICollectionViewLayoutAttributes *targetAttribute;
  if (sticky) {
    LynxCollectionViewLayout *layout =
        (LynxCollectionViewLayout *)collectionView.collectionViewLayout;
    targetAttribute = [layout layoutAttributesForStickItemAtIndexPath:targetIndexPath];
  } else {
    targetAttribute = [collectionView layoutAttributesForItemAtIndexPath:targetIndexPath];
  }

  if (_horizontalLayout) {
    CGFloat cellOffset = targetAttribute.frame.origin.x;
    CGFloat cellWidth = targetAttribute.frame.size.width;

    CGFloat collectionViewWidth = collectionView.frame.size.width;
    CGFloat collectionViewContentWidth = collectionView.contentSize.width;

    if (targetAttribute) {
      CGFloat targetOffset = -offset;
      // Be consistent with Android by accepting left/top in horizontal mode.
      if (scrollPosition == UICollectionViewScrollPositionLeft ||
          scrollPosition == UICollectionViewScrollPositionTop) {
        targetOffset += cellOffset;
      } else if (scrollPosition == UICollectionViewScrollPositionCenteredVertically) {
        // To avoid break change, still use the name of
        // `UICollectionViewScrollPositionCenteredVertically` in horizontal mode
        targetOffset += cellOffset - (collectionViewWidth - cellWidth) / 2.0;
      } else {
        // UICollectionViewScrollPositionBottom
        targetOffset += cellOffset - collectionViewWidth + cellWidth;
      }

      targetOffset = MAX(-collectionView.contentInset.left,
                         MIN(targetOffset, collectionViewContentWidth - collectionViewWidth +
                                               collectionView.contentInset.right));
      return CGPointMake(targetOffset, collectionView.contentOffset.y);
    }
  } else {
    CGFloat cellOffset = targetAttribute.frame.origin.y;
    CGFloat cellHeight = targetAttribute.frame.size.height;

    CGFloat collectionViewHeight = collectionView.frame.size.height;
    CGFloat collectionViewContentHeight = collectionView.contentSize.height;

    if (targetAttribute) {
      CGFloat targetOffset = -offset;
      if (scrollPosition == UICollectionViewScrollPositionTop) {
        targetOffset += cellOffset;
      } else if (scrollPosition == UICollectionViewScrollPositionCenteredVertically) {
        targetOffset += cellOffset - (collectionViewHeight - cellHeight) / 2.0;
      } else {
        // UICollectionViewScrollPositionBottom
        targetOffset += cellOffset - collectionViewHeight + cellHeight;
      }

      targetOffset = MAX(-collectionView.contentInset.top,
                         MIN(targetOffset, collectionViewContentHeight - collectionViewHeight +
                                               collectionView.contentInset.bottom));
      return CGPointMake(collectionView.contentOffset.x, targetOffset);
    }
  }
  return collectionView.contentOffset;
}

@end
