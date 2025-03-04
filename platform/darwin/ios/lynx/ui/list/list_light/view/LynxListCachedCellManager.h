// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxUIListProtocol.h>
@class LynxUIContext;

NS_ASSUME_NONNULL_BEGIN

@interface LynxListCachedCellManager : NSObject
// displaying on the screen
@property(nonatomic, strong, nullable) NSMutableArray<id<LynxListCell>> *displayingCells;
// cached cells, including previous on screen cells and preloaded cells
@property(nonatomic, strong, nullable) NSMutableArray<id<LynxListCell>> *upperCachedCells;
@property(nonatomic, strong, nullable) NSMutableArray<id<LynxListCell>> *lowerCachedCells;
@property(nonatomic, strong, readonly) NSArray<id<LynxListCell>> *allCachedCells;
@property(nonatomic, strong, nullable) id<LynxListCell> lastVisibleCell;
@property(nonatomic, strong, nullable) id<LynxListCell> firstVisibleCell;
@property(nonatomic, assign) NSInteger numberOfColumns;
@property(nonatomic, assign) BOOL isVerticalLayout;

/**
 Init method
 @param numberOfColumns - at least 1
 @param context - used to send LynxError
 */
- (instancetype)initWithColumnCount:(NSInteger)numberOfColumns uiContext:(LynxUIContext *)context;
/**
 Add new cell to cache
 @param cell - cell that need to be added
 @param cacheArray - displayingCells / upperCache / lowerCache
 */
- (void)addCell:(id<LynxListCell>)cell inArray:(NSMutableArray<id<LynxListCell>> *)cacheArray;
/**
 Find cell in certain cache
 @param index - the index of cell  that need to be added
 */
- (id<LynxListCell> _Nullable)findCellAtIndex:(NSInteger)index
                                      inArray:(NSMutableArray<id<LynxListCell>> *)cacheArray;
/**
 @param index - the index of cell  that need to be removed
 @return return true if the cell with this index exists
 */
- (BOOL)markRemoveCellAtIndex:(NSInteger)index;

/**
 Mark first/last index and first/last visible cell dirty and need to be updated
 */
- (void)markCellInfoDirty;

/**
 @param index - remove cell with index
 @return cell that has been removed
 */
- (id<LynxListCell>)removeCellAtIndex:(NSInteger)index;
/**
 @param index - index that needs to find in ALL caches
 @return found cell
 */
- (id<LynxListCell> _Nullable)cellAtIndex:(NSInteger)index;
/**
 @return array with cells that is now at top of each column.
 */
- (NSMutableDictionary<NSNumber *, id<LynxListCell>> *)topCells;
/**
 @return array with cells that is now at bottom of each column.
 */
- (NSMutableDictionary<NSNumber *, id<LynxListCell>> *)bottomCells;
/**
 @return the largest updateToPath in all caches.
 */
- (NSInteger)lastIndexInPathOrder;
/**
 @return the smallest updateToPath in all caches.
 */
- (NSInteger)firstIndexInPathOrder;
- (BOOL)isEmpty;

@end

NS_ASSUME_NONNULL_END
