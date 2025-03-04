// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCollectionViewLayoutSectionModel.h"
#import "LynxCollectionInvalidationContext.h"
#import "LynxCollectionViewLayoutModel.h"
#import "LynxListDebug.h"
#import "LynxUIUnitUtils.h"
#import "UIScrollView+Lynx.h"

NSInteger const kLynxCollectionViewLayoutInvalidIndex = -1;
typedef void (^LynxCollectionViewLayoutMoveUpdate)(NSArray<NSIndexPath*>*);

@interface LynxCollectionViewLayoutSectionModel () {
  struct {
    /**
    if this section is not set to be animated, we should return nil for initialLayoutAttributes,
    finalLayoutAttributes and do nothing in prepareForCollectionViewUpdates.
    */
    unsigned isAnimated : 1;
    /**
     marked as YES when bounds changed.
     the origin part of bounds is consumed by sticky handling.
     the size part of bounds is consumed by base layout.
     */
    unsigned boundsChanged : 1;
    /**
    some queries for initialLayoutAttributes and finalLayoutAttributes stem from the adjustment
    of the bounds (contentOffset) of the CollectionView. We should ignore them and return nil
    instead. So we mark 'inPreparingForCollectionViewUpdates' for queries stem form
    prepareForCollectionViewUpdates
    */
    unsigned isPreparingForCollectionViewUpdates : 1;
    /**
     layout invalidation stem from the component adjusted the height by itself, rather than notified
     by the dataSource. handled seperately because using `performBatchUpdates:completion:` will
     cause unwanted `collectionView:cellForItemAtIndexPath:`.
     */
    unsigned isPreparingForCellLayoutUpdate : 1;
    unsigned hasPreparedForCellLayoutUpdate : 1;
    /**
     when self-sizing cell scrolled into visible position, it will be loaded and the whole
     collectionViewLayout will thus be invalidated. The original contentOffset should not be
     preserved. The new contentOffset will be calculated and applied.
     */
    unsigned shouldAdjustCollectionViewContentOffset : 1;
    /**
    sticky header & footer switch
     */
    unsigned enableSticky : 1;
    unsigned indexAsZIndex : 1;
  } _sectionModelFlags;
}

// layout essentials
@property(nonatomic, nonnull)
    NSMutableDictionary<NSNumber*, UICollectionViewLayoutAttributes*>* layoutAttributesCache;
@property(nonatomic, nonnull) NSMutableArray<LynxCollectionViewLayoutModel*>* models;

@property(nonatomic) NSMutableArray<NSNumber*>* mainSizes;
@property(nonatomic) NSMutableArray<NSArray<NSNumber*>*>* mainSizesCache;
@property(nonatomic) NSInteger firstInvalidIndex;
@property(nonatomic) NSUInteger numberOfColumns;
@property(nonatomic) CGFloat mainAxisGap;
@property(nonatomic) CGFloat crossAxisGap;
@property(nonatomic) CGRect bounds;
@property(nonatomic) UIEdgeInsets insets;
@property(nonatomic) LynxCollectionViewLayoutType layoutType;
@property(nonatomic) NSArray<NSIndexPath*>* fullSpanItems;

// sticky header & footers
@property(nonatomic) NSArray<NSIndexPath*>* stickyTopItems;
@property(nonatomic) NSArray<NSIndexPath*>* stickyBottomItems;
@property(nonatomic) NSInteger stickyTopIndex;
@property(nonatomic) NSInteger stickyBottomIndex;
@property(nonatomic) UICollectionViewLayoutAttributes* stickyTopItemAttribute;
@property(nonatomic) UICollectionViewLayoutAttributes* stickyBottomItemAttribute;

// animation related
@property(nonatomic, nullable) NSIndexSet* insertIndices;
@property(nonatomic, nonnull) NSArray<LynxCollectionViewLayoutModel*>* snapshotModels;
@property(nonatomic, nullable) NSDictionary<NSNumber*, NSNumber*>* beforeAfterIndexMap;

// self-sizing cell contentOffset adjustment
@property(nonatomic) CGPoint targetContentOffset;

// estimated-height API
@property(nonatomic) NSDictionary<NSIndexPath*, NSNumber*>* estimatedHeights;

@end

@implementation LynxCollectionViewLayoutSectionModel

- (instancetype)initWithItemCount:(NSUInteger)count {
  self = [super init];
  if (self) {
    _sectionModelFlags.isAnimated = 0;
    _sectionModelFlags.boundsChanged = 0;
    _sectionModelFlags.isPreparingForCellLayoutUpdate = 0;
    _sectionModelFlags.isPreparingForCollectionViewUpdates = 0;
    _sectionModelFlags.hasPreparedForCellLayoutUpdate = 0;
    _sectionModelFlags.shouldAdjustCollectionViewContentOffset = 0;
    _sectionModelFlags.enableSticky = 0;
    _sectionModelFlags.indexAsZIndex = 0;

    _numberOfColumns = 1;
    _mainAxisGap = 0;
    _crossAxisGap = 0;
    _firstInvalidIndex = 0;
    _bounds = CGRectZero;
    _layoutType = LynxCollectionViewLayoutNone;
    _mainSizesCache = [[NSMutableArray alloc] init];
    _layoutAttributesCache = [[NSMutableDictionary alloc] init];
    [self resetModelsWithLength:count];
    [self resetMainSizesWithNumberOfColumns:_numberOfColumns];
    _stickyTopIndex = kLynxCollectionViewLayoutInvalidIndex;
    _stickyBottomIndex = kLynxCollectionViewLayoutInvalidIndex;
    _stickyOffset = 0;
    _stickyTopItemAttribute = nil;
    _stickyBottomItemAttribute = nil;
  }
  return self;
}

#pragma mark - Public

- (void)setEnableSticky:(BOOL)enableSticky {
  _sectionModelFlags.enableSticky = enableSticky;
}

- (BOOL)enableSticky {
  return _sectionModelFlags.enableSticky;
}

- (void)setStickyOffset:(CGFloat)stickyOffset {
  _stickyOffset = stickyOffset;
}

- (void)setIndexAsZIndex:(BOOL)indexAsZIndex {
  _sectionModelFlags.indexAsZIndex = indexAsZIndex;
}

- (BOOL)indexAsZIndex {
  return _sectionModelFlags.indexAsZIndex;
}

- (CGSize)contentSize {
  CGSize contentSize = _bounds.size;
  if (_horizontalLayout) {
    contentSize.width = [self largestMainSize];
  } else {
    contentSize.height = [self largestMainSize];
  }
  return contentSize;
}

- (void)layoutIfNeededForUICollectionView:(UICollectionView*)view {
  // while using `animatedWithDuration:` with `invalidatelayoutwithcontext:`
  // there is no callback like `finalizeCollectionViewUpdates()`
  // however, `prepareLayout` is always called before any `initialLayoutAttributes`
  // or `finalLayoutAttributes`. It is safe to clear the flag when `prepareLayout`
  // is called next time.
  // (is, has) -> (is, has)
  if (_sectionModelFlags.isPreparingForCellLayoutUpdate) {
    // (1, _) -> (0, 1)
    _sectionModelFlags.isPreparingForCellLayoutUpdate = NO;
    _sectionModelFlags.hasPreparedForCellLayoutUpdate = YES;
  } else {
    // (0, _) -> (0, 0)
    _sectionModelFlags.hasPreparedForCellLayoutUpdate = NO;
  }

  if (_firstInvalidIndex != kLynxCollectionViewLayoutInvalidIndex) {
    LYNX_LIST_DEBUG_LOG(@"relayoutFrom: %@", @(_firstInvalidIndex));
    [self retrieveMainSizeFromCacheAtInvalidIndex:_firstInvalidIndex];
    [self layoutFromIndex:_firstInvalidIndex];
    _firstInvalidIndex = kLynxCollectionViewLayoutInvalidIndex;
    if (_sectionModelFlags.enableSticky) {
      if (self.useOldSticky) {
        [self retrieveStickyItemsForBounds:_bounds ForUICollectionView:view];
      } else {
        [self updateStickyItemsForUICollectionView:view];
      }
    }
  } else if (_sectionModelFlags.enableSticky &&
             (_sectionModelFlags.boundsChanged || !self.useOldSticky)) {
    if (self.useOldSticky) {
      [self retrieveStickyItemsForBounds:_bounds ForUICollectionView:view];
    } else {
      [self updateStickyItemsForUICollectionView:view];
    }
    _sectionModelFlags.boundsChanged = NO;
  }
}

- (__kindof UICollectionViewLayoutAttributes*)layoutAttributeForElementAtIndexPath:
    (NSIndexPath*)indexPath {
  if (indexPath.row == _stickyTopIndex && _stickyTopItemAttribute != nil) {
    return _stickyTopItemAttribute;
  } else if (indexPath.row == _stickyBottomIndex && _stickyBottomItemAttribute != nil) {
    return _stickyBottomItemAttribute;
  }
  return [self layoutAttributesFromCacheAtRow:indexPath.row];
}

- (NSArray<__kindof UICollectionViewLayoutAttributes*>*)layoutAttributesForElementsInRect:
    (CGRect)rect {
  // return attribute of item which is in specific Rect
  NSMutableArray<UICollectionViewLayoutAttributes*>* layoutAttributes = [NSMutableArray array];
  [_models enumerateObjectsUsingBlock:^(LynxCollectionViewLayoutModel* _Nonnull model,
                                        NSUInteger idx, BOOL* _Nonnull stop) {
    if (idx == (NSUInteger)_stickyTopIndex &&
        CGRectIntersectsRect(rect, _stickyTopItemAttribute.frame)) {
      [layoutAttributes addObject:_stickyTopItemAttribute];
    } else if (idx == (NSUInteger)_stickyBottomIndex &&
               CGRectIntersectsRect(rect, _stickyBottomItemAttribute.frame)) {
      [layoutAttributes addObject:_stickyBottomItemAttribute];
    } else if (CGRectIntersectsRect(rect, model.frame)) {
      UICollectionViewLayoutAttributes* attributes = [self layoutAttributesFromCacheAtRow:idx];
      if (attributes) {
        [layoutAttributes addObject:attributes];
      }
    }
  }];

  return layoutAttributes;
}

// helper function
- (void)handleLayoutFundamentalsUpdateWithInvalidationContext:
    (LynxCollectionInvalidationContext*)context {
  if (context.estimatedHeights) {
    self.estimatedHeights = context.estimatedHeights;
  }
  if (context.fullSpanItems) {
    self.fullSpanItems = context.fullSpanItems;
  }
  if (context.stickyTopItems) {
    self.stickyTopItems = context.stickyTopItems;
  }
  if (context.stickyBottomItems) {
    self.stickyBottomItems = context.stickyBottomItems;
  }
}

- (void)handleLayoutUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.layoutType != LynxCollectionViewLayoutNone && context.layoutType != _layoutType) {
    _layoutType = context.layoutType;
    _firstInvalidIndex = 0;
  }
}

- (void)handleColumnCountUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.numberOfColumns != LynxCollectionInvalidNumberOfItems &&
      (NSUInteger)context.numberOfColumns != _numberOfColumns) {
    _numberOfColumns = context.numberOfColumns;
    _firstInvalidIndex = 0;
  }
}

- (void)handleInitialUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  [self handleLayoutFundamentalsUpdateWithInvalidationContext:context];

  if (context.numberOfItems != LynxCollectionInvalidNumberOfItems &&
      (NSUInteger)context.numberOfItems != [_models count]) {
    [self resetModelsWithLength:context.numberOfItems];
    _firstInvalidIndex = 0;
  }
}

- (void)handleElementTypeUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  [self handleLayoutFundamentalsUpdateWithInvalidationContext:context];
  _firstInvalidIndex = 0;
}

- (void)handleRegularUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  [self handleLayoutFundamentalsUpdateWithInvalidationContext:context];

  LynxCollectionViewLayoutMoveUpdate moveUpdate = [self updateWithMoveFrom:context.moveFrom];
  [self updateWithUpdates:context.updates];
  [self updateWithRemovals:context.removals];
  [self updateWithInsertions:context.insertions];

  if (moveUpdate) {
    moveUpdate(context.moveTo);
  }
}

- (void)handleBoundsUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (!CGRectIsNull(context.bounds) && !CGSizeEqualToSize(_bounds.size, context.bounds.size)) {
    _bounds = context.bounds;
    _firstInvalidIndex = 0;
  }
}

- (void)handleAnimationUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.isAnimated) {
    _sectionModelFlags.isAnimated = YES;
  }
}

- (void)handleSelfSizingWithInvalidationContext:(LynxCollectionInvalidationContext*)context
                                 collectionView:(nonnull UICollectionView*)collectionView {
  if (context.isSelfSizing) {
    // compute the contentOffset after self-sizing.
    // clang-format off
    //           ▲  ┌─────────────┐           ┌─────────────┐  ▲
    //           │  │  wholeView  │           │  wholeView  │  │
    //           │  │ ┌──▲───▲──┐ │ self-     │             │  │
    //           │  │ │  │   │  │ │ ───────►  │             │  │
    //           │  │ │  │   │  │ │ sizing    │             │  │
    //           │  │ │ A│  B│  │ │           │             │  │
    // contentOff│et│ │  │   │  │ │           │ ┌─▲────▲──┐ │  │
    //           ▼ ┌┼─┼──┼───▼──┼─┼┐          │ │a│   b│  │ │  │
    //             ││ │  │      │ ││         ┌┼─┼─┴────▼──┼─┼┐ ▼
    //             ││ └──▼──────┘ ││         ││ └─▼───────┘ ││ targetContentOffset
    //             ││ visibleArea ││         ││ visibleArea ││
    //             └┼─────────────┼┘         └┼─────────────┼┘
    //              └─────────────┘           └─────────────┘
    //
    //             A: oldHeight B: yOffsetFromTopOfCellToContentOffset
    //             a: newHeight b: newYOffsetFromTopOfCellToContentOffset
    //
    //             A/a == B/b ==> b = B * (a/A)
    // clang-format on
    NSIndexPath* indexPath = context.indexPathContainsContentOffset;
    NSUInteger row = indexPath.row;
    if (_horizontalLayout) {
      CGFloat xOffsetFromTopOfCellToContentOffset =
          _models[row].frame.origin.x - context.currentContentOffset.x;
      CGFloat oldWidth = _models[row].frame.size.width;
      [self updateWithUpdates:context.updates];
      [self layoutIfNeededForUICollectionView:collectionView];
      CGFloat newWidth = _models[row].frame.size.width;
      CGFloat newXOffsetFromTopOfCellToContentOffset = 0;
      if (self.fixSelfSizingOffsetFromStart) {
        newXOffsetFromTopOfCellToContentOffset = xOffsetFromTopOfCellToContentOffset;
      } else {
        if (oldWidth > 0 && xOffsetFromTopOfCellToContentOffset < 0) {
          if (newWidth >= oldWidth + xOffsetFromTopOfCellToContentOffset) {
            newXOffsetFromTopOfCellToContentOffset =
                (oldWidth + xOffsetFromTopOfCellToContentOffset) - newWidth;
          } else {
            newXOffsetFromTopOfCellToContentOffset =
                xOffsetFromTopOfCellToContentOffset * (newWidth / oldWidth);
          }
        }
      }

      _sectionModelFlags.shouldAdjustCollectionViewContentOffset = YES;
      _targetContentOffset =
          CGPointMake(_models[row].frame.origin.x - newXOffsetFromTopOfCellToContentOffset, 0);
    } else {
      CGFloat yOffsetFromTopOfCellToContentOffset =
          _models[row].frame.origin.y - context.currentContentOffset.y;
      CGFloat oldHeight = _models[row].frame.size.height;
      [self updateWithUpdates:context.updates];
      [self layoutIfNeededForUICollectionView:collectionView];
      CGFloat newHeight = _models[row].frame.size.height;
      CGFloat newYOffsetFromTopOfCellToContentOffset = 0;
      if (self.fixSelfSizingOffsetFromStart) {
        newYOffsetFromTopOfCellToContentOffset = yOffsetFromTopOfCellToContentOffset;
      } else {
        if (oldHeight > 0 && yOffsetFromTopOfCellToContentOffset < 0) {
          if (newHeight >= oldHeight + yOffsetFromTopOfCellToContentOffset) {
            newYOffsetFromTopOfCellToContentOffset =
                (oldHeight + yOffsetFromTopOfCellToContentOffset) - newHeight;
          } else {
            newYOffsetFromTopOfCellToContentOffset =
                yOffsetFromTopOfCellToContentOffset * (newHeight / oldHeight);
          }
        }
      }

      _sectionModelFlags.shouldAdjustCollectionViewContentOffset = YES;
      _targetContentOffset =
          CGPointMake(0, _models[row].frame.origin.y - newYOffsetFromTopOfCellToContentOffset);
    }
  }
}

- (void)handleInsetsUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.hasInsetUpdates) {
    _firstInvalidIndex = 0;
    _insets = context.insets;
  }
}

- (void)handleCellUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.updates) {
    [self updateWithUpdates:context.updates];
  }
}

- (void)handleMainAxisGapUpdateWithInvalidationContext:(LynxCollectionInvalidationContext*)context {
  if (context.mainAxisGap >= 0 && context.mainAxisGap != _mainAxisGap) {
    _mainAxisGap = context.mainAxisGap;
    _firstInvalidIndex = 0;
  }
}

- (void)handleCrossAxisGapUpdateWithInvalidationContext:
    (LynxCollectionInvalidationContext*)context {
  if (context.crossAxisGap >= 0 && context.crossAxisGap != _crossAxisGap) {
    _crossAxisGap = context.crossAxisGap;
    _firstInvalidIndex = 0;
  }
}

// there is no special handler for 'InitialScrollIndexUpdate,' as this kind of updates doesn't
// modify layout.
- (void)updateWithInvalidationContext:(LynxCollectionInvalidationContext*)context
                       collectionView:(nonnull UICollectionView*)collectionView {
  // system message
  if (context.invalidateEverything) {
    _firstInvalidIndex = 0;
  }

  // user-defined message
  switch (context.invalidationType) {
    // related to data change
    case ListInitialDataUpdate:
      [self handleInitialUpdateWithInvalidationContext:context];
      break;
    case ListRegularDataUpdate:
      [self handleRegularUpdateWithInvalidationContext:context];
      break;
    case ListElementTypeUpdate:
      [self handleElementTypeUpdateWithInvalidationContext:context];
      break;
    // related to property change
    case ListLayoutUpdate:
      [self handleLayoutUpdateWithInvalidationContext:context];
      break;
    case ListColumnCountUpdate:
      [self handleColumnCountUpdateWithInvalidationContext:context];
      break;
    case ListAnimationUpdate:
      [self handleAnimationUpdateWithInvalidationContext:context];
      break;
    case ListInsetsUpdate:
      [self handleInsetsUpdateWithInvalidationContext:context];
      break;
    case ListMainAxisGapUpdate:
      [self handleMainAxisGapUpdateWithInvalidationContext:context];
      break;
    case ListCrossAxisGapUpdate:
      [self handleCrossAxisGapUpdateWithInvalidationContext:context];
      break;
    // related to list-level layout change
    case ListBoundsUpdate:
      [self handleBoundsUpdateWithInvalidationContext:context];
      break;
    // related to component-level layout change
    case ListCellSelfSizingUpdate:
      [self handleSelfSizingWithInvalidationContext:context collectionView:collectionView];
      break;
    case ListCellUpdate:
      [self handleCellUpdateWithInvalidationContext:context];
      break;
    default:
      break;
  }
}

#pragma mark - Public, Animation Related

- (void)prepareForCollectionViewUpdates:(NSArray<UICollectionViewUpdateItem*>*)updateItems {
  if (!_sectionModelFlags.isAnimated) {
    return;
  }
  _sectionModelFlags.isPreparingForCollectionViewUpdates = YES;
  // Find the correspondence between the cells before 'performBatchUpdate' and the cells afters it.
  //
  // For example:
  //                   0   1   2   3   4   5   6
  //                  +-+ +-+ +-+ +-+ +-+ +-+ +-+
  //                  |-| |-| | | | | | | | | | |
  //                  +-+ +-+ +-+ +-+ +++ +++ +++
  //                   +-------^   ^   ^   ^   ^
  //                   |   +-------+   |   |   |
  //                   |   |       +---+   |   +---+
  //                   |   |       |       |       |
  //                  +++ +++ +-+ +++ +-+ +++ +-+ +++ +-+
  //                  | | | | |+| | | |+| | | |+| | | |+|
  //                  +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+
  //                   0   1   2   3   4   5   6   7   8
  //
  // updates: remove(0), remove(1), insert(2), insert(4), insert(6), insert(8)
  // correspondence: (2 - 0), (3 - 1), (4 - 3), (5 - 5), (6 - 7)
  // This correspondence is used when being asked
  // 'finalLayoutAttributesForDisappearingItemAtIndexPath:'. Becasue the indexPath we being queried
  // with is the indexPath before 'performBatchUpdate'. However, the 'layoutAttributes' we have to
  // return have to be found in '_model' with indexPath after 'performBatchUpdate'. Thus we must
  // build this correspondence map.

  NSMutableIndexSet* deleteIndices = [[NSMutableIndexSet alloc] init];
  NSMutableIndexSet* insertIndices = [[NSMutableIndexSet alloc] init];
  for (UICollectionViewUpdateItem* item in updateItems) {
    switch (item.updateAction) {
      case UICollectionUpdateActionDelete:
        [deleteIndices addIndex:item.indexPathBeforeUpdate.row];
        break;
      case UICollectionUpdateActionInsert:
        [insertIndices addIndex:item.indexPathAfterUpdate.row];
        break;
      case UICollectionUpdateActionNone:
      case UICollectionUpdateActionReload:
      case UICollectionUpdateActionMove:
      default:
        break;
    }
  }

  // we have to keep the insertIndices because upon being asked
  // 'initialLayoutAttributesForAppearingItemAtIndexPath:' the 'cellForRowAtIndexPath:' is not
  // called yet. Therefore, 'preferredLayoutAttributeFittingAttribute' is also not being called. The
  // layoutAttribute for this very indexPath has the default Height, which is the height of the
  // screen. Should this layoutAttribute were returned, the animation would be corrupted with weird,
  // non-rigid transformations. The current implementation of our CollectionView cannot provide iOS
  // with a satisfied layoutAttribute. So our choices is either 'return nil' or 'return a fixed
  // position'
  _insertIndices = insertIndices;

  NSInteger itemCountAfter = [_models count];
  NSInteger itemCountDelta = [insertIndices count] - [deleteIndices count];
  NSInteger itemCountBefore = itemCountAfter - itemCountDelta;
  NSMutableArray<NSNumber*>* beforeIndices =
      [[NSMutableArray alloc] initWithCapacity:itemCountBefore];
  NSMutableArray<NSNumber*>* afterIndices =
      [[NSMutableArray alloc] initWithCapacity:itemCountAfter];
  // fill these arrays using iota from 0.
  for (NSUInteger i = 0; i < (NSUInteger)itemCountBefore; ++i) {
    [beforeIndices addObject:[NSNumber numberWithInteger:i]];
  }

  for (NSUInteger i = 0; i < (NSUInteger)itemCountAfter; ++i) {
    [afterIndices addObject:[NSNumber numberWithInteger:i]];
  }
  // remove & insert, simulate what has happened.
  [beforeIndices removeObjectsAtIndexes:deleteIndices];
  [afterIndices removeObjectsAtIndexes:insertIndices];

  NSMutableDictionary<NSNumber*, NSNumber*>* beforeAfterIndexMap =
      [[NSMutableDictionary alloc] initWithCapacity:[beforeIndices count]];
  [beforeIndices
      enumerateObjectsUsingBlock:^(NSNumber* _Nonnull index, NSUInteger idx, BOOL* _Nonnull stop) {
        [beforeAfterIndexMap setObject:[afterIndices objectAtIndex:idx] forKey:index];
      }];

  _beforeAfterIndexMap = beforeAfterIndexMap;
}

- (BOOL)shouldApplyAnimation {
  return _sectionModelFlags.isAnimated && (_sectionModelFlags.isPreparingForCollectionViewUpdates ||
                                           _sectionModelFlags.hasPreparedForCellLayoutUpdate);
}

// what is on screen now --> finalLayoutAttributes --> ? --> initialAttributes --> what ends up on
// screen.
- (__kindof UICollectionViewLayoutAttributes*)
    initialLayoutAttributesForAppearingItemAtIndexPath:(NSIndexPath*)itemIndexPath
                               defaultLayoutAttributes:
                                   (UICollectionViewLayoutAttributes*)defaultAttributes
                                      inCollectionView:
                                          (UICollectionView* __nullable)collectionView {
  if (![self shouldApplyAnimation]) {
    return nil;
  }
  UICollectionViewLayoutAttributes* attributes = nil;

  // The 'inPreparingForCollectionViewUpdates' mode is removed as it will trigger an undesired,
  // duplicate animation. This problem has emerged since iOS 15. MODE:
  // inPreparingForCellLayoutUpdate
  if (_sectionModelFlags.hasPreparedForCellLayoutUpdate) {
    attributes =
        [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:itemIndexPath];
    if ((NSUInteger)itemIndexPath.row < [_snapshotModels count]) {
      attributes.frame = _snapshotModels[itemIndexPath.row].frame;
    }
  }

  return attributes;
}
// what is on screen now --> finalLayoutAttributes --> ? --> initialAttributes --> what ends up on
// screen.
- (__kindof UICollectionViewLayoutAttributes*)finalLayoutAttributesForDisappearingItemAtIndexPath:
    (NSIndexPath*)itemIndexPath {
  if (![self shouldApplyAnimation]) {
    return nil;
  }
  UICollectionViewLayoutAttributes* attributes = nil;

  // MODE: inPreparingForCellLayoutUpdate
  if (_sectionModelFlags.hasPreparedForCellLayoutUpdate) {
    attributes =
        [UICollectionViewLayoutAttributes layoutAttributesForCellWithIndexPath:itemIndexPath];
    if ((NSUInteger)itemIndexPath.row < [_models count]) {
      attributes.frame = _models[itemIndexPath.row].frame;
    }
    return attributes;
  }

  // MODE: inPreparingForCollectionViewUpdates
  NSNumber* beforeIndex = [_beforeAfterIndexMap objectForKey:@(itemIndexPath.row)];
  if (beforeIndex != nil) {
    itemIndexPath = [NSIndexPath indexPathForRow:beforeIndex.integerValue inSection:0];
    attributes = [[self layoutAttributeForElementAtIndexPath:itemIndexPath] copy];
  }
  return attributes;
}

- (void)finalizeCollectionViewUpdates {
  _beforeAfterIndexMap = nil;
  _insertIndices = nil;
  _sectionModelFlags.isPreparingForCollectionViewUpdates = NO;
}

- (void)adjustCollectionViewContentOffsetForSelfSizingCellInvaldationIfNeeded:
    (UICollectionView*)collectionView {
  if (_sectionModelFlags.shouldAdjustCollectionViewContentOffset) {
    // reset shouldAdjustCollectionViewContentOffset immdediately
    // in case of setContentOffset trigger KVO hooked by other components which may invalidateLayout
    // recursively
    _sectionModelFlags.shouldAdjustCollectionViewContentOffset = NO;
    [collectionView setLynxListAdjustingContentOffset:YES];
    [collectionView setContentOffset:_targetContentOffset];
    [collectionView setLynxListAdjustingContentOffset:NO];
  }
}

- (void)prepareForCellLayoutUpdate {
  // take a snapshot for the collectionViewLayout before the update of the cell
  _snapshotModels = [[NSArray alloc] initWithArray:_models copyItems:YES];
  _sectionModelFlags.isPreparingForCellLayoutUpdate = YES;
}

#pragma mark - Private

- (LynxCollectionViewLayoutModel*)defaultLayoutModelAtIndex:(NSUInteger)index {
  CGFloat defaultHeight = [LynxCollectionViewLayoutModel defaultHeight];
  if (_estimatedHeights) {
    NSIndexPath* indexPath = [NSIndexPath indexPathForRow:index inSection:0];
    NSNumber* estimatedHeight = [_estimatedHeights objectForKey:indexPath];
    LynxCollectionViewLayoutModel* model = nil;
    if (estimatedHeight != nil && [estimatedHeight doubleValue] > 0) {
      // if the estimatedHeight for current indexPath is valid, use this height for the default
      // model.
      CGFloat estimatedHeightDouble = [estimatedHeight doubleValue];
      if (_horizontalLayout) {
        model = [LynxCollectionViewLayoutModel modelWithWidth:estimatedHeightDouble];
      } else {
        model = [LynxCollectionViewLayoutModel modelWithHeight:estimatedHeightDouble];
      }
    } else if (index == 0) {
      // if the estimatedHeight for current indexPath is not valid
      // besides, this model is the first model, use this global default height.
      model = [LynxCollectionViewLayoutModel modelWithDefaultSize];
    } else {
      // if the estimatedHeight for current indexPath is not valid
      // and this model is not the first model, use the default height of the model before this one.
      LynxCollectionViewLayoutModel* prevModel = [_models objectAtIndex:(index - 1)];
      if (_horizontalLayout) {
        CGFloat prevWidth = [LynxCollectionViewLayoutModel defaultWidth];
        if (prevModel) {
          prevWidth = [_models objectAtIndex:(index - 1)].frame.size.width;
        }
        model = [LynxCollectionViewLayoutModel modelWithWidth:prevWidth];
      } else {
        CGFloat prevHeight = defaultHeight;
        if (prevModel) {
          prevHeight = [_models objectAtIndex:(index - 1)].frame.size.height;
        }
        model = [LynxCollectionViewLayoutModel modelWithHeight:prevHeight];
      }
    }
    return model;
  } else {
    // no estimatedHeight provided, use global defaultHeight instead.
    return [LynxCollectionViewLayoutModel modelWithDefaultSize];
  }
}

- (void)resetModelsWithLength:(NSUInteger)length {
  _models = [NSMutableArray arrayWithCapacity:length];
  for (NSUInteger i = 0; i < length; ++i) {
    [_models addObject:[self defaultLayoutModelAtIndex:i]];
  }
}

- (void)resetMainSizesWithNumberOfColumns:(NSUInteger)numberOfColumns {
  _mainSizes = [[NSMutableArray alloc] initWithCapacity:_numberOfColumns];
  for (NSUInteger i = 0; i < numberOfColumns; ++i) {
    [_mainSizes addObject:[NSNumber numberWithFloat:0.]];
  }
}

- (NSInteger)zIndexForRow:(NSIndexPath*)indexPath {
  if ([_stickyTopItems containsObject:indexPath] || [_stickyBottomItems containsObject:indexPath]) {
    // If cell is sticky, assign it with max zIndex, so it can cover commen cells.
    return NSIntegerMax;
  }
  return indexPath.row;
}

- (BOOL)isStickyItem:(NSInteger)item {
  return item == _stickyTopIndex || item == _stickyBottomIndex;
}

- (UICollectionViewLayoutAttributes*)layoutAttributesFromCacheAtRow:(NSInteger)row {
  if (!((NSUInteger)row < [_models count])) {
    return nil;
  }
  UICollectionViewLayoutAttributes* attributes = _layoutAttributesCache[@(row)];
  NSIndexPath* indexPathWithoutSection = [NSIndexPath indexPathForRow:row inSection:0];
  if (attributes == nil) {
    attributes = [UICollectionViewLayoutAttributes
        layoutAttributesForCellWithIndexPath:indexPathWithoutSection];
    [_layoutAttributesCache setObject:attributes forKey:[NSNumber numberWithInteger:row]];
  }
  // update layoutAttribute to sync with its model
  attributes.frame = _models[attributes.indexPath.row].frame;
  if (self.indexAsZIndex) {
    if ([_stickyTopItems containsObject:indexPathWithoutSection] ||
        [_stickyBottomItems containsObject:indexPathWithoutSection]) {
      // zIndex for all sticky items are greater than items that are not sticky.
      attributes.zIndex = [_models count] + row;
    } else {
      // cells with the same zIndex have an undetermined order.
      // we assign zIndex with index to make __overflowed__ content visible to cells before it.
      attributes.zIndex = row;
    }
  } else {
    attributes.zIndex = [_fullSpanItems containsObject:indexPathWithoutSection] ? 1 : 0;
  }
  return attributes;
}

- (void)retrieveMainSizeFromCacheAtInvalidIndex:(NSInteger)invalidIndex {
  if (invalidIndex == 0) {
    // if the invalidation is from the beginning, just reset it.
    [_mainSizesCache removeAllObjects];
    [self resetMainSizesWithNumberOfColumns:_numberOfColumns];
    return;
  } else if (invalidIndex == kLynxCollectionViewLayoutInvalidIndex) {
    // if there is no invalidation, do nothing
    return;
  }
  // retrieve the columnHeight we will start to compute with
  _mainSizes = [NSMutableArray arrayWithArray:_mainSizesCache[invalidIndex - 1]];
  // removal all trailing mainSizes
  [_mainSizesCache
      removeObjectsInRange:NSMakeRange(invalidIndex, [_mainSizesCache count] - invalidIndex)];
}

- (NSUInteger)numberOfFullSpanBeforeIndex:(NSUInteger)index {
  NSUInteger __block fullSpanCount = 0;
  // check how many fullspan items before BeginIndex
  [_fullSpanItems enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull fullspanIndex, NSUInteger idx,
                                               BOOL* _Nonnull stop) {
    if ((NSUInteger)fullspanIndex.row < index) {
      ++fullSpanCount;
    }
  }];

  return fullSpanCount;
}

- (NSInteger)findNearestFullSpanItem:(NSUInteger)index {
  if (_layoutType != LynxCollectionViewLayoutFlow) {
    return 0;
  }
  NSInteger __block nearestFullSpanItem = kLynxCollectionViewLayoutInvalidIndex;
  // Find the nearest fullspan item and layout from it. fullspan items is ordered
  [_fullSpanItems enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull fullspanIndex, NSUInteger idx,
                                               BOOL* _Nonnull stop) {
    if ((NSUInteger)fullspanIndex.row < index) {
      nearestFullSpanItem = fullspanIndex.row;
    } else {
      *stop = YES;
    }
  }];
  return nearestFullSpanItem;
}

// the method is to determine whether an item in in the first row.
// if so, add 'gap' to its offset.
- (CGFloat)adjustOffsetAtIndex:(NSUInteger)index
                originalOffset:(CGFloat)Offset
                 fullSpanCount:(NSUInteger)count {
  if (count > 0) {
    return Offset + self.mainAxisGap;
  }
  if (index < self.numberOfColumns) {
    return Offset;
  } else {
    return Offset + self.mainAxisGap;
  }
}

- (CGFloat)adjustOffsetAtIndexForFlow:(NSUInteger)index
                       originalOffset:(CGFloat)Offset
           closestFullSpanBeforeIndex:(NSInteger)closestFullSpanBeforeIndex {
  if (_layoutType != LynxCollectionViewLayoutFlow) {
    return 0.f;
  }
  if (closestFullSpanBeforeIndex >= 0 && (NSInteger)index > closestFullSpanBeforeIndex) {
    return Offset + self.mainAxisGap;
  }
  if (index < self.numberOfColumns) {
    return Offset;
  } else {
    return Offset + self.mainAxisGap;
  }
}

- (CGFloat)largestMainSizeInPreviousRowAtIndex:(NSUInteger)index
                          withNumberOfFullSpan:(NSUInteger)fullSpanCount {
  // try to find the Yoffset of the last item at previous row
  //                                   lastItemForPreviousRow is -1
  //  lastItemForPreviousRow                         +
  //             +                                   |
  //             |                                   |
  //             v    +lastYOffset                   v
  //  +--+--+--+-+-+  |               +---+---+--+---++
  //  |  |  |  |   |  |               |   |   |  |    ^
  //  |  |  +--+   |  |               |   |   |  |    |
  //  +--+  |  |   |  |               |   +---+  |    +
  //     |  |  |   |  |               |   |   |  | first row
  //     +--+  |   |  v               +---+   |  |
  //+-+--------+---+--+                       +--+
  //  |  |  |  |
  //  |  +--+  |
  //  |  |  |  |
  //  +--+  |  |
  //        +-++
  //          ^
  //          |
  //          +columnForBeginIndex = 2
  CGFloat lastOffset = 0;
  NSInteger columnForBeginIndex = (index - fullSpanCount) % _numberOfColumns;
  NSInteger lastItemInPreviousRowIndex = index - columnForBeginIndex - 1;
  if (lastItemInPreviousRowIndex == kLynxCollectionViewLayoutInvalidIndex) {
    lastOffset = 0;
  } else {
    NSArray<NSNumber*>* prevSizes = _mainSizesCache[lastItemInPreviousRowIndex];
    lastOffset = [self adjustOffsetAtIndex:index
                            originalOffset:[self largestSizeInMainSizes:prevSizes]
                             fullSpanCount:fullSpanCount];
  }
  return lastOffset;
}

- (CGFloat)largestMainSizeInPreviousRowAtIndexForFlow:(NSUInteger)index
                           closestFullSpanBeforeIndex:(NSInteger)closestFullSpanBeforeIndex {
  if (_layoutType != LynxCollectionViewLayoutFlow) {
    return 0.f;
  }
  CGFloat lastOffset = 0;
  NSInteger columnForBeginIndex = [self findColumnIndexForFlow:index
                                withClosestFullSpanIndexBefore:closestFullSpanBeforeIndex];
  NSInteger lastItemInPreviousRowIndex = index - columnForBeginIndex - 1;
  if (lastItemInPreviousRowIndex == kLynxCollectionViewLayoutInvalidIndex) {
    lastOffset = 0;
  } else {
    NSArray<NSNumber*>* prevSizes = _mainSizesCache[lastItemInPreviousRowIndex];
    lastOffset = [self adjustOffsetAtIndexForFlow:index
                                   originalOffset:[self largestSizeInMainSizes:prevSizes]
                       closestFullSpanBeforeIndex:closestFullSpanBeforeIndex];
  }
  return lastOffset;
}

- (NSInteger)findColumnIndexForFlow:(NSInteger)index
     withClosestFullSpanIndexBefore:(NSInteger)closestFullSpanIndexBefore {
  if (_layoutType != LynxCollectionViewLayoutFlow) {
    return 0;
  }
  NSInteger columnForBeginIndex = 0;
  if (closestFullSpanIndexBefore >= 0) {
    columnForBeginIndex = (index - closestFullSpanIndexBefore - 1) % _numberOfColumns;
  } else {
    columnForBeginIndex = index % _numberOfColumns;
  }
  return columnForBeginIndex;
}

// paddingStart = paddingLeft(vertical scroll) or paddingTop(horizontal scroll)
// paddingEnd = paddingRight(vertical scroll) or paddingBottom(horizontal scroll)
CGFloat LynxCollectionViewLayoutOffsetForFullSpanItems(CGFloat itemSize, CGFloat collectionSize,
                                                       CGFloat paddingStart, CGFloat paddingEnd) {
  CGFloat remainingWidth = collectionSize - itemSize;

  if (remainingWidth <= 0.) {
    return 0.;
  }

  remainingWidth -= paddingStart + paddingEnd;

  if (remainingWidth >= 0) {
    return paddingStart;
  }

  if (paddingStart > 1. && paddingEnd > 1.) {
    // if the width of the header is greater than the remaingWidth
    // we just apply padding on both side with same scale
    CGFloat scale = paddingStart / (paddingStart + paddingEnd);
    return paddingStart + (remainingWidth * scale);
  }

  return 0.;
}

- (void)layoutFromIndex:(NSInteger)beginIndex {
  NSUInteger numberOfColumns = self.numberOfColumns;
  // width that could be used for content = list width - left padding - right padding
  CGFloat cellContentAreaSize = 0;
  if (_horizontalLayout) {
    cellContentAreaSize = CGRectGetHeight(_bounds) - (_insets.top + _insets.bottom) -
                          (numberOfColumns - 1) * _crossAxisGap;
  } else {
    cellContentAreaSize = CGRectGetWidth(_bounds) - (_insets.left + _insets.right) -
                          (numberOfColumns - 1) * _crossAxisGap;
  }

  NSUInteger itemCount = [_models count];

  CGFloat normalItemCrossSize =
      [LynxUIUnitUtils roundPtToPhysicalPixel:(cellContentAreaSize / numberOfColumns)];

  NSUInteger fullSpanCountBefore = [self numberOfFullSpanBeforeIndex:beginIndex];
  NSInteger closestFullSpanItemBefore = [self findNearestFullSpanItem:beginIndex];
  CGFloat lastOffset = [self largestMainSizeInPreviousRowAtIndex:beginIndex
                                            withNumberOfFullSpan:fullSpanCountBefore];
  CGFloat lastOffsetForFlow =
      [self largestMainSizeInPreviousRowAtIndexForFlow:beginIndex
                            closestFullSpanBeforeIndex:closestFullSpanItemBefore];

  for (NSUInteger item = beginIndex; item < itemCount; item++) {
    CGFloat mainAxisOffset = 0.0f;
    NSUInteger columnIndex = 0;
    BOOL isFullSpan = [self.fullSpanItems containsObject:[NSIndexPath indexPathForRow:item
                                                                            inSection:0]];
    CGFloat itemCrossSize = 0, itemMainSize = 0;
    if (_horizontalLayout) {
      itemCrossSize = isFullSpan ? CGRectGetHeight(_bounds) : normalItemCrossSize;
      itemMainSize = [_models objectAtIndex:item].frame.size.width;
    } else {
      itemCrossSize = isFullSpan ? CGRectGetWidth(_bounds) : normalItemCrossSize;
      itemMainSize = [_models objectAtIndex:item].frame.size.height;
    }
    CGFloat itemMainSizeForLayout = ceil(itemMainSize);
    CGFloat crossAxisOffset = 0;

    if (isFullSpan) {
      columnIndex = 0;
      mainAxisOffset = [self adjustOffsetAtIndex:item
                                  originalOffset:[self largestMainSize]
                                   fullSpanCount:fullSpanCountBefore];
      fullSpanCountBefore++;
      closestFullSpanItemBefore = item;
      if (_horizontalLayout) {
        crossAxisOffset = LynxCollectionViewLayoutOffsetForFullSpanItems(
            CGRectGetHeight(_models[item].frame) - _insets.top - _insets.bottom,
            CGRectGetHeight(_bounds), _insets.top, _insets.bottom);
      } else {
        crossAxisOffset = LynxCollectionViewLayoutOffsetForFullSpanItems(
            CGRectGetWidth(_models[item].frame) - _insets.left - _insets.right,
            CGRectGetWidth(_bounds), _insets.left, _insets.right);
      }

      for (NSUInteger i = 0; i < [_mainSizes count]; ++i) {
        _mainSizes[i] =
            @(mainAxisOffset + (_needAlignHeight ? itemMainSizeForLayout : itemMainSize));
      }
    } else {
      if (_layoutType == LynxCollectionViewLayoutWaterfall) {
        columnIndex = [self shortestColumn];
        mainAxisOffset = [self adjustOffsetAtIndex:item
                                    originalOffset:[self shortestMainSize]
                                     fullSpanCount:fullSpanCountBefore];
      } else if (_layoutType == LynxCollectionViewLayoutFlow) {
        columnIndex = [self findColumnIndexForFlow:item
                    withClosestFullSpanIndexBefore:closestFullSpanItemBefore];
        if (columnIndex == 0) {
          mainAxisOffset = [self adjustOffsetAtIndexForFlow:item
                                             originalOffset:[self largestMainSize]
                                 closestFullSpanBeforeIndex:closestFullSpanItemBefore];
          lastOffsetForFlow = mainAxisOffset;
        } else {
          mainAxisOffset = lastOffsetForFlow;
        }
      } else {
        columnIndex = (item - fullSpanCountBefore) % numberOfColumns;
        if (columnIndex == 0) {
          mainAxisOffset = [self adjustOffsetAtIndex:item
                                      originalOffset:[self largestMainSize]
                                       fullSpanCount:fullSpanCountBefore];
          lastOffset = mainAxisOffset;
        } else {
          mainAxisOffset = lastOffset;
        }
      }
      if (_horizontalLayout) {
        crossAxisOffset = _insets.top + ((itemCrossSize + _crossAxisGap) * columnIndex);
      } else {
        crossAxisOffset = _insets.left + ((itemCrossSize + _crossAxisGap) * columnIndex);
      }
      _mainSizes[columnIndex] =
          @(mainAxisOffset + (_needAlignHeight ? itemMainSizeForLayout : itemMainSize));
    }
    [_mainSizesCache addObject:[NSArray arrayWithArray:_mainSizes]];
    if (_horizontalLayout) {
      _models[item].frame =
          CGRectMake(mainAxisOffset, crossAxisOffset, itemMainSize, itemCrossSize);
    } else {
      _models[item].frame =
          CGRectMake(crossAxisOffset, mainAxisOffset, itemCrossSize, itemMainSize);
    }
  }
}

- (void)updateFirstInvalidIndexWithIndex:(NSInteger)index {
  if (_firstInvalidIndex == kLynxCollectionViewLayoutInvalidIndex) {
    _firstInvalidIndex = index;
  }
  _firstInvalidIndex = MIN(_firstInvalidIndex, index);
}

- (void)updateWithRemovals:(NSArray<NSIndexPath*>* __nullable)removals {
  if (removals == nil || [removals count] == 0) {
    return;
  }
  NSMutableIndexSet* indicesToRemove = [[NSMutableIndexSet alloc] init];
  [removals enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull indexPath, NSUInteger idx,
                                         BOOL* _Nonnull stop) {
    [indicesToRemove addIndex:indexPath.row];
  }];
  [self updateFirstInvalidIndexWithIndex:[indicesToRemove firstIndex]];
  [_models removeObjectsAtIndexes:indicesToRemove];
}

- (void)updateWithInsertions:(NSArray<NSIndexPath*>* __nullable)insertions {
  if (insertions == nil) {
    return;
  }
  [insertions enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull indexPath, NSUInteger idx,
                                           BOOL* _Nonnull stop) {
    [self updateFirstInvalidIndexWithIndex:indexPath.row];
    [self.models insertObject:[self defaultLayoutModelAtIndex:indexPath.row] atIndex:indexPath.row];
  }];
}

- (LynxCollectionViewLayoutMoveUpdate)updateWithMoveFrom:
    (NSArray<NSIndexPath*>* __nullable)moveFrom {
  if (moveFrom == nil) {
    return nil;
  }
  NSMutableArray<LynxCollectionViewLayoutModel*>* moveFromModels =
      [NSMutableArray arrayWithCapacity:[moveFrom count]];
  [moveFrom enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull indexPath, NSUInteger idx,
                                         BOOL* _Nonnull stop) {
    [moveFromModels addObject:[self.models objectAtIndex:indexPath.row]];
  }];
  return ^void(NSArray<NSIndexPath*>* moveTo) {
    if (moveTo == nil || [moveTo count] != [moveFromModels count]) {
      return;
    }

    [moveTo enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull indexPath, NSUInteger idx,
                                         BOOL* _Nonnull stop) {
      [self updateFirstInvalidIndexWithIndex:indexPath.row];
      [self.models replaceObjectAtIndex:indexPath.row
                             withObject:[[moveFromModels objectAtIndex:idx] copy]];
    }];
  };
}

- (void)updateWithUpdates:(NSDictionary<NSIndexPath*, LynxCollectionViewLayoutModel*>*)updates {
  if (updates == nil) {
    return;
  }
  [updates enumerateKeysAndObjectsUsingBlock:^(NSIndexPath* _Nonnull indexPath,
                                               LynxCollectionViewLayoutModel* _Nonnull model,
                                               BOOL* _Nonnull stop) {
    CGSize oldSize = [self.models objectAtIndex:indexPath.row].frame.size;
    CGSize newSize = model.frame.size;
    if (!CGSizeEqualToSize(oldSize, newSize)) {
      [self updateFirstInvalidIndexWithIndex:indexPath.row];
      LynxCollectionViewLayoutModel* oldModel = [self.models objectAtIndex:indexPath.row];
      oldModel.frame = model.frame;
    }
  }];
}

#pragma mark - Helper

- (CGFloat)largestSizeInMainSizes:(NSArray<NSNumber*>*)mainSizes {
  __block CGFloat size = 0;
  [mainSizes
      enumerateObjectsUsingBlock:^(NSNumber* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        size = MAX(size, obj.floatValue);
      }];
  return size;
}

- (CGFloat)largestMainSize {
  return [self largestSizeInMainSizes:_mainSizes];
}

- (NSUInteger)shortestColumn {
  NSArray<NSNumber*>* mainSizes = _mainSizes;
  __block CGFloat size = CGFLOAT_MAX;
  __block NSUInteger index = 0;
  [mainSizes
      enumerateObjectsUsingBlock:^(NSNumber* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        if (size > obj.floatValue) {
          size = obj.floatValue;
          index = idx;
        }
      }];
  return index;
}

- (CGFloat)shortestMainSize {
  NSArray<NSNumber*>* mainSizes = _mainSizes;
  __block CGFloat size = CGFLOAT_MAX;
  [mainSizes
      enumerateObjectsUsingBlock:^(NSNumber* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        size = MIN(size, obj.floatValue);
      }];
  return size;
}

- (void)setBounds:(CGRect)newBounds {
  _bounds = newBounds;
  _sectionModelFlags.boundsChanged = YES;
}

/* when bounds change, need find the sticky items */
- (void)retrieveStickyItemsForBounds:(CGRect)newBounds ForUICollectionView:(UICollectionView*)view {
  _stickyTopIndex = kLynxCollectionViewLayoutInvalidIndex;
  _stickyBottomIndex = kLynxCollectionViewLayoutInvalidIndex;
  _stickyTopItemAttribute = nil;
  _stickyBottomItemAttribute = nil;

  if (self.fullSpanItems == nil || [self.fullSpanItems count] == 0) {
    return;
  }

  // step 1, find first and last visible cell
  NSInteger __block firstCellPosition = 0;
  NSInteger __block lastCellPosition = [_models count] - 1;
  CGFloat __block stickTopLineY = view.contentOffset.y + _stickyOffset;
  CGFloat __block stickBottomLineY = view.contentOffset.y + newBounds.size.height - _stickyOffset;
  [_models enumerateObjectsUsingBlock:^(LynxCollectionViewLayoutModel* _Nonnull model,
                                        NSUInteger idx, BOOL* _Nonnull stop) {
    if (CGRectIntersectsRect(newBounds, model.frame)) {
      if (model.frame.origin.y <= stickTopLineY &&
          model.frame.origin.y + model.frame.size.height > stickTopLineY) {
        firstCellPosition = idx;
      }

      if (model.frame.origin.y < stickBottomLineY &&
          model.frame.origin.y + model.frame.size.height >= stickBottomLineY) {
        lastCellPosition = idx;
      }
    }
  }];

  // step 2, find fullspan item before first visible and fullspan item after last visible
  NSInteger fullSpanAfterStickyTopIndex = kLynxCollectionViewLayoutInvalidIndex;
  NSInteger fullSpanBeforeStickyBottomIndex = kLynxCollectionViewLayoutInvalidIndex;
  // find fullspan before first visible and after last visible
  for (NSUInteger i = 0; i < _fullSpanItems.count; ++i) {
    if (_fullSpanItems[i].row <= firstCellPosition) {
      _stickyTopIndex = _fullSpanItems[i].row;
    }
    if (_fullSpanItems[i].row < lastCellPosition) {
      fullSpanBeforeStickyBottomIndex = _fullSpanItems[i].row;
    }
  }
  for (NSInteger i = _fullSpanItems.count - 1; i >= 0; --i) {
    if (_fullSpanItems[i].row >= lastCellPosition && _fullSpanItems[i].row > _stickyTopIndex) {
      _stickyBottomIndex = _fullSpanItems[i].row;
    }
    if (_fullSpanItems[i].row > firstCellPosition) {
      fullSpanAfterStickyTopIndex = _fullSpanItems[i].row;
    }
  }

  // step 3, check sticky-top and adjust attribute
  if ([self.stickyTopItems containsObject:[NSIndexPath indexPathForRow:_stickyTopIndex
                                                             inSection:0]]) {
    UICollectionViewLayoutAttributes* attribute =
        [self layoutAttributesFromCacheAtRow:_stickyTopIndex];
    _stickyTopItemAttribute = [attribute copy];

    CGFloat stickyTopY = _stickyTopItemAttribute.frame.origin.y;
    if (view.contentOffset.y > 0 && stickyTopY < view.contentOffset.y + _stickyOffset) {
      stickyTopY = view.contentOffset.y + _stickyOffset;
    }
    if (view.contentOffset.y < 0) {
      stickyTopY = view.contentOffset.y + _stickyOffset;
    }
    if (fullSpanAfterStickyTopIndex != kLynxCollectionViewLayoutInvalidIndex &&
        fullSpanAfterStickyTopIndex != _stickyBottomIndex) {
      UICollectionViewLayoutAttributes* nextFullSpanAttribute =
          [self layoutAttributesFromCacheAtRow:fullSpanAfterStickyTopIndex];
      CGFloat maxStickyTopY =
          nextFullSpanAttribute.frame.origin.y - _stickyTopItemAttribute.frame.size.height;
      if (stickyTopY > maxStickyTopY) {
        stickyTopY = maxStickyTopY;
      }
    }
    _stickyTopItemAttribute.frame =
        CGRectMake(attribute.frame.origin.x, stickyTopY, attribute.frame.size.width,
                   attribute.frame.size.height);
    _stickyTopItemAttribute.zIndex = NSIntegerMax;
  } else {
    _stickyTopIndex = kLynxCollectionViewLayoutInvalidIndex;
    _stickyTopItemAttribute = nil;
  }

  // step 4, check sticky-bottom and then adjust attribute
  if ([self.stickyBottomItems containsObject:[NSIndexPath indexPathForRow:_stickyBottomIndex
                                                                inSection:0]]) {
    UICollectionViewLayoutAttributes* attribute =
        [self layoutAttributesFromCacheAtRow:_stickyBottomIndex];
    _stickyBottomItemAttribute = [attribute copy];

    CGFloat stickyBottomY = view.contentOffset.y + newBounds.size.height -
                            _stickyBottomItemAttribute.frame.size.height - _stickyOffset;
    if (fullSpanBeforeStickyBottomIndex != kLynxCollectionViewLayoutInvalidIndex &&
        fullSpanBeforeStickyBottomIndex != _stickyTopIndex) {
      UICollectionViewLayoutAttributes* previousFullSpanAttribute =
          [self layoutAttributesFromCacheAtRow:fullSpanBeforeStickyBottomIndex];
      CGFloat previousFullSpanBottomY =
          previousFullSpanAttribute.frame.origin.y + previousFullSpanAttribute.frame.size.height;
      if (stickyBottomY < previousFullSpanBottomY) {
        stickyBottomY = previousFullSpanBottomY;
      }
    }
    _stickyBottomItemAttribute.frame =
        CGRectMake(attribute.frame.origin.x, stickyBottomY, attribute.frame.size.width,
                   attribute.frame.size.height);
    _stickyTopItemAttribute.zIndex = NSIntegerMax;
  } else {
    _stickyBottomIndex = kLynxCollectionViewLayoutInvalidIndex;
    _stickyBottomItemAttribute = nil;
  }
}

- (void)updateStickyTopItemsForUICollectionView:(UICollectionView*)view {
  CGFloat top = (self.stickyWithBounces ? MAX(view.contentOffset.y, 0) : view.contentOffset.y) +
                self.stickyOffset;
  __block NSInteger minIndex = NSIntegerMax;
  __block NSIndexPath* targetStickyTopIndex = nil;
  __block LynxCollectionViewLayoutModel* targetStickyTopModel = nil;

  [self.stickyTopItems
      enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        NSInteger index = obj.item;
        if (index >= 0 && index < (NSInteger)self.models.count) {
          LynxCollectionViewLayoutModel* model = self.models[index];
          if (model.frame.origin.y <= top) {
            if (!targetStickyTopIndex || index >= targetStickyTopIndex.item) {
              targetStickyTopIndex = obj;
              targetStickyTopModel = model;
            }
          }
          minIndex = MIN(minIndex, index);
        }
      }];

  if (!targetStickyTopIndex && minIndex != NSIntegerMax &&
      self.models[minIndex].frame.origin.y == 0) {
    targetStickyTopIndex = [NSIndexPath indexPathForItem:minIndex inSection:0];
    targetStickyTopModel = self.models[minIndex];
  }

  if (targetStickyTopIndex) {
    __block NSIndexPath* nextStickyTopIndex = nil;
    __block LynxCollectionViewLayoutModel* nextStickyTopModel = nil;

    [self.stickyTopItems enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull obj, NSUInteger idx,
                                                      BOOL* _Nonnull stop) {
      NSInteger index = obj.item;
      if (index >= 0 && index < (NSInteger)self.models.count) {
        if (index > targetStickyTopIndex.item &&
            (nextStickyTopIndex == nil || index < nextStickyTopIndex.item)) {
          nextStickyTopIndex = obj;
          nextStickyTopModel = self.models[index];
        }
      }
    }];

    _stickyTopIndex = targetStickyTopIndex.item;
    UICollectionViewLayoutAttributes* attribute =
        [self layoutAttributesFromCacheAtRow:_stickyTopIndex];
    _stickyTopItemAttribute = [attribute copy];

    CGFloat targetStickyTop = top;
    if (nextStickyTopIndex != nil) {
      if (nextStickyTopModel.frame.origin.y - top < targetStickyTopModel.frame.size.height) {
        targetStickyTop =
            nextStickyTopModel.frame.origin.y - targetStickyTopModel.frame.size.height;
      }
    }

    _stickyTopItemAttribute.frame = CGRectMake(
        _stickyTopItemAttribute.frame.origin.x, targetStickyTop,
        _stickyTopItemAttribute.frame.size.width, _stickyTopItemAttribute.frame.size.height);
    _stickyTopItemAttribute.zIndex = NSIntegerMax;
  } else {
    _stickyTopIndex = kLynxCollectionViewLayoutInvalidIndex;
    _stickyTopItemAttribute = nil;
  }
}

- (void)updateStickyBottomItemsForUICollectionView:(UICollectionView*)view {
  CGFloat bottom = (self.stickyWithBounces ? MIN(view.contentOffset.y,
                                                 view.contentSize.height - view.bounds.size.height)
                                           : view.contentOffset.y) +
                   view.bounds.size.height - self.stickyOffset;

  __block NSInteger maxIndex = -1;
  __block NSIndexPath* targetStickyBottomIndex = nil;
  __block LynxCollectionViewLayoutModel* targetStickyBottomModel = nil;

  [self.stickyBottomItems
      enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        NSInteger index = obj.item;
        if (index >= 0 && index < (NSInteger)self.models.count) {
          LynxCollectionViewLayoutModel* model = self.models[index];
          if (CGRectGetMaxY(model.frame) >= bottom) {
            if (!targetStickyBottomIndex || index <= targetStickyBottomIndex.item) {
              targetStickyBottomIndex = obj;
              targetStickyBottomModel = model;
            }
          }
          maxIndex = MAX(maxIndex, (NSInteger)index);
        }
      }];

  if (!targetStickyBottomIndex && maxIndex != -1 &&
      CGRectGetMaxY(self.models[maxIndex].frame) == view.contentSize.height) {
    targetStickyBottomIndex = [NSIndexPath indexPathForItem:maxIndex inSection:0];
    targetStickyBottomModel = self.models[maxIndex];
  }

  if (targetStickyBottomIndex) {
    __block NSIndexPath* nextStickyBottomIndex = nil;
    __block LynxCollectionViewLayoutModel* nextStickyBottomModel = nil;

    [self.stickyBottomItems enumerateObjectsUsingBlock:^(NSIndexPath* _Nonnull obj, NSUInteger idx,
                                                         BOOL* _Nonnull stop) {
      NSInteger index = obj.item;
      if (index >= 0 && index < (NSInteger)self.models.count) {
        if (index < targetStickyBottomIndex.item &&
            (nextStickyBottomIndex == nil || index > nextStickyBottomIndex.item)) {
          nextStickyBottomIndex = obj;
          nextStickyBottomModel = self.models[index];
        }
      }
    }];

    _stickyBottomIndex = targetStickyBottomIndex.item;
    UICollectionViewLayoutAttributes* attribute =
        [self layoutAttributesFromCacheAtRow:_stickyBottomIndex];
    _stickyBottomItemAttribute = [attribute copy];

    CGFloat targetStickyTop = bottom - targetStickyBottomModel.frame.size.height;
    if (nextStickyBottomIndex != nil) {
      if (CGRectGetMaxY(nextStickyBottomModel.frame) > targetStickyTop) {
        targetStickyTop = CGRectGetMaxY(nextStickyBottomModel.frame);
      }
    }

    _stickyBottomItemAttribute.frame = CGRectMake(
        _stickyBottomItemAttribute.frame.origin.x, targetStickyTop,
        _stickyBottomItemAttribute.frame.size.width, _stickyBottomItemAttribute.frame.size.height);
    _stickyBottomItemAttribute.zIndex = NSIntegerMax;
  } else {
    _stickyBottomIndex = kLynxCollectionViewLayoutInvalidIndex;
    _stickyBottomItemAttribute = nil;
  }
}

- (void)updateStickyItemsForUICollectionView:(UICollectionView*)view {
  [self updateStickyTopItemsForUICollectionView:view];
  [self updateStickyBottomItemsForUICollectionView:view];
}

@end
