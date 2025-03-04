// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxUIListInvalidationContext.h>

NSInteger const kLynxListViewLayoutInvalidIndex = -1;
@implementation LynxListLayoutManager

- (instancetype)init {
  self = [super init];
  if (self) {
    self.models = [NSMutableArray array];
    self.numberOfColumns = 1;
    self.numberOfColumns = 1;
    self.mainAxisGap = 0;
    self.crossAxisGap = 0;
    self.layoutType = LynxListLayoutFlow;
  }
  return self;
}

- (NSMutableArray<NSArray<NSNumber *> *> *)mainSizesCache {
  if (!_mainSizesCache) {
    _mainSizesCache = [NSMutableArray array];
  }
  return _mainSizesCache;
}

- (NSMutableArray<NSMutableArray<NSNumber *> *> *)layoutColumnInfo {
  if (!_layoutColumnInfo) {
    _layoutColumnInfo = [NSMutableArray array];
  }
  return _layoutColumnInfo;
}

- (BOOL)isVerticalLayout {
  // override by subclasses
  return YES;
}

#pragma mark update methods
- (void)updateBasicInvalidationContext:(LynxUIListInvalidationContext *)context
                                bounds:(CGRect)bounds {
  self.bounds = bounds;
  self.layoutType =
      (LynxListLayoutNone != context.layoutType) ? context.layoutType : self.layoutType;
  self.numberOfColumns =
      (0 != context.numberOfColumns) ? context.numberOfColumns : self.numberOfColumns;
  self.mainAxisGap = (0 != context.mainAxisGap) ? context.mainAxisGap : self.mainAxisGap;
  self.crossAxisGap = (0 != context.crossAxisGap) ? context.crossAxisGap : self.crossAxisGap;
  self.fullSpanItems = context.fullSpanItems ? context.fullSpanItems.copy : self.fullSpanItems;
  self.estimatedHeights =
      context.estimatedHeights.count > 0 ? context.estimatedHeights : self.estimatedHeights;
}

- (void)updateModelsWithInsertions:(NSArray<NSNumber *> *)insertions {
  [insertions
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger index = obj.integerValue;
        LynxListLayoutModelLight *model;
        if (self.estimatedHeights[obj]) {
          model = [[LynxListLayoutModelLight alloc]
              initWithFrame:CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width,
                                       self.estimatedHeights[obj].floatValue)];
        } else {
          model = [[LynxListLayoutModelLight alloc]
              initWithFrame:CGRectMake(0, 0, [UIScreen mainScreen].bounds.size.width,
                                       [UIScreen mainScreen].bounds.size.height)];
        }
        if ([self.fullSpanItems containsObject:[NSNumber numberWithInteger:index]]) {
          model.type = LynxLayoutModelFullSpan;
        } else {
          model.type = LynxLayoutModelNormal;
        }
        [self.models insertObject:model atIndex:index];
        self.firstInvalidIndex = MIN(self.firstInvalidIndex, index);
      }];
}

- (void)updateModelsWithRemovals:(NSArray<NSNumber *> *)removals {
  // remove in reverse order
  if (removals.count == 0) {
    return;
  }
  [removals
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
                         NSInteger index = obj.integerValue;
                         [self.models removeObjectAtIndex:index];
                         self.firstInvalidIndex = MIN(self.firstInvalidIndex, index);
                       }];
}

- (void)updateModels:(NSDictionary<NSNumber *, NSValue *> *)updates {
  __block NSInteger firstIndex = NSIntegerMax;
  [updates enumerateKeysAndObjectsUsingBlock:^(NSNumber *_Nonnull key, NSValue *_Nonnull obj,
                                               BOOL *_Nonnull stop) {
    NSInteger index = key.integerValue;
    LynxListLayoutModelLight *updatedModel =
        [[LynxListLayoutModelLight alloc] initWithFrame:obj.CGRectValue];

    firstIndex = MIN(firstIndex, index);
    [self.models replaceObjectAtIndex:index withObject:updatedModel];
  }];
  self.firstInvalidIndex = firstIndex;
}

#pragma mark section model layout utils
- (CGFloat)largestSizeInMainSizes:(NSArray<NSNumber *> *)mainSizes {
  __block CGFloat size = 0;
  [mainSizes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        size = MAX(size, obj.floatValue);
      }];
  return size;
}

- (void)resetMainSizesWithNumberOfColumns:(NSUInteger)numberOfColumns {
  self.mainSizes = [[NSMutableArray alloc] initWithCapacity:self.numberOfColumns];
  for (NSUInteger i = 0; i < numberOfColumns; ++i) {
    [self.mainSizes addObject:[NSNumber numberWithFloat:0.]];
  }
}

- (CGFloat)layoutOffsetForFullSpanItems:(CGFloat)itemSize
                              crossSize:(CGFloat)collectionSize
                           paddingStart:(CGFloat)paddingStart
                             paddingEnd:(CGFloat)paddingEnd {
  CGFloat remainingSize = collectionSize - itemSize;

  if (remainingSize <= 0.) {
    return 0.;
  }

  remainingSize -= paddingStart + paddingEnd;

  if (remainingSize >= 0) {
    return paddingStart;
  }

  if (paddingStart > 1. && paddingEnd > 1.) {
    // if the width of the header is greater than the remaingWidth
    // we just apply padding on both side with same scale
    CGFloat scale = paddingStart / (paddingStart + paddingEnd);
    return paddingStart + (remainingSize * scale);
  }

  return 0.;
}

- (void)retrieveMainSizeFromCacheAtInvalidIndex:(NSInteger)invalidIndex {
  if (invalidIndex == 0) {
    // if the invalidation is from the beginning, just reset it.
    [self.mainSizesCache removeAllObjects];
    [self resetMainSizesWithNumberOfColumns:self.numberOfColumns];
    return;
  } else if (invalidIndex == kLynxListViewLayoutInvalidIndex) {
    // if there is no invalidation, do nothing
    return;
  }
  // retrieve the columnHeight we will start to compute with
  self.mainSizes = [NSMutableArray arrayWithArray:self.mainSizesCache[invalidIndex - 1]];
  // removal all trailing mainSizes
  [self.mainSizesCache
      removeObjectsInRange:NSMakeRange(invalidIndex, [self.mainSizesCache count] - invalidIndex)];
}

- (CGFloat)shortestMainSize {
  NSArray<NSNumber *> *mainSizes = self.mainSizes;
  __block CGFloat size = CGFLOAT_MAX;
  [mainSizes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        size = MIN(size, obj.floatValue);
      }];
  return size;
}

// the method is to determine whether an item in in the first row.
// if so, add 'gap' to its offset.
- (CGFloat)adjustOffsetAtIndex:(NSUInteger)index
                originalOffset:(CGFloat)Offset
               nearestFullSpan:(NSUInteger)nearestFullSpan {
  if (nearestFullSpan >= 0 && index > nearestFullSpan) {
    return Offset + self.mainAxisGap;
  }
  if (index < self.numberOfColumns) {
    return Offset;
  } else {
    return Offset + self.mainAxisGap;
  }
}

- (NSUInteger)shortestColumn {
  NSArray<NSNumber *> *mainSizes = self.mainSizes;
  __block CGFloat size = CGFLOAT_MAX;
  __block NSUInteger index = 0;
  [mainSizes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (size > obj.floatValue) {
          size = obj.floatValue;
          index = idx;
        }
      }];
  return index;
}

- (NSUInteger)findNearestFullSpanItem:(NSUInteger)index {
  NSInteger __block nearestFullSpanItem = kLynxListViewLayoutInvalidIndex;
  // Find the nearest fullspan item and layout from it. fullspan items is ordered
  [self.fullSpanItems enumerateObjectsUsingBlock:^(NSNumber *_Nonnull fullspanIndex, NSUInteger idx,
                                                   BOOL *_Nonnull stop) {
    if ((NSUInteger)fullspanIndex.integerValue < index) {
      nearestFullSpanItem = fullspanIndex.integerValue;
    } else if ((NSUInteger)fullspanIndex.integerValue > index) {
      *stop = YES;
    }
  }];
  return nearestFullSpanItem;
}

- (CGFloat)largestMainSize {
  return [self largestSizeInMainSizes:self.mainSizes];
}

- (CGFloat)largestMainSizeInPreviousRowAtIndex:(NSUInteger)index
                      withNearestFullSpanIndex:(NSUInteger)nearestFullSpanIndex {
  CGFloat lastOffset = 0;
  NSInteger columnForBeginIndex = [self findColumnIndex:index
                               withNearestFullSpanIndex:nearestFullSpanIndex];
  NSInteger lastItemInPreviousRowIndex = index - columnForBeginIndex - 1;
  if (lastItemInPreviousRowIndex == kLynxListViewLayoutInvalidIndex) {
    lastOffset = 0;
  } else {
    NSArray<NSNumber *> *prevSizes = self.mainSizesCache[lastItemInPreviousRowIndex];
    lastOffset = [self adjustOffsetAtIndex:index
                            originalOffset:[self largestSizeInMainSizes:prevSizes]
                           nearestFullSpan:nearestFullSpanIndex];
  }
  return lastOffset;
}

- (NSInteger)findColumnIndex:(NSInteger)index
    withNearestFullSpanIndex:(NSInteger)nearestFullSpanIndex {
  NSInteger columnForBeginIndex = 0;
  if (nearestFullSpanIndex >= 0) {
    columnForBeginIndex = (index - nearestFullSpanIndex - 1) % _numberOfColumns;
  } else {
    columnForBeginIndex = index % _numberOfColumns;
  }
  return columnForBeginIndex;
}

- (BOOL)layoutModelVisibleInIndex:(NSInteger)index {
  return NO;
}

#pragma mark core layout
- (void)layoutFrom:(NSInteger)startIndex to:(NSInteger)endIndex {
  startIndex = MAX(0, startIndex);
  // 0~lastValidModel should be continuous. so 0~7 then 20~27 is illegal.
  startIndex = MIN(self.lastValidModel, startIndex);
  endIndex = MIN(endIndex, self.getCount);
  self.lastValidModel = endIndex;
  CGFloat maxWidth = -1;
  [self retrieveMainSizeFromCacheAtInvalidIndex:startIndex];
  if (self.layoutColumnInfo.count == 0) {
    for (int i = 0; i < (NSInteger)self.numberOfColumns; i++) {
      [self.layoutColumnInfo addObject:[NSMutableArray array]];
    }
  } else {
    [self.layoutColumnInfo enumerateObjectsUsingBlock:^(NSMutableArray<NSNumber *> *_Nonnull obj,
                                                        NSUInteger idx, BOOL *_Nonnull stop) {
      [obj enumerateObjectsWithOptions:NSEnumerationReverse
                            usingBlock:^(NSNumber *_Nonnull index, NSUInteger idx,
                                         BOOL *_Nonnull stop) {
                              if (index.integerValue >= startIndex) {
                                [obj removeObject:index];
                              } else {
                                *stop = YES;
                              }
                            }];
    }];
  }
  NSUInteger nearestFullSpanIndex = [self findNearestFullSpanItem:startIndex];
  CGFloat lastOffset = [self largestMainSizeInPreviousRowAtIndex:startIndex
                                        withNearestFullSpanIndex:nearestFullSpanIndex];

  for (NSInteger item = startIndex; item < (NSInteger)self.models.count; item++) {
    if (item >= self.getCount) {
      break;
    }
    CGFloat mainAxisOffset = 0.0f;
    NSUInteger columnIndex = 0;
    BOOL isFullSpan = [self.fullSpanItems containsObject:[NSNumber numberWithInteger:item]];

    CGFloat itemCrossSize = [self crossSize:isFullSpan];
    CGFloat itemMainSize = [self mainSize:item];

    CGFloat crossAxisOffset = 0;

    if (isFullSpan) {
      self.models[item].type = LynxLayoutModelFullSpan;
      columnIndex = 0;
      float currentLargestMainSize = [self largestMainSize];
      mainAxisOffset =
          item > 0 ? currentLargestMainSize + self.mainAxisGap : currentLargestMainSize;
      nearestFullSpanIndex = item;
      crossAxisOffset = [self layoutOffsetForFullSpanItem:item];
      for (NSUInteger i = 0; i < MIN(self.mainSizes.count, (NSUInteger)self.getCount); ++i) {
        self.mainSizes[i] =
            @(mainAxisOffset + (self.needAlignHeight ? ceil(itemMainSize) : itemMainSize));
        self.models[i].type = LynxLayoutModelFullSpan;
      }
      [self.layoutColumnInfo enumerateObjectsUsingBlock:^(NSMutableArray<NSNumber *> *_Nonnull obj,
                                                          NSUInteger idx, BOOL *_Nonnull stop) {
        [obj addObject:@(item)];
      }];
    } else {
      self.models[item].type = LynxLayoutModelNormal;
      if (self.layoutType == LynxListLayoutWaterfall) {
        columnIndex = [self shortestColumn];
        mainAxisOffset = [self adjustOffsetAtIndex:item
                                    originalOffset:[self shortestMainSize]
                                   nearestFullSpan:nearestFullSpanIndex];
      } else {
        columnIndex = [self findColumnIndex:item withNearestFullSpanIndex:nearestFullSpanIndex];
        if (columnIndex == 0) {
          mainAxisOffset = [self adjustOffsetAtIndex:item
                                      originalOffset:[self largestMainSize]
                                     nearestFullSpan:nearestFullSpanIndex];
          lastOffset = mainAxisOffset;
        } else {
          mainAxisOffset = lastOffset;
        }
      }

      crossAxisOffset =
          [self crossUpperInset] + ((itemCrossSize + self.crossAxisGap) * columnIndex);
      self.mainSizes[columnIndex] =
          @(mainAxisOffset + (self.needAlignHeight ? ceil(itemMainSize) : itemMainSize));
      [self.layoutColumnInfo[columnIndex] addObject:@(item)];
    }
    [self.mainSizesCache addObject:[NSArray arrayWithArray:self.mainSizes]];

    self.models[item].frame = [self generateModelFrame:mainAxisOffset
                                       crossAxisOffset:crossAxisOffset
                                          itemMainSize:itemMainSize
                                         itemCrossSize:itemCrossSize];
    self.models[item].columnIndex = columnIndex;
    maxWidth = MAX(mainAxisOffset + itemMainSize, maxWidth);
  }

  self.firstInvalidIndex = 0;
}

#pragma mark override
- (CGFloat)layoutOffsetForFullSpanItem:(NSInteger)index {
  return 0.;
}

- (CGRect)generateModelFrame:(CGFloat)mainAxisOffset
             crossAxisOffset:(CGFloat)crossAxisOffset
                itemMainSize:(CGFloat)itemMainSize
               itemCrossSize:(CGFloat)itemCrossSize {
  return CGRectZero;
}

- (CGFloat)crossSize:(BOOL)isFullSpan {
  return 0.;
}

- (CGFloat)mainSize:(NSInteger)index {
  return 0.;
}

- (CGFloat)crossUpperInset {
  return 0.;
}

- (CGRect)defaultModelFrame:(NSNumber *)index {
  return CGRectZero;
}

#pragma mark view methods

- (LynxListLayoutModelLight *)attributesFromIndex:(NSInteger)index {
  if (index < 0 || index >= [self getCount]) {
    [NSException raise:@"Invalid layout model index"
                format:@"Invalid index %ld in range (0, %ld)", (long)index, (long)[self getCount]];
  }
  LynxListLayoutModelLight *model = [self.models objectAtIndex:(NSUInteger)index];
  return model;
}

- (NSDictionary<NSNumber *, NSNumber *> *)findWhichItemToDisplayOnTop {
  NSMutableDictionary<NSNumber *, NSNumber *> *displayItems = [NSMutableDictionary dictionary];
  [self.layoutColumnInfo enumerateObjectsUsingBlock:^(NSMutableArray<NSNumber *> *_Nonnull column,
                                                      NSUInteger columnIndex, BOOL *_Nonnull stop) {
    NSInteger index = [self findWhichItemToDisplayInColumn:columnIndex];
    if (index >= 0) {
      displayItems[@(columnIndex)] = @(index);
      // A fullspan item will occupy the whole line so the search should stop here.
      if ([self.fullSpanItems containsObject:@(index)]) {
        *stop = YES;
      }
    }
  }];
  return displayItems.copy;
}

- (NSInteger)findWhichItemToDisplayInColumn:(NSInteger)columnIndex {
  NSMutableArray<NSNumber *> *_Nonnull column = self.layoutColumnInfo[columnIndex];
  __block NSInteger displayIndex = -1;
  [column
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull index, NSUInteger idx, BOOL *_Nonnull stop) {
        LynxListLayoutModelLight *model = self.models[index.integerValue];
        if (CGRectIntersectsRect(model.frame, self.bounds)) {
          displayIndex = index.integerValue;
          *stop = YES;
        }
      }];
  return displayIndex;
}

#pragma mark basic info
- (CGSize)getContentSize {
  // override by subclasses
  return CGSizeZero;
}

- (NSInteger)getCount {
  return self.models.count;
}
@end
