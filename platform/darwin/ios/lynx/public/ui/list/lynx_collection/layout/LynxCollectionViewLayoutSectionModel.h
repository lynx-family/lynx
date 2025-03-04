// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN
@class LynxCollectionInvalidationContext;
@interface LynxCollectionViewLayoutSectionModel : NSObject
- (instancetype)initWithItemCount:(NSUInteger)length;
- (void)updateWithInvalidationContext:(LynxCollectionInvalidationContext *)context
                       collectionView:(UICollectionView *)collectionView;
- (void)layoutIfNeededForUICollectionView:(UICollectionView *)collectionView;
- (NSArray<__kindof UICollectionViewLayoutAttributes *> *)layoutAttributesForElementsInRect:
    (CGRect)rect;

- (__kindof UICollectionViewLayoutAttributes *)layoutAttributeForElementAtIndexPath:
    (NSIndexPath *)indexPath;
- (CGSize)contentSize;

// for update-animation
- (void)prepareForCollectionViewUpdates:(NSArray<UICollectionViewUpdateItem *> *)updateItems;
- (__kindof UICollectionViewLayoutAttributes *)
    initialLayoutAttributesForAppearingItemAtIndexPath:(NSIndexPath *)itemIndexPath
                               defaultLayoutAttributes:
                                   (UICollectionViewLayoutAttributes *)defaultAttributes
                                      inCollectionView:(UICollectionView *__nullable)collectionView;
- (__kindof UICollectionViewLayoutAttributes *)finalLayoutAttributesForDisappearingItemAtIndexPath:
    (NSIndexPath *)itemIndexPath;
- (void)finalizeCollectionViewUpdates;
- (void)setBounds:(CGRect)newBounds;

@property(nonatomic, assign) BOOL needAlignHeight;

@property(nonatomic, assign) BOOL useOldSticky;

@property(nonatomic, assign) BOOL horizontalLayout;

@property(nonatomic, assign) BOOL fixSelfSizingOffsetFromStart;

// for sticky items
@property(nonatomic) BOOL enableSticky;
@property(nonatomic) CGFloat stickyOffset;
@property(nonatomic) BOOL indexAsZIndex;
@property(nonatomic) BOOL stickyWithBounces;

// for `initial-scroll-index`
- (void)adjustCollectionViewContentOffsetForSelfSizingCellInvaldationIfNeeded:
    (UICollectionView *)collectionView;

// for animation while component size adjusted by `onComponentLayoutUpdated`
- (void)prepareForCellLayoutUpdate;

- (UICollectionViewLayoutAttributes *)layoutAttributesFromCacheAtRow:(NSInteger)row;
- (BOOL)isStickyItem:(NSInteger)item;
@end
NS_ASSUME_NONNULL_END
