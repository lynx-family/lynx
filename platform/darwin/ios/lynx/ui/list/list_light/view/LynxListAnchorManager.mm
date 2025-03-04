// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListAnchorManager.h"
#import "LynxListCachedCellManager.h"
#import "LynxListViewCellLight.h"

@implementation LynxListAnchorManager

- (instancetype)init {
  self = [super init];
  if (self) {
    self.numberOfColumns = 1;
    self.isVerticalLayout = YES;
  }
  return self;
}

#pragma mark insertion
- (NSInteger)findAnchorCell:(LynxListCachedCellManager *)cachedCells
             anchorPolicies:(LynxAnchorPolicies)anchorPolicies
                 layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo {
  if (anchorPolicies.anchorPriorityFromBegin) {
    return [self fromBegin:cachedCells anchorPolicies:anchorPolicies layoutInfo:layoutColumnInfo];
  } else {
    return [self fromEnd:cachedCells anchorPolicies:anchorPolicies layoutInfo:layoutColumnInfo];
  }
}

- (NSInteger)fromEnd:(LynxListCachedCellManager *)cachedCells
      anchorPolicies:(LynxAnchorPolicies)anchorPolicies
          layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo {
  if (anchorPolicies.insertAnchorModeInside) {
    id<LynxListCell> anchorCell = [self findBottomMostIndexInArray:cachedCells.displayingCells
                                                       skipRemoved:YES];
    return anchorCell ? anchorCell.updateToPath : -1;
  } else {
    id<LynxListCell> anchorCell = [self findTopMostIndexInArray:cachedCells.lowerCachedCells
                                                    skipRemoved:YES];
    if (nil != anchorCell) {
      return anchorCell.updateToPath;
    } else {
      return [self generateAnchorCellInBottomSide:cachedCells layoutInfo:layoutColumnInfo];
    }
  }
}

- (NSInteger)fromBegin:(LynxListCachedCellManager *)cachedCells
        anchorPolicies:(LynxAnchorPolicies)anchorPolicies
            layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo {
  if (anchorPolicies.insertAnchorModeInside) {
    id<LynxListCell> anchorCell = [self findTopMostIndexInArray:cachedCells.displayingCells
                                                    skipRemoved:YES];
    return anchorCell ? anchorCell.updateToPath : -1;
  } else {
    id<LynxListCell> anchorCell = [self findBottomMostIndexInArray:cachedCells.upperCachedCells
                                                       skipRemoved:YES];
    if (nil != anchorCell) {
      return anchorCell.updateToPath;
    } else {
      return [self generateAnchorCellInUpperSide:cachedCells layoutInfo:layoutColumnInfo];
    }
  }
}

#pragma mark removal

- (NSInteger)findAnchorCellForRemoval:(LynxListCachedCellManager *)cachedCells
                       anchorPolicies:(LynxAnchorPolicies)anchorPolicies
                           layoutInfo:
                               (NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo
                        deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes {
  if (anchorPolicies.anchorPriorityFromBegin) {
    id<LynxListCell> anchorCell = [self findTopMostIndexInArray:cachedCells.displayingCells
                                                    skipRemoved:YES];
    if (anchorCell) {
      return anchorCell.updateToPath;
    }
    return [self fromBeginForRemoval:cachedCells
                      anchorPolicies:anchorPolicies
                          layoutInfo:layoutColumnInfo
                       deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes];
  } else {
    id<LynxListCell> anchorCell = [self findBottomMostIndexInArray:cachedCells.displayingCells
                                                       skipRemoved:YES];
    if (anchorCell) {
      return anchorCell.updateToPath;
    }
    return [self fromEndForRemoval:cachedCells
                    anchorPolicies:anchorPolicies
                        layoutInfo:layoutColumnInfo
                     deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes];
  }
}

- (NSInteger)fromBeginForRemoval:(LynxListCachedCellManager *)cachedCells
                  anchorPolicies:(LynxAnchorPolicies)anchorPolicies
                      layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo
                   deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes {
  // step1: If the firstDisplayingCell is not removed, it's the anchor.
  id<LynxListCell> anchorCell = [self findTopMostIndexInArray:cachedCells.displayingCells
                                                  skipRemoved:NO];
  if (nil != anchorCell && !anchorCell.removed) {
    return anchorCell.updateToPath;
  } else {
    // step2: Since the NO.1 anchor removed, use regress policy to find the NO.2 anchor.
    if (anchorPolicies.deleteRegressPolicyToTop) {
      anchorCell = [self findBottomMostIndexInArray:cachedCells.upperCachedCells skipRemoved:YES];
      if (nil != anchorCell) {
        return anchorCell.updateToPath;
      } else {
        return [self generateAnchorCellInUpperSide:cachedCells
                                        layoutInfo:layoutColumnInfo
                                     deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes];
      }
    } else {
      anchorCell = [self findTopMostIndexInArray:cachedCells.displayingCells skipRemoved:YES];
      if (nil != anchorCell) {
        return anchorCell.updateToPath;
      } else {
        anchorCell = [self findTopMostIndexInArray:cachedCells.lowerCachedCells skipRemoved:YES];
        if (nil != anchorCell) {
          return anchorCell.updateToPath;
        } else {
          return [self generateAnchorCellInBottomSide:cachedCells layoutInfo:layoutColumnInfo];
        }
      }
    }
  }
}

#pragma mark generate new cell as anchor if it doesn't exist
// If the previous finding can't find approriate anchor cell, then generate a new one.
// The new cell should be non-removed and has a smaller updateToPath.
- (NSInteger)generateAnchorCellInUpperSide:(LynxListCachedCellManager *)cachedCells
                                layoutInfo:
                                    (NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo
                             deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes {
  // If upperCache is empty, generate a new top cell to be the anchor.
  NSInteger firstVisibleIndex = cachedCells.firstIndexInPathOrder;
  NSMutableArray<NSNumber *> *searchArray = [NSMutableArray arrayWithCapacity:firstVisibleIndex];
  for (int i = 0; i < firstVisibleIndex; i++) {
    [searchArray addObject:@(NO)];
  }
  [deleteIndexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (obj.integerValue < firstVisibleIndex) {
          searchArray[obj.integerValue] = @(YES);
        }
      }];
  __block NSInteger anchorIndex = -1;
  [searchArray
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
                         if (!obj) {
                           anchorIndex = idx;
                         }
                       }];
  return anchorIndex;
}

- (NSInteger)generateAnchorCellInUpperSide:(LynxListCachedCellManager *)cachedCells
                                layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)
                                               layoutColumnInfo {
  NSInteger anchorIndex = cachedCells.firstIndexInPathOrder;
  id<LynxListCell> anchorCell = [cachedCells cellAtIndex:anchorIndex];
  return [self closestAttributesToUpperVisibleBound:anchorIndex
                                           inColumn:layoutColumnInfo[anchorCell.columnIndex]];
}

- (NSInteger)generateAnchorCellInBottomSide:(LynxListCachedCellManager *)cachedCells
                                 layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)
                                                layoutColumnInfo {
  NSInteger anchorIndex = cachedCells.lastIndexInPathOrder;
  id<LynxListCell> anchorCell = [cachedCells cellAtIndex:anchorIndex];
  return [self closestAttributesToLowerVisibleBound:anchorIndex
                                           inColumn:layoutColumnInfo[anchorCell.columnIndex]];
}

- (NSInteger)fromEndForRemoval:(LynxListCachedCellManager *)cachedCells
                anchorPolicies:(LynxAnchorPolicies)anchorPolicies
                    layoutInfo:(NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo
                 deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes {
  // step1: If the firstDisplayingCell is not removed, it's the anchor.
  id<LynxListCell> anchorCell = [self findBottomMostIndexInArray:cachedCells.displayingCells
                                                     skipRemoved:NO];
  if (nil != anchorCell && !anchorCell.removed) {
    return anchorCell.updateToPath;
  } else {
    // step2: Since the NO.1 anchor removed, use regress policy to find the NO.2 anchor.
    if (anchorPolicies.deleteRegressPolicyToTop) {
      anchorCell = [self findBottomMostIndexInArray:cachedCells.displayingCells skipRemoved:YES];
      if (nil != anchorCell) {
        return anchorCell.updateToPath;
      } else {
        anchorCell = [self findBottomMostIndexInArray:cachedCells.upperCachedCells skipRemoved:YES];
        if (nil != anchorCell) {
          return anchorCell.updateToPath;
        } else {
          return [self generateAnchorCellInUpperSide:cachedCells
                                          layoutInfo:layoutColumnInfo
                                       deleteIndexes:(NSArray<NSNumber *> *)deleteIndexes];
        }
      }
    } else {
      anchorCell = [self findTopMostIndexInArray:cachedCells.lowerCachedCells skipRemoved:YES];
      if (nil != anchorCell) {
        return anchorCell.updateToPath;
      } else {
        return [self generateAnchorCellInBottomSide:cachedCells layoutInfo:layoutColumnInfo];
      }
    }
  }
}

#pragma mark helper
/*
 1. The lower the bottom, the higher its priority.
 2. the bigger the index, the higher its priority.
 3. Ignore any cell with removed mark.
 */
- (nullable id<LynxListCell>)findBottomMostIndexInArray:(NSArray<id<LynxListCell> > *)array
                                            skipRemoved:(BOOL)skipRemoved {
  __block CGFloat distance = -1;
  __block id<LynxListCell> anchorCell;
  [array enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                      BOOL *_Nonnull stop) {
    if (!skipRemoved || !obj.removed) {
      if ([self orientationBottomOfCell:obj] > distance) {
        distance = [self orientationBottomOfCell:obj];
        anchorCell = obj;
      } else if ([self orientationBottomOfCell:obj] == distance &&
                 obj.updateToPath > anchorCell.updateToPath) {
        anchorCell = obj;
      }
    }
  }];
  return anchorCell;
}

/*
 1. The smaller the top, the higher its priority.
 2. The smaller the index, the higher its priority.
 3. Ignore any cell with removed mark.
*/
- (nullable id<LynxListCell>)findTopMostIndexInArray:(NSArray<id<LynxListCell> > *)array
                                         skipRemoved:(BOOL)skipRemoved {
  __block CGFloat distance = CGFLOAT_MAX;
  __block id<LynxListCell> anchorCell;
  [array enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                      BOOL *_Nonnull stop) {
    if (!skipRemoved || !obj.removed) {
      if ([self orientationTopOrigin:obj.frame] < distance) {
        distance = [self orientationTopOrigin:obj.frame];
        anchorCell = obj;
      } else if ([self orientationTopOrigin:obj.frame] == distance &&
                 obj.updateToPath < anchorCell.updateToPath) {
        anchorCell = obj;
      }
    }
  }];
  return anchorCell;
}

- (NSInteger)closestAttributesToUpperVisibleBound:(NSInteger)index
                                         inColumn:(NSArray<NSNumber *> *)columnInfo {
  if (self.numberOfColumns == 1) {
    return MAX(index - 1, 0);
  }
  NSInteger indexInColumn = [self indexOf:@(index)
                                 inColumn:columnInfo
                                      low:0
                                     high:columnInfo.count - 1];
  if (indexInColumn <= 0) {
    return -1;
  } else {
    return columnInfo[indexInColumn - 1].integerValue;
  }
}

- (NSInteger)closestAttributesToLowerVisibleBound:(NSInteger)index
                                         inColumn:(NSArray<NSNumber *> *)columnInfo {
  if (self.numberOfColumns == 1) {
    return index + 1;
  }
  // Maybe in other column.
  if (index >= columnInfo.lastObject.integerValue) {
    return index + 1;
  }
  NSInteger indexInColumn = [self indexOf:@(index)
                                 inColumn:columnInfo
                                      low:0
                                     high:columnInfo.count - 1];
  if (indexInColumn < 0 || indexInColumn >= (NSInteger)columnInfo.count - 1) {
    return -1;
  } else {
    return columnInfo[indexInColumn + 1].integerValue;
  }
}

/**
 @param index target to find in columnInfo
 @param columnInfo increasing, non-repeat array
 @param low lower range of current binary search
 @param high higher range of current binary search
 @return the sequence number of 'index' in 'columnInfo'. return -1 if index not found in columnInfo
 */
- (NSInteger)indexOf:(NSNumber *)index
            inColumn:(NSArray<NSNumber *> *)columnInfo
                 low:(NSInteger)low
                high:(NSInteger)high {
  if (columnInfo.count == 0 || low < 0 || high < 0 || low > high ||
      low > (NSInteger)columnInfo.count - 1 || high > (NSInteger)columnInfo.count - 1) {
    return -1;
  }
  // If columnInfo has the same values, the smallest one will be returned
  NSInteger middle = (low + high) >> 1;
  if (columnInfo[middle].integerValue == index.integerValue) {
    return middle;
  } else if (columnInfo[middle].integerValue > index.integerValue) {
    return [self indexOf:index inColumn:columnInfo low:low high:middle - 1];
  } else {
    return [self indexOf:index inColumn:columnInfo low:middle + 1 high:high];
  }
  return -1;
}

- (CGFloat)orientationBottomOfCell:(id<LynxListCell>)cell {
  return self.isVerticalLayout ? CGRectGetMaxY(cell.frame) : CGRectGetMaxX(cell.frame);
}

- (CGFloat)orientationTopOrigin:(CGRect)rect {
  return self.isVerticalLayout ? rect.origin.y : rect.origin.x;
}

@end
