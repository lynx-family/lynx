// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxUIListProtocol.h>
/**
 Base class for internal layoutManagers. Provides basic helper methods for waterfall and fullspan
 layout. Can be inherits to implement customized layout.
 */

NS_ASSUME_NONNULL_BEGIN
@interface LynxListLayoutManager : NSObject <LynxListLayoutProtocol>
@property(nonatomic, strong) NSMutableArray<LynxListLayoutModelLight *>
    *models;  // Stores every layoutModels, both with and without valid layout.
@property(nonatomic, assign)
    NSInteger firstInvalidIndex;  // Smallest updated index from diff update.

#pragma mark Layout Info
@property(nonatomic, assign)
    NSUInteger numberOfColumns;  // numberOfColumns must be initialized to a non-zero value.
@property(nonatomic, assign) CGFloat mainAxisGap;
@property(nonatomic, assign) CGFloat crossAxisGap;
@property(nonatomic, assign) UIEdgeInsets insets;  // Padding.
@property(nonatomic, assign) CGRect bounds;        // Current visible area of view.
@property(nonatomic, strong) NSArray<NSNumber *> *fullSpanItems;
@property(nonatomic, assign) LynxListLayoutType layoutType;  // Waterfall | Flow
@property(nonatomic, assign) BOOL needAlignHeight;           // Used to round model frame size.
@property(nonatomic, assign)
    NSInteger lastValidModel;  // The last model with valid layout in lazy layout.

#pragma mark helpers
/**
 Stores current column height.
 */
@property(nonatomic, strong) NSMutableArray<NSNumber *> *mainSizes;
@property(nonatomic, strong) NSMutableArray<NSArray<NSNumber *> *> *mainSizesCache;

/**
 Sort the models in each column based on their index order.
 Note that due to lazy layout, only the models with valid layouts are stored.
 */
@property(nonatomic, strong) NSMutableArray<NSMutableArray<NSNumber *> *> *layoutColumnInfo;

/**
 Default heights to initialize layout models.
 */
@property(nonatomic, strong, nullable) NSDictionary<NSNumber *, NSNumber *> *estimatedHeights;
- (void)retrieveMainSizeFromCacheAtInvalidIndex:(NSInteger)invalidIndex;
- (CGFloat)largestSizeInMainSizes:(NSArray<NSNumber *> *)mainSizes;
- (NSUInteger)findNearestFullSpanItem:(NSUInteger)index;
- (CGFloat)largestMainSizeInPreviousRowAtIndex:(NSUInteger)index
                      withNearestFullSpanIndex:(NSUInteger)nearestFullSpanIndex;
- (void)resetMainSizesWithNumberOfColumns:(NSUInteger)numberOfColumns;
- (CGFloat)layoutOffsetForFullSpanItems:(CGFloat)itemSize
                              crossSize:(CGFloat)collectionSize
                           paddingStart:(CGFloat)paddingStart
                             paddingEnd:(CGFloat)paddingEnd;
- (CGFloat)largestMainSize;
- (CGFloat)adjustOffsetAtIndex:(NSUInteger)index
                originalOffset:(CGFloat)Offset
               nearestFullSpan:(NSUInteger)nearestFullSpanIndex;
- (NSUInteger)shortestColumn;
- (CGFloat)shortestMainSize;
@end
NS_ASSUME_NONNULL_END
