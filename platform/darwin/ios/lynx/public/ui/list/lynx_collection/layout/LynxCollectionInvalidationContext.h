// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxCollectionViewLayoutModel.h"

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSInteger const LynxCollectionInvalidNumberOfItems;
FOUNDATION_EXPORT NSInteger const LynxCollectionInvalidNumberOfColumns;

typedef NS_ENUM(NSUInteger, InvalidationType) {
  // default
  ListNoneUpdate,
  // set inside 'applyFirstTime' and 'apply', respectively
  ListInitialDataUpdate,
  ListRegularDataUpdate,
  // set inside 'setListType'
  ListLayoutUpdate,
  // set inside 'setColumnCount'
  ListColumnCountUpdate,
  // set when the bounds of the collectionview changes
  ListBoundsUpdate,
  // set when the insets of the collectionview changes
  ListInsetsUpdate,
  // set when the bounds of a cell changes; this usually happens when a cell's height is updated by
  // the calculated results
  ListCellUpdate,
  // set inside 'setUpdateAnimation'
  ListAnimationUpdate,
  // set inside ‘invalidationContextForPreferredLayoutAttributes’; this usually happens when a
  // cell's height changes and the collectionview isn't aware of.
  ListCellSelfSizingUpdate,
  // set inside ‘setInitialScrollIndex’
  ListInitialScrollIndexUpdate,
  // set inside 'setGaps' methods
  ListMainAxisGapUpdate,
  ListCrossAxisGapUpdate,
  // set full span items or sticky items
  ListElementTypeUpdate,
};

@class LynxCollectionViewLayoutModel;

@interface LynxCollectionInvalidationContext : UICollectionViewLayoutInvalidationContext

- (instancetype)initWithNumberOfItemsChanging:(NSInteger)number
                                fullSpanItems:(NSArray<NSIndexPath*>*)fullSpanItems
                               stickyTopItems:(NSArray<NSIndexPath*>*)stickyTopItems
                            stickyBottomItems:(NSArray<NSIndexPath*>*)stickyBottomItems
                             estimatedHeights:
                                 (NSDictionary<NSIndexPath*, NSNumber*>*)estimatedHeights;
- (instancetype)initWithFullSpanItems:(NSArray<NSIndexPath*>*)fullSpanItems
                       stickyTopItems:(NSArray<NSIndexPath*>*)stickyTopItems
                    stickyBottomItems:(NSArray<NSIndexPath*>*)stickyBottomItems
                     estimatedHeights:(NSDictionary<NSIndexPath*, NSNumber*>*)estimatedHeights;

- (instancetype)initWithLayoutTypeSwitching:(LynxCollectionViewLayoutType)type;
- (instancetype)initWithNumberOfColumnsChanging:(NSUInteger)numberOfColumns;
- (instancetype)initWithMainAxisGapChanging:(CGFloat)mainAxisGap;
- (instancetype)initWithCrossAxisGapChanging:(CGFloat)crossAxisGap;

- (instancetype)initWithBoundsChanging:(CGRect)bounds;
- (instancetype)initWithInsetChanging:(UIEdgeInsets)insets;

- (instancetype)initWithRemovals:(NSArray<NSIndexPath*>*)removals
                      insertions:(NSArray<NSIndexPath*>*)insertions
                        moveFrom:(NSArray<NSIndexPath*>*)moveFrom
                          moveTo:(NSArray<NSIndexPath*>*)moveTo
                   fullSpanItems:(NSArray<NSIndexPath*>*)fullSpanItems
                  stickyTopItems:(NSArray<NSIndexPath*>*)stickyTopItems
               stickyBottomItems:(NSArray<NSIndexPath*>*)stickyBottomItems
                estimatedHeights:(NSDictionary<NSIndexPath*, NSNumber*>*)estimatedHeights;

- (instancetype)initWithUpdateAtIndexPath:(NSIndexPath*)indexPath bounds:(CGRect)bounds;
- (instancetype)initWithResetAnimationTo:(BOOL)animated;
- (instancetype)initWithSelfSizingCellAtIndexPath:(NSIndexPath*)indexPath
                                           bounds:(CGRect)bounds
                                   collectionView:(UICollectionView*)collectionView
                                     isHorizontal:(BOOL)isHorizontal;
- (instancetype)initWithInitialScrollIndexSet;

@property(nonatomic, assign) InvalidationType invalidationType;

@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* removals;
@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* insertions;
@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* moveFrom;
@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* moveTo;
@property(nonatomic, nullable, readonly)
    NSDictionary<NSIndexPath*, LynxCollectionViewLayoutModel*>* updates;
@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* fullSpanItems;
@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* stickyTopItems;
@property(nonatomic, nullable, readonly) NSArray<NSIndexPath*>* stickyBottomItems;

@property(nonatomic, readonly) CGRect bounds;

@property(nonatomic, readonly) BOOL hasInsetUpdates;
@property(nonatomic, readonly) UIEdgeInsets insets;

@property(nonatomic, readonly) NSInteger numberOfItems;
@property(nonatomic, readonly) NSInteger numberOfColumns;
@property(nonatomic, readonly) CGFloat mainAxisGap;
@property(nonatomic, readonly) CGFloat crossAxisGap;
@property(nonatomic, readonly) LynxCollectionViewLayoutType layoutType;

@property(nonatomic, readonly, getter=isAnimated) BOOL animated;

#pragma mark - Self-sizing Cell ContentOffset Adjustment for `initial-scroll-index`
@property(nonatomic, readonly) BOOL didSetInitialScrollIndex;
@property(nonatomic, readonly, getter=isSelfSizing) BOOL selfSizing;
@property(nonatomic) CGPoint currentContentOffset;
@property(nonatomic) NSIndexPath* indexPathContainsContentOffset;
@property(nonatomic, readonly) NSDictionary<NSIndexPath*, NSNumber*>* estimatedHeights;

@end

NS_ASSUME_NONNULL_END
