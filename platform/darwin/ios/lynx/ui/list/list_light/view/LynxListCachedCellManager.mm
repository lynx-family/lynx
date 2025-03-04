// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListCachedCellManager.h"
#import "LynxError.h"
#import "LynxListViewCellLight.h"
#import "LynxLog.h"
#import "LynxSubErrorCode.h"
#import "LynxUIContext.h"
#import "LynxView+Internal.h"

@interface LynxListCachedCellManager ()
@property(nonatomic, assign) NSInteger firstIndexInPathOrder;
@property(nonatomic, assign) NSInteger lastIndexInPathOrder;
@property(nonatomic, assign) BOOL markPathOrderDirty;
@property(nonatomic, assign) BOOL markVisibleCellDirty;
@property(nonatomic, weak) LynxUIContext *uiContext;
@end

@implementation LynxListCachedCellManager
#pragma mark init
- (instancetype)initWithColumnCount:(NSInteger)numberOfColumns uiContext:(LynxUIContext *)context {
  self = [super init];
  if (self) {
    self.displayingCells = [NSMutableArray array];
    self.lowerCachedCells = [NSMutableArray array];
    self.upperCachedCells = [NSMutableArray array];
    self.uiContext = context;
    // The numberOfColumns has to be at least 1
    self.numberOfColumns = MAX(numberOfColumns, 1);
    self.firstIndexInPathOrder = -1;
    self.lastIndexInPathOrder = -1;
  }
  return self;
}

#pragma mark update
- (void)addCell:(id<LynxListCell>)cell inArray:(NSMutableArray<id<LynxListCell>> *)cacheArray {
  if (!cell) {
    return;
  }
  if ([self cellAtIndex:cell.updateToPath]) {
    LynxError *duplicatedCellError =
        [LynxError lynxErrorWithCode:ECLynxComponentListDuplicatedCell
                             message:@"cell already exists!"
                       fixSuggestion:@"This error is caught by native, please ask Lynx for help."
                               level:LynxErrorLevelError
                          customInfo:@{@"lynx_context_cell_index" : @(cell.updateToPath)}];
    [_uiContext reportError:duplicatedCellError];
    return;
  }
  self.markPathOrderDirty = YES;
  self.markVisibleCellDirty = YES;
  [cacheArray addObject:cell];
  [self updateVisibleCells];
}

- (BOOL)markRemoveCellAtIndex:(NSInteger)index {
  id<LynxListCell> cell = [self cellAtIndex:index];
  if (nil != cell) {
    cell.removed = YES;
    return YES;
  }
  return NO;
}

- (void)markCellInfoDirty {
  self.markPathOrderDirty = YES;
  self.markVisibleCellDirty = YES;
}

- (void)updateIndexInPathOrder {
  if ([self isEmpty]) {
    self.firstIndexInPathOrder = -1;
    _lastIndexInPathOrder = -1;
    return;
  }
  NSSortDescriptor *sortOrder = [NSSortDescriptor sortDescriptorWithKey:@"updateToPath"
                                                              ascending:YES];
  NSArray<id<LynxListCell>> *sortedCachedCellsInPathOrder =
      [[self allCachedCells] sortedArrayUsingDescriptors:[NSArray arrayWithObject:sortOrder]];
  self.firstIndexInPathOrder = sortedCachedCellsInPathOrder.firstObject.updateToPath;
  self.lastIndexInPathOrder = sortedCachedCellsInPathOrder.lastObject.updateToPath;
}

- (void)updateVisibleCells {
  if ([self isEmpty]) {
    self.firstVisibleCell = nil;
    self.lastVisibleCell = nil;
    return;
  }
  NSSortDescriptor *sortOrder = [NSSortDescriptor sortDescriptorWithKey:@"updateToPath"
                                                              ascending:YES];
  NSArray<id<LynxListCell>> *sortedDisplayingCellsInPathOrder =
      [[self displayingCells] sortedArrayUsingDescriptors:[NSArray arrayWithObject:sortOrder]];
  [sortedDisplayingCellsInPathOrder
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        if (!obj.isInStickyStatus) {
          self.firstVisibleCell = obj;
          *stop = YES;
        }
      }];
  [sortedDisplayingCellsInPathOrder
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                    BOOL *_Nonnull stop) {
                         if (!obj.isInStickyStatus) {
                           self.lastVisibleCell = obj;
                           *stop = YES;
                         }
                       }];
}

- (id<LynxListCell>)removeCellAtIndex:(NSInteger)index {
  self.markPathOrderDirty = YES;
  self.markVisibleCellDirty = YES;
  id<LynxListCell> cell = [self findCellAtIndex:index inArray:self.displayingCells];
  if (nil != cell) {
    [self.displayingCells removeObject:cell];
  } else {
    cell = [self findCellAtIndex:index inArray:self.lowerCachedCells];
    if (nil != cell) {
      [self.lowerCachedCells removeObject:cell];
    } else {
      cell = [self findCellAtIndex:index inArray:self.upperCachedCells];
      if (nil != cell) {
        [self.upperCachedCells removeObject:cell];
      }
    }
  }
  return cell;
}

#pragma mark find
- (id<LynxListCell> _Nullable)findCellAtIndex:(NSInteger)index
                                      inArray:(NSMutableArray<id<LynxListCell>> *)cacheArray {
  __block id<LynxListCell> cell;
  [cacheArray enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                           BOOL *_Nonnull stop) {
    if (obj.updateToPath == index) {
      cell = obj;
      *stop = YES;
    }
  }];
  return cell;
}

- (id<LynxListCell> _Nullable)cellAtIndex:(NSInteger)index {
  // Find cell in displayingCells first, then go to search caches.
  // Different caches cannot have the same cell, otherwise the ECLynxComponentListDuplicatedCell
  // will occur.
  id<LynxListCell> cell = [self findCellAtIndex:index inArray:self.displayingCells];
  if (cell) {
    return cell;
  }
  cell = [self findCellAtIndex:index inArray:self.lowerCachedCells];
  if (cell) {
    return cell;
  }
  cell = [self findCellAtIndex:index inArray:self.upperCachedCells];
  if (cell) {
    return cell;
  }

  return nil;
}

#pragma mark cell infomation

- (id<LynxListCell>)firstVisibleCell {
  if (self.markVisibleCellDirty) {
    [self updateVisibleCells];
    self.markVisibleCellDirty = NO;
  }
  return _firstVisibleCell;
}

- (id<LynxListCell>)lastVisibleCell {
  if (self.markVisibleCellDirty) {
    [self updateVisibleCells];
    self.markVisibleCellDirty = NO;
  }
  return _lastVisibleCell;
}

- (NSInteger)lastIndexInPathOrder {
  if (self.markPathOrderDirty) {
    [self updateIndexInPathOrder];
    self.markPathOrderDirty = NO;
  }
  return _lastIndexInPathOrder;
}

- (NSInteger)firstIndexInPathOrder {
  if (self.markPathOrderDirty) {
    [self updateIndexInPathOrder];
    self.markPathOrderDirty = NO;
  }
  return _firstIndexInPathOrder;
}

- (NSMutableDictionary<NSNumber *, id<LynxListCell>> *)topCells {
  if (_numberOfColumns == 1) {
    id<LynxListCell> cell = self.firstVisibleCell;
    if (!cell) {
      LynxError *topCellNotFoundError =
          [LynxError lynxErrorWithCode:ECLynxComponentListCellNotFound
                               message:@"topCell not found in cache."
                         fixSuggestion:@"This error is caught by native, please ask Lynx for help."
                                 level:LynxErrorLevelError
                            customInfo:@{@"lynx_context_description" : self.description}];
      [_uiContext reportError:topCellNotFoundError];
      return nil;
    }
    return @{@(0) : cell}.mutableCopy;
  }
  NSMutableDictionary<NSNumber *, id<LynxListCell>> *topCells = [NSMutableDictionary dictionary];
  __block NSInteger minimumIndex = NSIntegerMax;
  [self.displayingCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
    if (obj.layoutType == LynxLayoutModelFullSpan) {
      if (obj.updateToPath < minimumIndex && !obj.isInStickyStatus) {
        for (int i = 0; i < self.numberOfColumns; i++) {
          topCells[@(i)] = obj;
        }
        minimumIndex = obj.updateToPath;
      }
    }
  }];
  [self.displayingCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
    id<LynxListCell> cell = topCells[@(obj.columnIndex)];
    if (obj.layoutType != LynxLayoutModelFullSpan) {
      if ((!cell || obj.updateToPath < cell.updateToPath) && !obj.isInStickyStatus) {
        topCells[@(obj.columnIndex)] = obj;
      }
    }
  }];
  return topCells.copy;
}

- (NSMutableDictionary<NSNumber *, id<LynxListCell>> *)bottomCells {
  if (self.numberOfColumns == 1) {
    id<LynxListCell> cell = self.lastVisibleCell;
    if (!cell) {
      LynxError *bottomCellNotFoundError =
          [LynxError lynxErrorWithCode:ECLynxComponentListCellNotFound
                               message:@"BottomCell not found in cache"
                         fixSuggestion:@"This error is caught by native, please ask Lynx for help."
                                 level:LynxErrorLevelError
                            customInfo:@{@"lynx_context_description" : self.description}];
      [_uiContext reportError:bottomCellNotFoundError];
      return nil;
    }
    return @{@(0) : cell}.mutableCopy;
  }
  NSMutableDictionary<NSNumber *, id<LynxListCell>> *bottomCells = [NSMutableDictionary dictionary];
  __block NSInteger maxIndex = -1;
  [self.displayingCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
    if (obj.layoutType == LynxLayoutModelFullSpan) {
      if (obj.updateToPath > maxIndex && !obj.isInStickyStatus) {
        for (int i = 0; i < self.numberOfColumns; i++) {
          bottomCells[@(i)] = obj;
        }
        maxIndex = obj.updateToPath;
      }
    }
  }];
  [self.displayingCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                     BOOL *_Nonnull stop) {
    if (obj.layoutType != LynxLayoutModelFullSpan) {
      id<LynxListCell> cell = bottomCells[@(obj.columnIndex)];
      if (!cell || obj.updateToPath > cell.updateToPath) {
        bottomCells[@(obj.columnIndex)] = obj;
      }
    }
  }];
  return bottomCells;
}

- (NSArray<id<LynxListCell>> *)allCachedCells {
  NSMutableArray<id<LynxListCell>> *allCachedCells =
      [NSMutableArray arrayWithArray:self.upperCachedCells];
  [allCachedCells addObjectsFromArray:self.displayingCells];
  [allCachedCells addObjectsFromArray:self.lowerCachedCells];
  return [allCachedCells copy];
}

#pragma mark helper

- (CGFloat)orientationBottomOfCell:(id<LynxListCell>)cell {
  return self.isVerticalLayout ? cell.frame.size.height + cell.frame.origin.y
                               : cell.frame.size.width + cell.frame.origin.x;
}

- (BOOL)isEmpty {
  return self.displayingCells.count == 0 && self.upperCachedCells.count == 0 &&
         self.lowerCachedCells.count == 0;
}

- (NSString *)description {
  __block NSMutableString *des =
      [[NSMutableString alloc] initWithString:@"[list] displayingCells: upper: "];
  [_upperCachedCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                  BOOL *_Nonnull stop) {
    [des appendString:[NSString stringWithFormat:@"%ld '%@' (%@) ", (long)obj.updateToPath,
                                                 obj.itemKey, @(obj.frame)]];
  }];

  [des appendString:@"\tdisplaying: "];
  [_displayingCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                 BOOL *_Nonnull stop) {
    [des appendString:[NSString stringWithFormat:@"%ld '%@' (%@) ", (long)obj.updateToPath,
                                                 obj.itemKey, @(obj.frame)]];
  }];

  [des appendString:@"\tlower: "];
  [_lowerCachedCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                                  BOOL *_Nonnull stop) {
    [des appendString:[NSString stringWithFormat:@"%ld '%@' (%@) ", (long)obj.updateToPath,
                                                 obj.itemKey, @(obj.frame)]];
  }];

  return des;
}

@end
