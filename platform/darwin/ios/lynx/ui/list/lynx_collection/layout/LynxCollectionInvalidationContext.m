// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionInvalidationContext.h"
#import <Foundation/Foundation.h>
#import "LynxCollectionViewLayout.h"
#import "LynxCollectionViewLayoutModel.h"

NSInteger const LynxCollectionInvalidNumberOfItems = -1;
NSInteger const LynxCollectionInvalidNumberOfColumns = -1;

@implementation LynxCollectionInvalidationContext

- (instancetype _Nonnull)init {
  self = [super init];
  if (self) {
    _bounds = CGRectNull;
    _layoutType = LynxCollectionViewLayoutNone;
    _numberOfItems = -1;
    _numberOfColumns = -1;
    _didSetInitialScrollIndex = NO;
    _mainAxisGap = -1;
    _crossAxisGap = -1;
    _invalidationType = ListNoneUpdate;
  }
  return self;
}

- (instancetype)initWithNumberOfItemsChanging:(NSInteger)number
                                fullSpanItems:(NSArray<NSIndexPath *> *)fullSpanItems
                               stickyTopItems:(NSArray<NSIndexPath *> *)stickyTopItems
                            stickyBottomItems:(NSArray<NSIndexPath *> *)stickyBottomItems
                             estimatedHeights:
                                 (NSDictionary<NSIndexPath *, NSNumber *> *)estimatedHeights {
  self = [self init];
  if (self) {
    _numberOfItems = number;
    _fullSpanItems = fullSpanItems;
    _stickyTopItems = stickyTopItems;
    _stickyBottomItems = stickyBottomItems;
    _estimatedHeights = estimatedHeights;
    _invalidationType = ListInitialDataUpdate;
  }
  return self;
}

- (instancetype)initWithLayoutTypeSwitching:(LynxCollectionViewLayoutType)type {
  self = [self init];
  if (self) {
    _layoutType = type;
    _invalidationType = ListLayoutUpdate;
  }
  return self;
}

- (instancetype)initWithNumberOfColumnsChanging:(NSUInteger)numberOfColumns {
  self = [self init];
  if (self) {
    _numberOfColumns = numberOfColumns;
    _invalidationType = ListColumnCountUpdate;
  }
  return self;
}

- (instancetype)initWithMainAxisGapChanging:(CGFloat)mainAxisGap {
  self = [self init];
  if (self) {
    _mainAxisGap = mainAxisGap;
    _invalidationType = ListMainAxisGapUpdate;
  }
  return self;
}

- (instancetype)initWithCrossAxisGapChanging:(CGFloat)crossAxisGap {
  self = [self init];
  if (self) {
    _crossAxisGap = crossAxisGap;
    _invalidationType = ListCrossAxisGapUpdate;
  }
  return self;
}

- (instancetype)initWithBoundsChanging:(CGRect)bounds {
  self = [self init];
  if (self) {
    _bounds = bounds;
    _invalidationType = ListBoundsUpdate;
  }
  return self;
}

- (instancetype)initWithInsetChanging:(UIEdgeInsets)insets {
  self = [self init];
  if (self) {
    _insets = insets;
    _hasInsetUpdates = YES;
    _invalidationType = ListInsetsUpdate;
  }
  return self;
}

- (instancetype)initWithRemovals:(NSArray<NSIndexPath *> *_Nonnull)removals
                      insertions:(NSArray<NSIndexPath *> *_Nonnull)insertions
                        moveFrom:(NSArray<NSIndexPath *> *_Nonnull)moveFrom
                          moveTo:(NSArray<NSIndexPath *> *_Nonnull)moveTo
                   fullSpanItems:(NSArray<NSIndexPath *> *)fullSpanItems
                  stickyTopItems:(NSArray<NSIndexPath *> *)stickyTopItems
               stickyBottomItems:(NSArray<NSIndexPath *> *)stickyBottomItems
                estimatedHeights:(NSDictionary<NSIndexPath *, NSNumber *> *)estimatedHeights {
  self = [self init];
  if (self) {
    _insertions = insertions;
    _removals = removals;
    _moveFrom = moveFrom;
    _moveTo = moveTo;
    _fullSpanItems = fullSpanItems;
    _stickyTopItems = stickyTopItems;
    _stickyBottomItems = stickyBottomItems;
    _estimatedHeights = estimatedHeights;
    _invalidationType = ListRegularDataUpdate;
  }
  return self;
}

- (instancetype)initWithFullSpanItems:(NSArray<NSIndexPath *> *)fullSpanItems
                       stickyTopItems:(NSArray<NSIndexPath *> *)stickyTopItems
                    stickyBottomItems:(NSArray<NSIndexPath *> *)stickyBottomItems
                     estimatedHeights:(NSDictionary<NSIndexPath *, NSNumber *> *)estimatedHeights {
  self = [self init];
  if (self) {
    _fullSpanItems = fullSpanItems;
    _stickyTopItems = stickyTopItems;
    _stickyBottomItems = stickyBottomItems;
    _estimatedHeights = estimatedHeights;
    _invalidationType = ListElementTypeUpdate;
  }
  return self;
}

- (instancetype)initWithUpdateAtIndexPath:(NSIndexPath *_Nonnull)indexPath bounds:(CGRect)bound {
  self = [self init];
  if (self) {
    _updates = @{indexPath : [LynxCollectionViewLayoutModel modelWithBounds:bound]};
    _invalidationType = ListCellUpdate;
  }
  return self;
}

- (instancetype)initWithResetAnimationTo:(BOOL)animated {
  self = [self init];
  if (self) {
    _animated = animated;
    _invalidationType = ListAnimationUpdate;
  }
  return self;
}

- (instancetype)initWithSelfSizingCellAtIndexPath:(NSIndexPath *)indexPath
                                           bounds:(CGRect)bounds
                                   collectionView:(nonnull UICollectionView *)collectionView
                                     isHorizontal:(BOOL)isHorizontal {
  self = [self init];
  if (self) {
    _selfSizing = YES;
    _currentContentOffset = collectionView.contentOffset;
    _invalidationType = ListCellSelfSizingUpdate;

    CGPoint contentOffsetForFindingIndexPath = CGPointZero;

    if (!isHorizontal) {
      CGFloat collectionViewWidthMid = collectionView.bounds.size.width / 2;
      if (_currentContentOffset.y < collectionView.contentInset.top) {
        contentOffsetForFindingIndexPath =
            CGPointMake(collectionViewWidthMid, collectionView.contentInset.top + 0.0001);
      } else {
        contentOffsetForFindingIndexPath =
            CGPointMake(collectionViewWidthMid, collectionView.bounds.origin.y);
      }
    } else {
      CGFloat collectionViewHeightMid = collectionView.bounds.size.height / 2;
      if (_currentContentOffset.x < collectionView.contentInset.left) {
        contentOffsetForFindingIndexPath =
            CGPointMake(collectionView.contentInset.left + 0.0001, collectionViewHeightMid);
      } else {
        contentOffsetForFindingIndexPath =
            CGPointMake(collectionView.bounds.origin.x, collectionViewHeightMid);
      }
    }

    _indexPathContainsContentOffset =
        [collectionView indexPathForItemAtPoint:contentOffsetForFindingIndexPath];

    // The `contentOffsetForFindingIndexPath` is always at the top, so if there is a sticky item,
    // indexPathForItemAtPoint: will return the sticky one, and it can not be the target index path
    // which we will fix its height changes.
    LynxCollectionViewLayout *layout =
        (LynxCollectionViewLayout *)collectionView.collectionViewLayout;
    BOOL isSticky = [layout isStickyItem:_indexPathContainsContentOffset];
    if (isSticky) {
      // If there is not visible cell that contains `contentOffsetForFindingIndexPath`, the
      // indexPath will be the displaying one that contains `contentOffsetForFindingIndexPath`, so,
      // init with it.

      // Then, we enumrate all the visible cells to test if one of them contains
      // `contentOffsetForFindingIndexPath`
      [collectionView.visibleCells
          enumerateObjectsUsingBlock:^(__kindof UICollectionViewCell *_Nonnull obj, NSUInteger idx,
                                       BOOL *_Nonnull stop) {
            CGPoint pointInCell = [collectionView convertPoint:contentOffsetForFindingIndexPath
                                                        toView:obj];
            if (CGRectContainsPoint(obj.bounds, pointInCell)) {
              NSIndexPath *path = [collectionView indexPathForCell:obj];
              if (![layout isStickyItem:path]) {
                _indexPathContainsContentOffset = path;
                *stop = YES;
              }
            }
          }];
    }

    _updates = @{indexPath : [LynxCollectionViewLayoutModel modelWithBounds:bounds]};
  }
  return self;
}

- (instancetype)initWithInitialScrollIndexSet {
  self = [self init];
  if (self) {
    _didSetInitialScrollIndex = YES;
    _invalidationType = ListInitialScrollIndexUpdate;
  }
  return self;
}

@end
