// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

@class LynxListCachedCellManager;

typedef struct {
  /*
   True means the top cell has highest priority.
   False means the bottom cell has highest priority.
   */
  BOOL anchorPriorityFromBegin;
  /*
   Used when the highest priority anchor is deleted.
   True means the next-priority anchor should be searched in cells that has smaller origin.
   False means the next-priority anchor should be searched in cells that has bigger origin.
   */
  BOOL deleteRegressPolicyToTop;
  /*
   Used when new cells are about to insert at the edge of current bounds.
   True means the anchor cell should be a cell inside the bounds so the new cells will be inserted
   outside the bounds. False means the anchor cell should be a cell close but not inside the bounds
   so the new cells will be inserted inside the bounds and become visible immediately.
   */
  BOOL insertAnchorModeInside;
} LynxAnchorPolicies;

@interface LynxListAnchorManager : NSObject
@property(nonatomic, assign) BOOL isVerticalLayout;
@property(nonatomic, assign) NSInteger numberOfColumns;  // must be initialized
- (NSInteger)findAnchorCell:(LynxListCachedCellManager *)cachedCells
             anchorPolicies:(LynxAnchorPolicies)anchorPolicies
                 layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo;
- (NSInteger)findAnchorCellForRemoval:(LynxListCachedCellManager *)cachedCells
                       anchorPolicies:(LynxAnchorPolicies)anchorPolicies
                           layoutInfo:
                               (NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo
                        deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes;
- (NSInteger)closestAttributesToUpperVisibleBound:(NSInteger)index
                                         inColumn:(NSArray<NSNumber *> *)columnInfo;
- (NSInteger)closestAttributesToLowerVisibleBound:(NSInteger)index
                                         inColumn:(NSArray<NSNumber *> *)columnInfo;

@end
