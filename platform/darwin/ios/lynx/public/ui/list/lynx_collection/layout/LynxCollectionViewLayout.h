// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>
NS_ASSUME_NONNULL_BEGIN
@class LynxCollectionScroll;
@interface LynxCollectionViewLayout : UICollectionViewLayout
@property(nonatomic, weak) LynxCollectionScroll *scroll;
@property(nonatomic) BOOL indexAsZIndex;

@property(nonatomic, strong, nullable) NSIndexPath *targetIndexPathAfterBatchUpdate;
@property(nonatomic, assign) CGFloat targetOffsetDeltaAfterBatchUpdate;
@property(nonatomic) BOOL needsAdjustContentOffsetForSelfSizingCells;
@property(nonatomic, assign) BOOL needUpdateValidLayoutAttributesAfterDiff;
@property(nonatomic, assign) BOOL enableAlignHeight;

- (void)setEnableSticky:(BOOL)enableSticky;
- (void)setHorizontalLayout:(BOOL)useHorizontalLayout;
- (void)setStickyOffset:(CGFloat)stickOffset;
- (void)prepareForCellLayoutUpdate;
- (void)setEnableAlignHeight:(BOOL)enableAlignHeight;
- (void)setFixSelfSizingOffsetFromStart:(BOOL)fromStart;
- (void)setUseOldSticky:(BOOL)useOldSticky;
- (void)setStickyWithBounces:(BOOL)stickyWithBounces;
- (BOOL)isStickyItem:(NSIndexPath *)indexPath;
- (UICollectionViewLayoutAttributes *)layoutAttributesForStickItemAtIndexPath:
    (NSIndexPath *)indexPath;
@end
NS_ASSUME_NONNULL_END
