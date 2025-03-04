// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutModelLight.h>
#import <Lynx/LynxUIListInvalidationContext.h>

@protocol LynxListLayoutProtocol <NSObject>
/*
 The index of the first item with invalid layout after every update.
 */
@property(nonatomic, assign) NSInteger firstInvalidIndex;

/**
 Sort the models in each column based on their index order.
 Note that due to lazy layout, only the models with valid layouts are stored.
 */
@property(nonatomic, strong) NSMutableArray<NSMutableArray<NSNumber *> *> *layoutColumnInfo;

@property(nonatomic, assign)
    NSInteger lastValidModel;  // The last model with valid layout in lazy layout.

@property(nonatomic, assign) NSUInteger numberOfColumns;
@property(nonatomic, assign) CGFloat mainAxisGap;
@property(nonatomic, assign) CGFloat crossAxisGap;
@property(nonatomic, assign) UIEdgeInsets insets;  // padding

/**
  @return The number of layoutModels.
*/
- (NSInteger)getCount;

/**
  @return Current view's contentSize. The size should based on layout result.
*/
- (CGSize)getContentSize;

/**
 Determine the layout orientation.
 @return True - The list layout is vertical.
 @return False - The list layout is horizontal.
*/
- (BOOL)isVerticalLayout;

/**
 Removes models at positions.
 @param removals - indexes of models need to be removed.
 */
- (void)updateModelsWithRemovals:(NSArray<NSNumber *> *)removals;

/**
 Insert new models at indexes.
 @param insertions - Indexes of models need to be inserted. All models will be initialized with
 default size.
 */
- (void)updateModelsWithInsertions:(NSArray<NSNumber *> *)insertions;

/**
 Update the models at certain indexes to new sizes.
 @param updates - <indexes of updated models, new CGRect>
 */
- (void)updateModels:(NSDictionary<NSNumber *, NSValue *> *)updates;

/**
 Find layout models that contain the current offset, meaning they should be displayed at this
 position. One for each column.
 @return column index -> layout model index
 */
- (NSDictionary<NSNumber *, NSNumber *> *)findWhichItemToDisplayOnTop;

/**
 Called when diff / scroll / layoutInfo changes. If no layoutInfo change, just change the bounds
 info.
 @param bounds - view's bounds
 @param context - can be multiple types
 */
- (void)updateBasicInvalidationContext:(LynxUIListInvalidationContext *)context
                                bounds:(CGRect)bounds;

/**
 Since the layout is lazy, the layoutManager should only update the layout model in the given area.
 However, to ensure consistency, it must also check the area of [0, startIndex] and make it valid if
 it's not.
 @param startIndex - smallest index of lazy layout area.
 @param endIndex - biggest index of lazy layout area.
*/
- (void)layoutFrom:(NSInteger)startIndex to:(NSInteger)endIndex;

/**
 @return The layoutModel at index.
*/
- (LynxListLayoutModelLight *)attributesFromIndex:(NSInteger)index;

/**
 Detect whether the cell at index intersects current visible area.
 @param index - cell index.
 @return return YES if this cell intersects current visible area.
*/
- (BOOL)layoutModelVisibleInIndex:(NSInteger)index;
@end

@protocol LynxListCell <NSObject>
@property(nonatomic, assign) NSInteger updateToPath;  // Position of this cell.
@property(nonatomic, strong) NSString *itemKey;       // UniqueID for diff
@property(nonatomic, assign) BOOL
    removed;  // Mark removed before it really gets recycled. This is used to adjust the anchor.
@property(nonatomic, assign) NSInteger columnIndex;  // Mark which column it currently locates.
@property(nonatomic, assign) LynxLayoutModelType layoutType;  // Mark if its a fullspan item.
@property(nonatomic, strong) NSString *reuseIdentifier;
@property(nonatomic, assign) CGRect frame;
@property(nonatomic, assign)
    int64_t operationID;  // An ID to mark the asynchronously returned content.
/*
 If the cell's original position is totally out of view's bounds, then it's in sticky status.
 It means the cell should never been considered in fill algorithm.
 */
@property(nonatomic, assign) BOOL isInStickyStatus;

/*
 To support multi-sticky, each sticky cell should has a stickyPosition as their sticky offset. We
 don't use offset here to avoid misunderstanding.
 */
@property(nonatomic, assign) CGFloat stickyPosition;
/*
 We need an extra layer to wrap the content better so we can do animations and apply complex
 background and borders.
 */
@property(nonatomic, strong) UIView *contentView;
/**
 Use layoutModel to update cell's layout.
 @param model - layoutModel for the cell at its position.
 */
- (void)applyLayoutModel:(LynxListLayoutModelLight *)model;
@end

@protocol LynxListCellContentProducer <NSObject>
/**
 Generates a filled cell for the given key.
 @param cell - A reusable cell managed by reusePool in listView.
 @param itemKey - UniqueID to identify data to generate correct cell content.
 @return cell - A filled and ready-to-show cell conforms to the LynxListCell protocol.
 */
- (id<LynxListCell>)cell:(id<LynxListCell>)cell forKey:(NSString *)itemKey;
@end

@protocol LynxListEventsProtocol <NSObject>
/**
 @return Numbers of all cells, including both cached or uncached cells.
 */
- (NSInteger)totalItemsCount;

/**
 @return All cells on screen. Not including cells in preload buffer.
 */
- (NSArray<id<LynxListCell>> *)attachedCells;

@end
