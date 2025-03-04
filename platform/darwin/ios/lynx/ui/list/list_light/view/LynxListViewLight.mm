// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxEventTarget.h>
#import <Lynx/LynxListAnchorManager.h>
#import <Lynx/LynxListCachedCellManager.h>
#import <Lynx/LynxListHorizontalLayoutManager.h>
#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListReusePool.h>
#import <Lynx/LynxListVerticalLayoutManager.h>
#import <Lynx/LynxListViewCellLight.h>
#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIListDataSource.h>
#import <Lynx/LynxUIListInvalidationContext.h>
#import <Lynx/LynxUIListScrollManager.h>
#import <LynxUI+Internal.h>
#import <objc/runtime.h>
#import "LynxEventEmitter.h"
#import "LynxListDebug.h"
#import "LynxSubErrorCode.h"
#import "LynxTraceEvent.h"
#import "LynxUIComponent.h"
#import "LynxUIContext+Internal.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/shell/lynx_shell.h"

typedef NS_ENUM(NSInteger, LynxCellPosition) {
  LynxCellPositionNone,
  LynxCellPositionInBounds,
  LynxCellPositionAboveBounds,
  LynxCellPositionBelowBounds,
};
typedef NS_ENUM(NSInteger, LynxListViewScrollPosition) {
  LynxListViewScrollPositionTop,
  LynxListViewScrollPositionBottom,
  LynxListViewScrollPositionMiddle,
};
typedef NS_ENUM(NSInteger, LynxListViewSelfSizingAlignment) {
  LynxListViewSelfSizingAlignmentTop,
  LynxListViewSelfSizingAlignmentBottom,
};
typedef struct {
  CGFloat topToTop;        // anchorCell's top to bounds' top
  CGFloat topToBottom;     // anchorCell's top to bounds' bottom
  CGFloat bottomToTop;     // anchorCell's bottom to bounds' top
  CGFloat bottomToBottom;  // anchorCell's bottom to bounds' bottom
} LynxEdgeDistance;

static NSString *traceSectionName = @"view_light";

@interface LynxListViewLight ()
@property(nonatomic, strong) LynxListLayoutManager *innerLayout;
@property(nonatomic, weak) id<LynxListLayoutProtocol> customizedLayout;
@property(nonatomic, weak) LynxUIListDataSource *dataSource;
@property(nonatomic, strong) LynxUIListScrollManager *scrollManager;
@property(nonatomic, strong) LynxListAnchorManager *anchorManager;
@property(nonatomic, weak) LynxEventEmitter *eventEmitter;
@property(nonatomic, strong) LynxListCachedCellManager *cachedCells;
@property(nonatomic, strong) LynxListReusePool *reusePool;
@property(nonatomic, strong) NSMutableArray<NSString *> *reuseIdentifiers;
@property(nonatomic, weak) LynxUIContext *context;
@property(nonatomic, assign) NSInteger sign;
// Sticky items
@property(nonatomic, strong) NSArray<NSNumber *> *stickyTopItems;
@property(nonatomic, strong) NSArray<NSNumber *> *stickyBottomItems;
@property(nonatomic, assign) NSInteger initialScrollIndex;
// Should generate attachedCells in scroll events
@property(nonatomic, assign) BOOL needsVisibleCells;
// Flag to avoid inserting the same empty displayingCells in insertions and adjust bounds change
@property(nonatomic, assign) BOOL isHandlingEmptyDisplayingCells;
// Flag to avoid call cycle of selfsizing -> adjustWithBounds -> loadNewCell -> selfsizing
@property(nonatomic, assign) BOOL isSelfSizing;
@property(nonatomic, assign) BOOL isUpdatingInitialIndex;
@property(nonatomic, assign) BOOL blockFillInMoveAndUpdate;
@end

@implementation LynxListViewLight
#pragma mark init
- (instancetype)init {
  self = [super init];
  if (self) {
    self.delegate = self.scrollManager;
    self.cachedCells = [[LynxListCachedCellManager alloc] initWithColumnCount:1
                                                                    uiContext:self.context];
    self.initialScrollIndex = -1;
    self.anchorManager = [[LynxListAnchorManager alloc] init];
    self.reusePool = [[LynxListReusePool alloc] init];
    self.numberOfColumns = 1;
    self.anchorPriorityFromBegin = YES;
    self.deleteRegressPolicyToTop = YES;
    self.insertAnchorModeInside = NO;
  }
  return self;
}

- (LynxUIListScrollManager *)scrollManager {
  if (!_scrollManager) {
    _scrollManager = [[LynxUIListScrollManager alloc] init];
  }
  return _scrollManager;
}

- (void)setSign:(NSInteger)sign {
  _sign = sign;
  [_scrollManager setSign:sign];
}

- (void)setUIContext:(LynxUIContext *)context {
  _context = context;
}

- (void)setLayout:(LynxListLayoutManager *_Nullable)layout {
  if (nil == layout) {
    _innerLayout = [[LynxListVerticalLayoutManager alloc] init];
  } else {
    _customizedLayout = layout;
  }
}

- (void)setEventEmitter:(LynxEventEmitter *)eventEmitter {
  _eventEmitter = eventEmitter;
  [_scrollManager setEventEmitter:eventEmitter];
}

- (void)updateStickyItems:(LynxUIListInvalidationContext *)context {
  self.stickyTopItems = context.stickyTopItems;
  self.stickyBottomItems = context.stickyBottomItems;
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  if (![[self layout] isVerticalLayout]) {
    self.contentInset = UIEdgeInsetsMake(0, padding.left, 0, padding.right);
  } else {
    self.contentInset = UIEdgeInsetsMake(padding.top, 0, padding.bottom, 0);
  }
  [self layout].insets = padding;
}

#pragma mark utils
- (id<LynxListLayoutProtocol>)layout {
  return _customizedLayout ?: _innerLayout;
}

- (void)registerCellClass:(Class)cellClass
         reuseIdentifiers:(NSArray<NSString *> *)reuseIdentifiers {
  for (NSInteger i = 0; i < (NSInteger)reuseIdentifiers.count; i++) {
    [_reusePool registerClass:cellClass forCellReuseIdentifier:[reuseIdentifiers objectAtIndex:i]];
  }
}

- (void)updateScrollThresholds:(LynxUIListScrollThresholds *)scrollThreSholds {
  if (scrollThreSholds) {
    [self.scrollManager updateScrollThresholds:scrollThreSholds];
  }
}

- (void)setVerticalOrientation:(BOOL)verticalOrientation {
  _verticalOrientation = verticalOrientation;
  self.cachedCells.isVerticalLayout = verticalOrientation;
  if (!verticalOrientation && [_innerLayout isVerticalLayout]) {
    _innerLayout = [[LynxListHorizontalLayoutManager alloc] init];
  }
  // Dynamically change horizontalLayoutManager to vertical is not supported
  if (verticalOrientation && ![_innerLayout isVerticalLayout]) {
    LynxError *dynamicChangeError =
        [LynxError lynxErrorWithCode:ECLynxComponentListDynamicChangeOrientation
                             message:@"list dont support changing orientation dynamically"
                       fixSuggestion:@"Please do not change teh value of vertical-orientation."
                               level:LynxErrorLevelError];
    [_context reportLynxError:dynamicChangeError];
  }
  self.scrollManager.horizontal = !verticalOrientation;
}

- (void)updateReuseIdentifiers:(NSArray<NSString *> *)reuseIdentifiers {
  self.reuseIdentifiers = reuseIdentifiers.copy;
}

#pragma mark upper cache
// Find the cell with lowest origin.top to fill (waterfall).
- (NSInteger)nextIndexForUpperCache:(NSArray<NSNumber *> *)topCells {
  __block CGFloat lowestDistance = -1;
  __block NSInteger lowerIndex = -1, columnIndex = -1;
  [topCells
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger index = obj.integerValue;
        LynxListLayoutModelLight *model = [self.layout attributesFromIndex:index];
        if ([self orientationTopOrigin:model.frame] > lowestDistance) {
          lowestDistance = [self orientationTopOrigin:model.frame];
          lowerIndex = index;
          columnIndex = idx;
        }
      }];
  NSInteger nextIndex = [_anchorManager
      closestAttributesToUpperVisibleBound:lowerIndex
                                  inColumn:self.layout.layoutColumnInfo[columnIndex]];
  return nextIndex;
}

- (NSMutableOrderedSet<NSNumber *> *)upperCacheIndexes {
  NSMutableOrderedSet<NSNumber *> *upperCacheIndexes = [[NSMutableOrderedSet alloc] init];
  NSMutableDictionary<NSNumber *, id<LynxListCell>> *topCells = [self.cachedCells topCells];
  NSMutableArray<NSNumber *> *topIndexes = [NSMutableArray array];
  for (int i = 0; i < self.numberOfColumns; i++) {
    id<LynxListCell> cell = topCells[@(i)];
    // If column i don't have any cells, ask layout for a new one.
    if (!cell) {
      NSDictionary<NSNumber *, NSNumber *> *nextTopCellsIndexes =
          [self.layout findWhichItemToDisplayOnTop];
      [topIndexes addObject:nextTopCellsIndexes[@(i)]];
    }
    if (cell) {
      [topIndexes addObject:@(cell.updateToPath)];
    }
  }
  while ((NSInteger)upperCacheIndexes.count < self.preloadBufferCount) {
    NSInteger nextIndex = [self nextIndexForUpperCache:topIndexes];
    if (nextIndex < 0 || nextIndex >= self.layout.getCount ||
        [upperCacheIndexes containsObject:@(nextIndex)]) {
      break;
    }
    // upper cache shouldn't include stickyTopItems
    if (![self isSticky:nextIndex]) {
      [upperCacheIndexes insertObject:@(nextIndex) atIndex:0];
    }
    LynxListLayoutModelLight *model = [self.layout attributesFromIndex:nextIndex];
    if (model.type == LynxLayoutModelFullSpan) {
      [topIndexes enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx,
                                               BOOL *_Nonnull stop) {
        [topIndexes replaceObjectAtIndex:idx withObject:@(nextIndex)];
      }];
    } else {
      [topIndexes replaceObjectAtIndex:model.columnIndex withObject:@(nextIndex)];
    }
  }
  return upperCacheIndexes;
}

- (void)adjustUpperCache {
  if ([self.cachedCells isEmpty]) {
    return;
  }
  NSMutableArray<id<LynxListCell>> *refreshUpperCache = [NSMutableArray array];
  NSMutableOrderedSet<NSNumber *> *upperCacheIndexes = [self upperCacheIndexes];
  [upperCacheIndexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        // sticky shouldn't in uppercache
        NSInteger nextIndex = obj.integerValue;
        id<LynxListCell> cell = [self.cachedCells cellAtIndex:nextIndex];
        if (!cell) {
          cell = [self loadNewCellAtIndex:nextIndex];
          if (!cell) {
            *stop = YES;
          }
          [self invalidLayoutForCellIfNecessaryAndSync:cell];
        } else {
          [self.cachedCells removeCellAtIndex:cell.updateToPath];
        }
        [cell applyLayoutModel:[self.layout attributesFromIndex:cell.updateToPath]];
        [refreshUpperCache addObject:cell];
      }];
  [self.cachedCells.upperCachedCells
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                    BOOL *_Nonnull stop) {
                         [self recycleCell:obj];
                       }];
  self.cachedCells.upperCachedCells = refreshUpperCache;
}

#pragma mark lowerCache
// Find the cell with highest origin.bottom to fill (waterfall).
- (NSInteger)nextIndexForLowerCache:(NSMutableArray<NSNumber *> *)info {
  __block CGFloat upperDistance = CGFLOAT_MAX;
  __block NSInteger upperCellIndex;
  __block NSInteger columnIndex = -1;
  [info enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
    NSInteger index = obj.integerValue;
    LynxListLayoutModelLight *model = [self.layout attributesFromIndex:index];
    if ([self orientationBottomOfRect:model.frame] < upperDistance) {
      upperDistance = [self orientationBottomOfRect:model.frame];
      upperCellIndex = index;
      columnIndex = idx;
    }
  }];
  NSInteger nextIndex = [_anchorManager
      closestAttributesToLowerVisibleBound:upperCellIndex
                                  inColumn:self.layout.layoutColumnInfo[columnIndex]];
  return nextIndex;
}

- (NSMutableOrderedSet<NSNumber *> *)lowerCacheIndexes {
  NSMutableOrderedSet<NSNumber *> *lowerCacheIndexes = [[NSMutableOrderedSet alloc] init];
  NSMutableArray<NSNumber *> *bottomIndexes = [NSMutableArray array];
  NSMutableDictionary<NSNumber *, id<LynxListCell>> *bottomCells = [self.cachedCells bottomCells];
  for (int i = 0; i < self.numberOfColumns; i++) {
    id<LynxListCell> cell = bottomCells[@(i)];
    // If column i don't have any cells, ask layout for a new one.
    if (!cell) {
      NSDictionary<NSNumber *, NSNumber *> *nextBottomCellsIndexes =
          [self.layout findWhichItemToDisplayOnTop];
      [bottomIndexes addObject:nextBottomCellsIndexes[@(i)]];
    }
    if (cell) {
      [bottomIndexes addObject:@(cell.updateToPath)];
    }
  }
  while ((NSInteger)lowerCacheIndexes.count < self.preloadBufferCount) {
    NSInteger nextIndex = [self nextIndexForLowerCache:bottomIndexes];
    if (nextIndex < 0 || nextIndex >= self.layout.getCount ||
        [lowerCacheIndexes containsObject:@(nextIndex)]) {
      break;
    }
    [lowerCacheIndexes insertObject:@(nextIndex) atIndex:0];
    LynxListLayoutModelLight *model = [self.layout attributesFromIndex:nextIndex];
    if (model.type == LynxLayoutModelFullSpan) {
      [bottomIndexes enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx,
                                                  BOOL *_Nonnull stop) {
        [bottomIndexes replaceObjectAtIndex:idx withObject:@(nextIndex)];
      }];
    } else {
      [bottomIndexes replaceObjectAtIndex:model.columnIndex withObject:@(nextIndex)];
    }
  }
  return lowerCacheIndexes;
}

- (void)adjustLowerCache {
  if ([self.cachedCells isEmpty]) {
    return;
  }
  NSMutableArray<id<LynxListCell>> *refreshLowerCache = [NSMutableArray array];
  NSMutableOrderedSet<NSNumber *> *lowerCacheIndexes = [self lowerCacheIndexes];
  [lowerCacheIndexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger nextIndex = obj.integerValue;
        if (![self isSticky:nextIndex]) {
          id<LynxListCell> cell = [self.cachedCells cellAtIndex:nextIndex];
          if (!cell) {
            cell = [self loadNewCellAtIndex:nextIndex];
            if (!cell) {
              *stop = YES;
            }
            [self invalidLayoutForCellIfNecessaryAndSync:cell];
          } else {
            [self.cachedCells removeCellAtIndex:cell.updateToPath];
          }
          [cell applyLayoutModel:[self.layout attributesFromIndex:cell.updateToPath]];
          [refreshLowerCache addObject:cell];
        }
      }];
  [self.cachedCells.lowerCachedCells
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                    BOOL *_Nonnull stop) {
                         [self recycleCell:obj];
                       }];
  self.cachedCells.lowerCachedCells = refreshLowerCache;
}

#pragma mark dispatch
- (void)dispatchInvalidationContext:(LynxUIListInvalidationContext *)context {
  if (!context) {
    return;
  }
  [LynxTraceEvent beginSection:traceSectionName
                      withName:@"dispatchInvalidationContext"
                     debugInfo:@{@"updateType" : @(context.listUpdateType)}];
  [[self layout] updateBasicInvalidationContext:context bounds:self.bounds];
  switch (context.listUpdateType) {
    case LynxListUpdateTypeDataUpdate:
      [self performBatchUpdatesWithContext:context];
      break;
    case LynxListUpdateTypeLayoutGeneralPropsUpdate:
      [self performGeneralPropsUpdate:context];
      break;
    case LynxListUpdateTypeInitialScrollIndexUpdate:
      [self performInitialScrollIndex:context.initialScrollIndex];
      break;
    case LynxListUpdateTypeScrollBoundsUpdate:
      [self adjustWithBoundsChange];
      break;
    case LynxListUpdateTypeScrollThresholdsUpdate:
      [self performScrollThresholdsUpdate:context];
      break;
    case LynxListUpdateTypeLayoutSelfSizing:
      [self performSelfSizing:context];
      break;
    case LynxListUpdateTypeScrollToPositionUpdate:
      [self performScrollToPosition:context];
      break;
    default:
      break;
  }
  [LynxTraceEvent endSection:traceSectionName withName:@"dispatchInvalidationContext"];
}

- (void)performBatchUpdatesWithContext:(LynxUIListInvalidationContext *)context {
  LYNX_LIST_DEBUG_LOG(
      @"diffResult: removals:%@, insertions:%@, updateTo:%@, updateFrom:%@, moveTo:%@, moveFrom:%@",
      context.removals, context.insertions, context.updateTo, context.updateFrom, context.moveTo,
      context.moveFrom);

  [self updateStickyItems:context];
  // The adjustWithBounds should not be called before all updates finish completely
  _blockFillInMoveAndUpdate = YES;
  if (context.removals && context.removals.count > 0) {
    [self updateModelsWithRemovals:context.removals];
    [self deleteItemsAtIndexes:context.removals];
  }
  if (context.insertions && context.insertions.count > 0) {
    [self updateModelsWithInsertions:context.insertions];
    [self insertItemsAtIndexes:context.insertions];
  }
  if (context.moveTo && context.moveFrom && context.moveTo.count > 0 &&
      context.moveFrom.count > 0 && context.moveTo.count == context.moveFrom.count) {
    [context.moveFrom
        enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
          [self updateModelsWithRemovals:@[ obj ]];
          [self deleteItemsAtIndexes:@[ obj ]];

          [self updateModelsWithInsertions:@[ [context.moveTo objectAtIndex:idx] ]];
          [self insertItemsAtIndexes:@[ [context.moveTo objectAtIndex:idx] ]];
        }];
  }
  if (context.updateTo && context.updateFrom && context.updateTo.count > 0 &&
      context.updateFrom.count > 0 && context.updateTo.count == context.updateFrom.count) {
    [context.updateTo
        enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
          NSInteger updateTo = obj.integerValue;
          NSInteger updateFrom = [context.updateTo objectAtIndex:idx].integerValue;
          [self updateCellAtIndex:updateFrom toIndex:updateTo];
        }];
  }
  self.contentSize = [[self layout] getContentSize];
  _blockFillInMoveAndUpdate = NO;
  [self adjustWithBoundsChange];
  self.contentSize = [[self layout] getContentSize];
  if (self.initialScrollIndex >= 0) {
    if (self.initialScrollIndex < [[self layout] getCount]) {
      self.contentOffset = [self
          orientationPointConverter:
              [self orientationTopOrigin:[self.layout attributesFromIndex:self.initialScrollIndex]
                                             .frame]];
      self.initialScrollIndex = -1;
    }
  }
}

- (void)performGeneralPropsUpdate:(LynxUIListInvalidationContext *)context {
  [[self layout] layoutFrom:0 to:[self layout].getCount];
  self.cachedCells.numberOfColumns =
      context.numberOfColumns > 0 ? context.numberOfColumns : self.cachedCells.numberOfColumns;
  [self invalidLayoutFromIndex:0];
  self.anchorManager.numberOfColumns =
      context.numberOfColumns > 0 ? context.numberOfColumns : self.anchorManager.numberOfColumns;
  self.needsVisibleCells =
      context.needsVisibleCells ? context.needsVisibleCells.boolValue : self.needsVisibleCells;
  self.contentSize = [[self layout] getContentSize];
}

- (void)performScrollToPosition:(LynxUIListInvalidationContext *)context {
  BOOL isSticky = [self isSticky:context.scrollToPosition];
  CGPoint targetOffset = [self targetContentOffsetAtIndex:context.scrollToPosition
                                                   offset:context.offset
                                                   sticky:isSticky
                                           scrollPosition:context.alignTo];

  __weak __typeof(self) weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    CGPoint idealTargetOffset = [strongSelf targetContentOffsetAtIndex:context.scrollToPosition
                                                                offset:context.offset
                                                                sticky:isSticky
                                                        scrollPosition:context.alignTo];
    if (!(CGPointEqualToPoint(strongSelf.contentOffset, idealTargetOffset))) {
      [strongSelf setContentOffset:idealTargetOffset animated:NO];
    }
  });
  [self setContentOffset:targetOffset animated:context.smooth];
}

- (void)performSelfSizing:(LynxUIListInvalidationContext *)context {
  self.isSelfSizing = YES;
  if (!self.cachedCells.isEmpty) {
    __block NSInteger firstIndex = self.cachedCells.lastVisibleCell.updateToPath;
    [self.cachedCells.displayingCells
        enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          if (obj.updateToPath < firstIndex && !context.updates[@(obj.updateToPath)]) {
            firstIndex = obj.updateToPath;
          }
        }];
    if (!context.updates[@(firstIndex)]) {
      // Can find a valid anchor
      [self handleUpdatesAlignments:LynxListViewSelfSizingAlignmentTop
                         alignIndex:firstIndex
                            updates:context.updates];
    } else {
      // If can't find valid anchor, use previous cell as anchor
      // Now the initial index only supports single column
      if (self.anchorPriorityFromBegin) {
        firstIndex = MAX(0, firstIndex - 1);
        [self handleUpdatesAlignments:LynxListViewSelfSizingAlignmentBottom
                           alignIndex:firstIndex
                              updates:context.updates];
      } else {
        firstIndex = MIN([[self layout] getCount], firstIndex + 1);
        [self handleUpdatesAlignments:LynxListViewSelfSizingAlignmentBottom
                           alignIndex:firstIndex
                              updates:context.updates];
      }
    }
  } else {
    // If the cachedCells is empty, only need to trigger layout models and adjust contentOffset.
    [[self layout] updateModels:context.updates];
    NSInteger firstInvalidIndex = self.layout.firstInvalidIndex;
    [[self layout]
        layoutFrom:[self layout].firstInvalidIndex
                to:MAX(self.cachedCells.lastIndexInPathOrder, [self layout].firstInvalidIndex) +
                   self.preloadBufferCount + self.numberOfColumns + 1];

    self.contentSize = [[self layout] getContentSize];
    NSInteger index = firstInvalidIndex;
    LynxListLayoutModelLight *model = [[self layout] attributesFromIndex:index];
    self.contentOffset = [self orientationPointConverter:[self orientationTopOrigin:model.frame]];
  }
  self.isSelfSizing = NO;
}

- (void)performInitialScrollIndex:(NSInteger)initialScrollIndex {
  self.initialScrollIndex = initialScrollIndex;
}

- (void)performScrollThresholdsUpdate:(LynxUIListInvalidationContext *)context {
  [self updateScrollThresholds:context.scrollThresholds];
}

#pragma mark load & recycle
- (id<LynxListCell>)loadNewCellAtIndex:(NSInteger)index {
  [LynxTraceEvent beginSection:traceSectionName
                      withName:@"loadNewCellAtIndex"
                     debugInfo:@{@"index" : @(index)}];
  LYNX_LIST_DEBUG_LOG(@"(%@)loadNewCellAtIndex: %ld", self, (long)index);
  if (index < 0 || index >= [self layout].getCount) {
    return nil;
  }
  id<LynxListCell> cell = [self.dataSource listView:self cellForItemAtIndex:index];
  cell.updateToPath = index;
  cell.reuseIdentifier = self.reuseIdentifiers[index];
  [LynxTraceEvent endSection:traceSectionName withName:@"loadNewCellAtIndex"];
  return cell;
}

- (void)onComponentLayoutUpdated:(LynxUIComponent *)component {
  [LynxTraceEvent beginSection:traceSectionName withName:@"onComponentLayoutUpdated"];
  UIView *cellView = component.view.superview.superview;

  if (![cellView respondsToSelector:NSSelectorFromString(@"updateToPath")]) {
    return;
  }
  LynxListViewCellLightLynxUI *cell = (LynxListViewCellLightLynxUI *)cellView;
  NSInteger index = cell.updateToPath;
  if (index < [[self layout] getCount]) {
    [self dispatchInvalidationContext:[[LynxUIListInvalidationContext alloc] initWithModelUpdates:@{
            @(index) : [NSValue valueWithCGRect:component.updatedFrame]
          }]];
    [self invalidLayoutFromIndex:cell.updateToPath];
    [cell applyLayoutModel:[[self layout] attributesFromIndex:cell.updateToPath]];
  }
  [LynxTraceEvent endSection:traceSectionName withName:@"onComponentLayoutUpdated"];
}

- (void)onAsyncComponentLayoutUpdated:(nonnull LynxUIComponent *)component
                          operationID:(int64_t)operationID {
  [LynxTraceEvent beginSection:traceSectionName withName:@"onAsyncComponentLayoutUpdated"];
  if (self.isAsync) {
    [self.cachedCells.allCachedCells
        enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          if ([obj isKindOfClass:LynxListViewCellLightLynxUI.class]) {
            LynxListViewCellLightLynxUI *cell = (LynxListViewCellLightLynxUI *)obj;
            if (operationID == cell.operationID) {
              if (cell.ui != component) {
                LynxUI *oriUI = cell.ui;
                if (oriUI) {
                  [cell removeLynxUI];
                  auto shellPtr = _context.shellPtr;
                  if (shellPtr) {
                    reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->EnqueueListNode(
                        static_cast<int32_t>(self.sign), static_cast<int32_t>(oriUI.sign));
                  }
                }
                [cell addLynxUI:component];
              }
              if (self.enableFadeInAnimation) {
                cell.contentView.alpha = 0;
                [UIView animateWithDuration:_updateAnimationFadeInDuration
                                      delay:0
                                    options:UIViewAnimationOptionAllowUserInteraction
                                 animations:^{
                                   cell.contentView.alpha = 1;
                                 }
                                 completion:^(BOOL finished){

                                 }];
              }
              [self invalidLayoutForCellIfNecessary:cell];
              [self adjustWithBoundsChange];
              *stop = YES;
            }
          }
        }];
  }
  [LynxTraceEvent endSection:traceSectionName withName:@"onAsyncComponentLayoutUpdated"];
}

- (void)recycleCell:(id<LynxListCell>)cell {
  [LynxTraceEvent beginSection:traceSectionName
                      withName:@"recycleCell"
                     debugInfo:@{@"cell_info" : @(cell.updateToPath)}];
  LYNX_LIST_DEBUG_LOG(@"(%@)recycleCell %ld info: %@", self, cell.updateToPath, cell.itemKey);
  [self.dataSource listView:self recycleCell:cell];
  [self.cachedCells removeCellAtIndex:cell.updateToPath];
  [_reusePool enqueueReusableCell:cell];
  LYNX_LIST_DEBUG_LOG(@"(%@)AfterRecycleCell %@", self, self.cachedCells.description);
  [LynxTraceEvent endSection:traceSectionName withName:@"recycleCell"];
}

- (id<LynxListCell>)dequeueReusableCellForIndex:(NSInteger)index {
  return [_reusePool dequeueReusableCellInIndex:index
                            withReuseIdentifier:[_reuseIdentifiers objectAtIndex:index]];
}

#pragma mark fill
// Use flag cell
- (void)adjustWithBoundsChange {
  [LynxTraceEvent beginSection:traceSectionName withName:@"adjustWithBoundsChange"];
  if (self.isSelfSizing || self.blockFillInMoveAndUpdate) {
    [LynxTraceEvent endSection:traceSectionName withName:@"adjustWithBoundsChange"];
    return;
  }
  // if componentCompleteInfo flush before diff info, return here.
  if ([[self layout] getCount] == 0) {
    [LynxTraceEvent endSection:traceSectionName withName:@"adjustWithBoundsChange"];
    return;
  }
  // if bouncing, don't trigger new layout.
  if ([self isBouncing]) {
    [LynxTraceEvent endSection:traceSectionName withName:@"adjustWithBoundsChange"];
    return;
  }
  [self refreshDisplayCells];
  LYNX_LIST_DEBUG_LOG(@"cachedCells: %@", self.cachedCells);
  if (self.cachedCells.displayingCells.count > 0) {
    [self offloadStickyTop];
    [self fillToUpperBoundsIfNecessary];
    [self fillToLowerBoundsIfNecessary];
    [self handleSticky];
  } else if (!self.isHandlingEmptyDisplayingCells) {
    // do not trigger insertions inside insertions
    [self loadAppropriateCellInCurrentOffset];
    [self fillToUpperBoundsIfNecessary];
    [self fillToLowerBoundsIfNecessary];
  }

  [self adjustLowerCache];
  [self adjustUpperCache];
  [LynxTraceEvent endSection:traceSectionName withName:@"adjustWithBoundsChange"];
}

/**
 Find suitable cell for certain offset and ensure the display will not be blank. Called when the
 offset changed too fast and caches are all empty.
 */
- (void)loadAppropriateCellInCurrentOffset {
  NSDictionary<NSNumber *, NSNumber *> *cellShouldDisplay =
      [[self layout] findWhichItemToDisplayOnTop];
  while (cellShouldDisplay.count == 0 &&
         [self layout].lastValidModel < [self layout].getCount - 1) {
    [[self layout]
        layoutFrom:[self layout].firstInvalidIndex
                to:MAX(self.cachedCells.lastIndexInPathOrder, [self layout].firstInvalidIndex) +
                   self.preloadBufferCount + 1];
  }
  [cellShouldDisplay enumerateKeysAndObjectsUsingBlock:^(
                         NSNumber *_Nonnull key, NSNumber *_Nonnull obj, BOOL *_Nonnull stop) {
    NSInteger index = obj.integerValue;
    id<LynxListCell> cell = [self loadNewCellAtIndex:index];
    [self invalidLayoutForCellIfNecessaryAndSync:cell];
    [self.cachedCells addCell:cell inArray:self.cachedCells.displayingCells];
    [cell applyLayoutModel:[[self layout] attributesFromIndex:index]];
    // fullspan item.
    if (cell.layoutType == LynxLayoutModelFullSpan) {
      *stop = YES;
    }
  }];
}

- (void)fillToUpperBoundsIfNecessary {
  [LynxTraceEvent beginSection:traceSectionName withName:@"fillToUpperBoundsIfNecessary"];
  NSMutableDictionary<NSNumber *, id<LynxListCell>> *topCells = [self.cachedCells topCells];
  [topCells enumerateKeysAndObjectsUsingBlock:^(
                NSNumber *_Nonnull key, id<LynxListCell> _Nonnull obj, BOOL *_Nonnull stop) {
    CGFloat currentDistance =
        [self orientationTopOrigin:obj.frame] - MAX(0, [self orientationTopOrigin:self.bounds]);
    id<LynxListCell> currentCell = obj;
    while (currentDistance > CGFLOAT_EPSILON) {
      NSInteger nextIndex = [self.anchorManager
          closestAttributesToUpperVisibleBound:currentCell.updateToPath
                                      inColumn:[self layout].layoutColumnInfo[key.integerValue]];
      // Invalid nextIndex as layout is lazy.
      if (nextIndex < 0) {
        nextIndex = [self triggerLayoutManuallyToFindNextIndex:currentCell toTop:YES];
      }
      if (![[self layout] layoutModelVisibleInIndex:nextIndex]) {
        break;
      }
      id<LynxListCell> cell = [self.cachedCells cellAtIndex:nextIndex];
      if (!cell) {
        cell = [self loadNewCellAtIndex:nextIndex];
        if (!cell) {
          break;
        }
        [self invalidLayoutForCellIfNecessaryAndSync:cell];
      } else {
        [self.cachedCells removeCellAtIndex:cell.updateToPath];
      }
      [cell applyLayoutModel:[[self layout] attributesFromIndex:cell.updateToPath]];
      [self.cachedCells addCell:cell inArray:self.cachedCells.displayingCells];
      currentCell = cell;
      currentDistance =
          [self orientationTopOrigin:cell.frame] - [self orientationTopOrigin:self.bounds];
    }
  }];
  [LynxTraceEvent endSection:traceSectionName withName:@"fillToUpperBoundsIfNecessary"];
}

- (BOOL)needFillToBottom:(NSMutableDictionary<NSNumber *, id<LynxListCell>> *)bottomCells {
  __block BOOL need = NO;
  [bottomCells enumerateKeysAndObjectsUsingBlock:^(
                   NSNumber *_Nonnull key, id<LynxListCell> _Nonnull obj, BOOL *_Nonnull stop) {
    CGFloat distance =
        [self orientationBottomOfRect:self.bounds] - [self orientationBottomOfRect:obj.frame];
    if (distance > 0) {
      need = YES;
      *stop = YES;
    }
  }];
  return need;
}

- (void)fillToLowerBoundsIfNecessary {
  [LynxTraceEvent beginSection:traceSectionName withName:@"fillToLowerBoundsIfNecessary"];
  NSMutableDictionary<NSNumber *, id<LynxListCell>> *bottomCells = [self.cachedCells bottomCells];
  // When the layoutModel didn't change its size, we should trigger lazy layout as the
  // invalidLayoutForCellIfNecessaryAndSync won't trigger layout.
  NSInteger maxNextIndex = -1;
  while ([self needFillToBottom:bottomCells]) {
    __block NSInteger maxIndex = -1;
    [bottomCells enumerateKeysAndObjectsUsingBlock:^(
                     NSNumber *_Nonnull key, id<LynxListCell> _Nonnull obj, BOOL *_Nonnull stop) {
      maxIndex = MAX(obj.updateToPath, maxIndex);
    }];
    NSInteger nextIndex = maxIndex + 1;
    LYNX_LIST_DEBUG_LOG(@"(%@)nextIndex %ld", self, nextIndex);
    maxNextIndex = MAX(nextIndex, maxNextIndex);
    if (nextIndex > [self layout].getCount - 1) {
      break;
    }
    LynxListLayoutModelLight *model = [[self layout] attributesFromIndex:nextIndex];
    if (CGRectIntersectsRect(model.frame, self.bounds)) {
      id<LynxListCell> cell;
      if ([self isSticky:nextIndex]) {
        cell = [self.cachedCells findCellAtIndex:nextIndex
                                         inArray:self.cachedCells.displayingCells];
      } else {
        cell = [self.cachedCells findCellAtIndex:nextIndex
                                         inArray:self.cachedCells.lowerCachedCells];
      }
      if (!cell) {
        cell = [self loadNewCellAtIndex:nextIndex];
        if (!cell) {
          break;
        }
        [self invalidLayoutForCellIfNecessaryAndSync:cell];
      } else {
        [self.cachedCells removeCellAtIndex:cell.updateToPath];
      }
      [cell applyLayoutModel:[[self layout] attributesFromIndex:cell.updateToPath]];
      [self.cachedCells addCell:cell inArray:self.cachedCells.displayingCells];
      bottomCells[@(cell.columnIndex)] = cell;
    } else {
      break;
    }
  }
  LYNX_LIST_DEBUG_LOG(@"(%@)maxNextIndex %ld, lastValidModel %ld", self, maxNextIndex,
                      self.layout.lastValidModel);
  // If the nextIndex used all valid layout models, trigger layout.
  if (maxNextIndex >= self.layout.lastValidModel) {
    [self.layout layoutFrom:maxNextIndex
                         to:maxNextIndex + self.preloadBufferCount + self.numberOfColumns];
    self.contentSize = [[self layout] getContentSize];
  }
  [LynxTraceEvent endSection:traceSectionName withName:@"fillToLowerBoundsIfNecessary"];
}

// If scrolls too fast and the lazy layout don't have enough valid layout models, trigger it
// additionally.
- (NSInteger)triggerLayoutManuallyToFindNextIndex:(id<LynxListCell>)currentCell toTop:(BOOL)toTop {
  [[self layout]
      layoutFrom:self.cachedCells.firstIndexInPathOrder
              to:currentCell.updateToPath + self.numberOfColumns + self.preloadBufferCount + 1];
  NSInteger nextIndex = -1;
  if (toTop) {
    nextIndex = [self.anchorManager
        closestAttributesToUpperVisibleBound:currentCell.updateToPath
                                    inColumn:[self layout]
                                                 .layoutColumnInfo[currentCell.columnIndex]];
  } else {
    nextIndex = [self.anchorManager
        closestAttributesToLowerVisibleBound:currentCell.updateToPath
                                    inColumn:[self layout]
                                                 .layoutColumnInfo[currentCell.columnIndex]];
  }
  return nextIndex;
}

- (void)refreshDisplayCells {
  [LynxTraceEvent beginSection:traceSectionName withName:@"refreshDisplayCells"];
  LynxListCachedCellManager *adjustedCachedCells =
      [[LynxListCachedCellManager alloc] initWithColumnCount:self.cachedCells.numberOfColumns
                                                   uiContext:_context];
  [self.cachedCells.allCachedCells enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj,
                                                                NSUInteger idx,
                                                                BOOL *_Nonnull stop) {
    if ([self orientationBottomOfRect:obj.frame] < [self orientationTopOrigin:self.bounds]) {
      [adjustedCachedCells addCell:obj inArray:adjustedCachedCells.upperCachedCells];
    } else if ([self orientationTopOrigin:obj.frame] > [self orientationBottomOfRect:self.bounds]) {
      [adjustedCachedCells addCell:obj inArray:adjustedCachedCells.lowerCachedCells];
    } else {
      [adjustedCachedCells addCell:obj inArray:adjustedCachedCells.displayingCells];
    }
  }];
  self.cachedCells = adjustedCachedCells;
  [LynxTraceEvent endSection:traceSectionName withName:@"refreshDisplayCells"];
}

#pragma mark insert
- (void)insertItemsAtIndexes:(NSArray<NSNumber *> *)indexes {
  if (!indexes || indexes.count == 0) {
    return;
  }
  if ([self.cachedCells isEmpty]) {
    [self insertInEmptyDisplayingCells];
    return;
  }
  NSInteger anchorIndex = [_anchorManager findAnchorCell:self.cachedCells
                                          anchorPolicies:[self makeAnchorPolicies]
                                              layoutInfo:self.layout.layoutColumnInfo];
  id<LynxListCell> anchorCell = [self getAnchorCellInIndex:anchorIndex];
  LynxEdgeDistance anchorDistance = [self makeAnchorDistance:anchorCell];

  [self updateUpdateToPathAfterInsertion:indexes];
  [self.cachedCells.allCachedCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        [obj applyLayoutModel:[self.layout attributesFromIndex:obj.updateToPath]];
      }];
  // If the list has estimatedHeight, newly inserted cell may only take small space in screen,
  // resulting in skipping the filltoUpper/LowerBounds
  NSMutableArray<id<LynxListCell>> *insertedCell = [NSMutableArray array];
  [indexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if (obj.integerValue >= self.cachedCells.firstIndexInPathOrder &&
            obj.integerValue <= self.cachedCells.lastIndexInPathOrder) {
          id<LynxListCell> cell = [self loadNewCellAtIndex:obj.integerValue];
          [self invalidLayoutForCellIfNecessaryAndSync:cell];
          [cell applyLayoutModel:[self.layout attributesFromIndex:obj.integerValue]];
          [insertedCell addObject:cell];
        }
      }];
  // Add them in displayingCells first and adjust them to actual pool in adjustWithBoundsChange.
  [insertedCell enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                             BOOL *_Nonnull stop) {
    [self.cachedCells addCell:obj inArray:self.cachedCells.displayingCells];
  }];
  [self adjustContentOffsetBasedOnPolicy:anchorDistance anchor:anchorCell];
}

- (void)updateUpdateToPathAfterInsertion:(NSArray<NSNumber *> *)indexes {
  [indexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger currentIndex = obj.integerValue;
        [self.cachedCells.allCachedCells
            enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                         BOOL *_Nonnull stop) {
              if (obj.updateToPath >= currentIndex) {
                obj.updateToPath = obj.updateToPath + 1;
              }
            }];
      }];
  [self.cachedCells markCellInfoDirty];
}

- (void)insertInEmptyDisplayingCells {
  self.isHandlingEmptyDisplayingCells = YES;
  NSDictionary<NSNumber *, NSNumber *> *cellShouldDisplay =
      [[self layout] findWhichItemToDisplayOnTop];
  [cellShouldDisplay enumerateKeysAndObjectsUsingBlock:^(
                         NSNumber *_Nonnull key, NSNumber *_Nonnull obj, BOOL *_Nonnull stop) {
    NSInteger index = obj.integerValue;
    id<LynxListCell> cell = [self loadNewCellAtIndex:index];
    [self invalidLayoutForCellIfNecessaryAndSync:cell];
    [cell applyLayoutModel:[[self layout] attributesFromIndex:index]];
    [self.cachedCells addCell:cell inArray:self.cachedCells.displayingCells];

    // fullspan item will occupy the whole line
    if (cell.layoutType == LynxLayoutModelFullSpan) {
      *stop = YES;
    }
  }];
  [self adjustWithBoundsChange];
  self.isHandlingEmptyDisplayingCells = NO;
}

- (void)updateModelsWithInsertions:(NSArray<NSNumber *> *)insertions {
  if (!insertions || insertions.count == 0) {
    return;
  }
  [[self layout] updateModelsWithInsertions:insertions];
  [[self layout] layoutFrom:[self layout].firstInvalidIndex to:[self layout].getCount];
}

#pragma mark delete
- (void)updateModelsWithRemovals:(NSArray<NSNumber *> *)removals {
  if (!removals || removals.count == 0) {
    return;
  }
  [[self layout] updateModelsWithRemovals:removals];
  [[self layout] layoutFrom:removals.firstObject.integerValue
                         to:self.cachedCells.lastIndexInPathOrder + self.numberOfColumns +
                            self.preloadBufferCount + 1];
}

- (void)deleteItemsAtIndexes:(NSArray<NSNumber *> *)indexes {
  LYNX_LIST_DEBUG_LOG(@"(%@)deleteItemsAtIndexes indexes %@", self, indexes);
  __block BOOL validDeletion = NO;
  [indexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        validDeletion = [self.cachedCells markRemoveCellAtIndex:obj.integerValue] || validDeletion;
      }];
  // No changing in displaying or cached cells.
  if (!validDeletion) {
    [self updateUpdateToPathAfterRemoval:indexes];
    id<LynxListCell> cell = self.cachedCells.allCachedCells.firstObject;
    NSInteger index = cell.updateToPath;
    CGFloat prevDistance = [self orientationBottomOfRect:cell.frame] - self.contentOffset.y;
    [self.cachedCells.allCachedCells
        enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                     BOOL *_Nonnull stop) {
          [obj applyLayoutModel:[self.layout attributesFromIndex:obj.updateToPath]];
        }];
    LynxListLayoutModelLight *model = [[self layout] attributesFromIndex:index];
    self.contentOffset =
        [self orientationPointConverter:[self orientationBottomOfRect:model.frame] - prevDistance];
    return;
  }
  NSInteger anchorIndex = [_anchorManager findAnchorCellForRemoval:self.cachedCells
                                                    anchorPolicies:[self makeAnchorPolicies]
                                                        layoutInfo:self.layout.layoutColumnInfo
                                                     deleteIndexes:indexes];
  id<LynxListCell> anchorCell = [self getAnchorCellInIndex:anchorIndex];
  LynxEdgeDistance anchorDistance = [self makeAnchorDistance:anchorCell];

  [indexes
      enumerateObjectsWithOptions:NSEnumerationReverse
                       usingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
                         id<LynxListCell> cell = [self.cachedCells cellAtIndex:obj.integerValue];
                         if (cell) {
                           [self recycleCell:cell];
                         }
                       }];
  LYNX_LIST_DEBUG_LOG(@"(%@)before updateUpdateToPathAfterRemoval %@", self,
                      self.cachedCells.description);
  [self updateUpdateToPathAfterRemoval:indexes];
  [self.cachedCells.allCachedCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        [obj applyLayoutModel:[self.layout attributesFromIndex:obj.updateToPath]];
        obj.removed = NO;
      }];
  // Only adjust the contentOffset but not along with the adjustWithBoundsChange as the
  // blockFillInMoveAndUpdate is still NO
  [self adjustContentOffsetBasedOnPolicy:anchorDistance anchor:anchorCell];
}

- (id<LynxListCell>)getAnchorCellInIndex:(NSInteger)anchorIndex {
  id<LynxListCell> anchorCell = [self.cachedCells cellAtIndex:anchorIndex];
  if (anchorCell) {
    return anchorCell;
  }
  anchorCell = [self loadNewCellAtIndex:anchorIndex];
  if (!anchorCell) {
    return nil;
  }
  [self invalidLayoutForCellIfNecessaryAndSync:anchorCell];
  LynxListLayoutModelLight *model = [self.layout attributesFromIndex:anchorCell.updateToPath];
  [anchorCell applyLayoutModel:model];
  LynxCellPosition cellRelatedPosition = [self cellPosition:model];
  switch (cellRelatedPosition) {
    case LynxCellPositionNone:
      anchorCell = nil;
      break;
    case LynxCellPositionInBounds:
      [self.cachedCells addCell:anchorCell inArray:self.cachedCells.displayingCells];
      break;
    case LynxCellPositionAboveBounds:
      [self.cachedCells addCell:anchorCell inArray:self.cachedCells.upperCachedCells];
      break;
    case LynxCellPositionBelowBounds:
      [self.cachedCells addCell:anchorCell inArray:self.cachedCells.lowerCachedCells];
      break;
    default:
      break;
  }
  return anchorCell;
}

- (void)updateUpdateToPathAfterRemoval:(NSArray<NSNumber *> *)indexes {
  NSMutableArray<NSNumber *> *subs =
      [NSMutableArray arrayWithCapacity:self.cachedCells.allCachedCells.count];
  for (NSInteger i = 0; i < (NSInteger)self.cachedCells.allCachedCells.count; i++) {
    [subs addObject:@0];
  }
  [indexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger currentIndex = obj.integerValue;
        [self.cachedCells.allCachedCells
            enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                         BOOL *_Nonnull stop) {
              if (obj.updateToPath > currentIndex) {
                [subs replaceObjectAtIndex:idx withObject:@(subs[idx].integerValue + 1)];
              }
            }];
      }];
  [self.cachedCells.allCachedCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        obj.updateToPath = obj.updateToPath - subs[idx].integerValue;
      }];
  [self.cachedCells markCellInfoDirty];
}

#pragma mark updates
- (void)updateCellAtIndex:(NSInteger)updateFrom toIndex:(NSInteger)updateTo {
  id<LynxListCell> cell = [self.cachedCells cellAtIndex:updateFrom];
  if (nil != cell) {
    cell = [_dataSource listView:self updateCell:cell toItemAtIndex:updateTo];
    [self invalidLayoutForCellIfNecessaryAndSync:cell];
    [cell applyLayoutModel:[self.layout attributesFromIndex:cell.updateToPath]];
  }
  [self adjustWithBoundsChange];
}

- (void)handleUpdates {
  [[self layout]
      layoutFrom:[self layout].firstInvalidIndex
              to:MAX(self.cachedCells.lastIndexInPathOrder, [self layout].firstInvalidIndex) +
                 self.preloadBufferCount + self.numberOfColumns + 1];
  self.contentSize = [[self layout] getContentSize];
  [self.cachedCells.allCachedCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        LynxListLayoutModelLight *model = [[self layout] attributesFromIndex:obj.updateToPath];
        if ([self isSticky:obj.updateToPath]) {
          obj.frame = (CGRect){obj.frame.origin, model.frame.size};
        } else {
          [obj applyLayoutModel:model];
        }
      }];
}

- (void)handleUpdatesAlignments:(LynxListViewSelfSizingAlignment)alignmentOption
                     alignIndex:(NSInteger)index
                        updates:(NSDictionary<NSNumber *, NSValue *> *)updates {
  LynxListLayoutModelLight *firstModel = [[self layout] attributesFromIndex:index];
  CGFloat prevDistance = 0.f;
  switch (alignmentOption) {
    case LynxListViewSelfSizingAlignmentTop: {
      prevDistance = [self orientationTopOrigin:firstModel.frame] - [self orientationOffset];
      [[self layout] updateModels:updates];
      [self handleUpdates];
      firstModel = [[self layout] attributesFromIndex:index];
      self.contentOffset = [self
          orientationPointConverter:[self orientationTopOrigin:firstModel.frame] - prevDistance];
    } break;
    case LynxListViewSelfSizingAlignmentBottom: {
      prevDistance = [self orientationBottomOfRect:firstModel.frame] - [self orientationOffset];
      [[self layout] updateModels:updates];
      [self handleUpdates];
      firstModel = [[self layout] attributesFromIndex:index];
      self.contentOffset = [self
          orientationPointConverter:[self orientationBottomOfRect:firstModel.frame] - prevDistance];
    } break;
    default:
      break;
  }
}

#pragma mark anchor
- (void)adjustContentOffsetBasedOnPolicy:(LynxEdgeDistance)distance
                                  anchor:(id<LynxListCell>)anchor {
  // No anchor found, reset to the begin/end
  if (!anchor) {
    LYNX_LIST_DEBUG_LOG(@"adjustContentOffsetBasedOnPolicy can't find anchor and return to edge");
    if (self.anchorPriorityFromBegin) {
      CGPoint targetOffset = CGPointMake(-self.contentInset.left, -self.contentInset.top);
      LYNX_LIST_DEBUG_LOG(@"adjust to edge:%@", NSStringFromCGPoint(targetOffset));
      self.contentOffset = targetOffset;
      return;
    } else {
      CGPoint targetOffset =
          [self orientationPointConverter:[self orientationMaxScrollableDistance]];
      LYNX_LIST_DEBUG_LOG(@"adjust to edge:%@", NSStringFromCGPoint(targetOffset));
      self.contentOffset = targetOffset;
      return;
    }
  }

  CGPoint idealContentOffset = [self idealTargetContentOffset:distance anchor:anchor];
  // Force at least once adjustment.
  if (CGPointEqualToPoint(idealContentOffset, self.contentOffset)) {
    [self adjustWithBoundsChange];
  }
  // Keep adjust contentOffset if self sizing change anchor's offset.
  while (ABS(idealContentOffset.y - self.contentOffset.y) > 0.01 ||
         ABS(idealContentOffset.x - self.contentOffset.x) > 0.01) {
    self.contentOffset = idealContentOffset;
    idealContentOffset = [self idealTargetContentOffset:distance anchor:anchor];
  }
}

- (CGPoint)idealTargetContentOffset:(LynxEdgeDistance)distance anchor:(id<LynxListCell>)anchor {
  CGPoint targetContentOffset;
  LynxListLayoutModelLight *model = [self.layout attributesFromIndex:anchor.updateToPath];
  if (self.anchorPriorityFromBegin) {
    if (self.anchorVisibility == LynxAnchorVisibilityShow) {
      targetContentOffset = model.frame.origin;
    } else if (self.anchorVisibility == LynxAnchorVisibilityHide) {
      targetContentOffset =
          [self orientationPointConverter:[self orientationBottomOfRect:model.frame]];
    } else {
      if (self.anchorAlignToBottom) {
        targetContentOffset =
            [self orientationPointConverter:[self orientationBottomOfRect:model.frame] -
                                            distance.bottomToTop];
      } else {
        targetContentOffset = [self
            orientationPointConverter:[self orientationTopOrigin:model.frame] - distance.topToTop];
      }
    }

  } else {
    if (self.anchorVisibility == LynxAnchorVisibilityShow) {
      targetContentOffset =
          [self orientationPointConverter:MAX([self orientationBottomOfRect:model.frame] -
                                                  [self orientationMainSize:self.bounds],
                                              0)];
    } else if (self.anchorVisibility == LynxAnchorVisibilityHide) {
      targetContentOffset =
          [self orientationPointConverter:MAX([self orientationTopOrigin:model.frame] -
                                                  [self orientationMainSize:self.bounds],
                                              0)];
    } else {
      if (self.anchorAlignToBottom) {
        targetContentOffset =
            [self orientationPointConverter:MAX([self orientationBottomOfRect:model.frame] -
                                                    distance.bottomToBottom -
                                                    [self orientationMainSize:self.bounds],
                                                0)];
      } else {
        targetContentOffset =
            [self orientationPointConverter:MAX([self orientationTopOrigin:model.frame] -
                                                    distance.topToBottom -
                                                    [self orientationMainSize:self.bounds],
                                                0)];
      }
    }
  }
  return targetContentOffset;
}

#pragma mark sticky
- (void)offloadStickyTop {
  __block CGFloat stickyOffset = 0;
  [self.stickyTopItems
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger stickyIndex = obj.integerValue;
        LynxListLayoutModelLight *model = [self.layout attributesFromIndex:stickyIndex];
        id<LynxListCell> cell = [self.cachedCells cellAtIndex:stickyIndex];
        if ([self orientationBottomOfRect:model.frame] >=
            MAX(0, [self orientationOffset]) + stickyOffset) {
          cell.stickyPosition = 0;
          cell.isInStickyStatus = NO;
        } else {
          stickyOffset += [self orientationMainSize:model.frame];
        }
      }];
}

/*
 Adjust all sticky cells in displayingCells to the sticky position.
 Support multi-sticky.
 */
- (void)handleSticky {
  NSInteger firstStickyIndexInArray = [self findfirstStickyIndex];
  if (firstStickyIndexInArray >= 0) {
    NSInteger firstStickyIndex =
        [_stickyTopItems objectAtIndex:firstStickyIndexInArray].integerValue;
    // Find the next sticky item (in ordered sticky array). It may effects the first sticky cell.
    NSInteger nextStickyItem =
        firstStickyIndexInArray < (NSInteger)_stickyTopItems.count - 1
            ? [_stickyTopItems objectAtIndex:firstStickyIndexInArray + 1].integerValue
            : -1;
    LynxListLayoutModelLight *firstStickyItemLayoutModel =
        [[self layout] attributesFromIndex:firstStickyIndex];
    // The firstSticky cell should always show on screen
    id<LynxListCell> firstStickyCell = [self.cachedCells cellAtIndex:firstStickyIndex];
    if (!firstStickyCell) {
      firstStickyCell = [self loadNewCellAtIndex:firstStickyIndex];
      [self invalidLayoutForCellIfNecessaryAndSync:firstStickyCell];
      [self.cachedCells addCell:firstStickyCell inArray:self.cachedCells.displayingCells];
      firstStickyItemLayoutModel = [[self layout] attributesFromIndex:firstStickyIndex];
    }
    // Set zPosition so the sticky cell will always show on top.
    ((UIView *)firstStickyCell).layer.zPosition = NSIntegerMax;
    // Smooth transition between the first sticky node and the second sticky node.
    CGFloat delta = 0;
    if (nextStickyItem >= 0) {
      LynxListLayoutModelLight *nextStickyItemLayoutModel =
          [[self layout] attributesFromIndex:nextStickyItem];
      if ([self orientationMainSize:firstStickyItemLayoutModel.frame] + [self orientationOffset] >
          [self orientationTopOrigin:nextStickyItemLayoutModel.frame]) {
        delta = [self orientationMainSize:firstStickyItemLayoutModel.frame] +
                [self orientationOffset] -
                [self orientationTopOrigin:nextStickyItemLayoutModel.frame];
      }
    }
    CGFloat targetPosition = MAX(0, [self orientationOffset] - delta);
    CGPoint targetOrigin =
        [self layout].isVerticalLayout
            ? CGPointMake(firstStickyItemLayoutModel.frame.origin.x, targetPosition)
            : CGPointMake(targetPosition, firstStickyItemLayoutModel.frame.origin.y);
    firstStickyCell.frame = (CGRect){targetOrigin, firstStickyItemLayoutModel.frame.size};
    firstStickyCell.isInStickyStatus = YES;
  }
  [_stickyBottomItems enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx,
                                                   BOOL *_Nonnull stop) {
    NSInteger stickyIndex = obj.integerValue;
    LynxListLayoutModelLight *firstStickyBottomLayoutModel =
        [self.layout attributesFromIndex:stickyIndex];
    id<LynxListCell> firstStickyBottomCell = [self.cachedCells cellAtIndex:stickyIndex];
    // Need to adjust frame for sticky
    if ([self orientationBottomOfRect:firstStickyBottomLayoutModel.frame] >
        [self orientationBottomOfRect:self.bounds]) {
      // If sticky-bottom cell is not created, create a new one.
      if (!firstStickyBottomCell) {
        firstStickyBottomCell = [self loadNewCellAtIndex:stickyIndex];
        [self invalidLayoutForCellIfNecessaryAndSync:firstStickyBottomCell];
        [self.cachedCells addCell:firstStickyBottomCell inArray:self.cachedCells.displayingCells];
        firstStickyBottomLayoutModel = [self.layout attributesFromIndex:stickyIndex];
      }
      // Set zPosition so the sticky cell will always show on top.
      ((UIView *)firstStickyBottomCell).layer.zPosition = NSIntegerMax;
      CGFloat targetPosition =
          MIN([self orientationOffset], [self orientationMaxScrollableDistance]) +
          [self orientationMainSize:self.bounds] -
          [self orientationMainSize:firstStickyBottomLayoutModel.frame];
      // Find previous sticky-bottom cell
      if (idx > 0) {
        NSInteger previousStickyBottomIndex =
            [_stickyBottomItems objectAtIndex:idx - 1].integerValue;
        LynxListLayoutModelLight *previousStickyBottomLayoutModel =
            [[self layout] attributesFromIndex:previousStickyBottomIndex];
        targetPosition = MAX([self orientationBottomOfRect:previousStickyBottomLayoutModel.frame],
                             targetPosition);
      }
      CGPoint targetOrigin =
          [self layout].isVerticalLayout
              ? CGPointMake(firstStickyBottomLayoutModel.frame.origin.x, targetPosition)
              : CGPointMake(targetPosition, firstStickyBottomLayoutModel.frame.origin.y);
      firstStickyBottomCell.frame = (CGRect){targetOrigin, firstStickyBottomLayoutModel.frame.size};
      firstStickyBottomCell.isInStickyStatus = YES;
      *stop = YES;
    } else {
      firstStickyBottomCell.isInStickyStatus = NO;
      firstStickyBottomCell.frame = firstStickyBottomLayoutModel.frame;
    }
  }];
}

- (NSInteger)findfirstStickyIndex {
  __block NSInteger firstStickyIndexInArray = -1;
  [_stickyTopItems
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger index = obj.integerValue;
        if (index >= 0 && index < [[self layout] getCount]) {
          LynxListLayoutModelLight *model = [[self layout] attributesFromIndex:index];
          id<LynxListCell> cell = [self.cachedCells cellAtIndex:index];
          if ([self orientationTopOrigin:model.frame] <= MAX(0, [self orientationOffset]) &&
              [self orientationOffset] >= 0) {
            firstStickyIndexInArray = MAX(firstStickyIndexInArray, (NSInteger)idx);
          } else {
            cell.isInStickyStatus = NO;
            cell.frame = model.frame;
          }
        }
      }];
  return firstStickyIndexInArray;
}

- (BOOL)isSticky:(NSInteger)index {
  return [self.stickyTopItems containsObject:@(index)] ||
         [self.stickyBottomItems containsObject:@(index)];
}

#pragma mark scroll
- (CGPoint)targetContentOffsetAtIndex:(NSInteger)targetIndex
                               offset:(CGFloat)offset
                               sticky:(BOOL)sticky
                       scrollPosition:(NSString *)scrollPosition {
  LynxListLayoutModelLight *model = [self.layout attributesFromIndex:targetIndex];
  LynxListViewScrollPosition convertedScrollPosition =
      [self convertToScrollPosition:scrollPosition];
  CGPoint targetContentOffset;
  switch (convertedScrollPosition) {
    case LynxListViewScrollPositionTop:
      targetContentOffset =
          [self orientationPointConverter:[self orientationTopOrigin:model.frame] + offset];
      break;
    case LynxListViewScrollPositionBottom:
      targetContentOffset =
          [self orientationPointConverter:[self orientationBottomOfRect:model.frame] -
                                          [self orientationMainSize:self.bounds] + offset];
      break;
    case LynxListViewScrollPositionMiddle:
      targetContentOffset =
          [self orientationPointConverter:[self orientationTopOrigin:model.frame] +
                                          0.5 * [self orientationMainSize:model.frame] -
                                          0.5 * [self orientationMainSize:self.bounds] + offset];
      break;
    default:
      break;
  }
  return targetContentOffset;
}

- (LynxListViewScrollPosition)convertToScrollPosition:(NSString *)position {
  if ([position isEqualToString:@"bottom"]) {
    return LynxListViewScrollPositionBottom;
  }
  if ([position isEqualToString:@"top"]) {
    return LynxListViewScrollPositionTop;
  }
  if ([position isEqualToString:@"middle"]) {
    return LynxListViewScrollPositionMiddle;
  }
  return LynxListViewScrollPositionTop;
}

- (id<LynxListCell>)visibleCellAtPoint:(CGPoint)point {
  __block id<LynxListCell> target;
  [self.cachedCells.displayingCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        CGPoint pointInLayer = [self
            convertPoint:CGPointMake(point.x + self.contentOffset.x, point.y + self.contentOffset.y)
                  toView:obj.contentView];
        BOOL contains = [obj.contentView.layer containsPoint:pointInLayer];
        if (contains) {
          target = obj;
          *stop = YES;
        }
      }];
  return target;
}

#pragma mark layout
- (void)invalidLayoutForCellIfNecessaryAndSync:(id<LynxListCell>)cell {
  if (self.isAsync) {
    [cell applyLayoutModel:[[self layout] attributesFromIndex:cell.updateToPath]];
    return;
  }
  [self invalidLayoutForCellIfNecessary:cell];
}

- (void)invalidLayoutForCellIfNecessary:(id<LynxListCell>)cell {
  if (nil == cell) {
    return;
  }
  LynxListLayoutModelLight *model = [[self layout] attributesFromIndex:cell.updateToPath];
  if (model.frame.size.height != cell.frame.size.height ||
      model.frame.size.width != cell.frame.size.width) {
    [self dispatchInvalidationContext:[[LynxUIListInvalidationContext alloc] initWithModelUpdates:@{
            @(cell.updateToPath) : [NSValue valueWithCGRect:cell.frame]
          }]];
  }
  [cell applyLayoutModel:[[self layout] attributesFromIndex:cell.updateToPath]];
}

// Update every cell in caches to latest layoutModel.
- (void)invalidLayoutFromIndex:(NSInteger)index {
  [self.cachedCells.allCachedCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        if (obj.updateToPath >= index) {
          [obj applyLayoutModel:[[self layout] attributesFromIndex:obj.updateToPath]];
        }
      }];
}

#pragma mark orientation

- (CGFloat)orientationMainSize:(CGRect)rect {
  return [[self layout] isVerticalLayout] ? rect.size.height : rect.size.width;
}

- (CGFloat)orientationTopOrigin:(CGRect)rect {
  return [[self layout] isVerticalLayout] ? rect.origin.y : rect.origin.x;
}

- (CGFloat)orientationOffset {
  return [self layout].isVerticalLayout ? self.contentOffset.y : self.contentOffset.x;
}

- (CGFloat)orientationBottomOfRect:(CGRect)rect {
  return [self layout].isVerticalLayout ? CGRectGetMaxY(rect) : CGRectGetMaxX(rect);
}

- (CGFloat)orientationMaxScrollableDistance {
  return [self layout].isVerticalLayout
             ? self.contentSize.height - self.frame.size.height + self.contentInset.bottom
             : self.contentSize.width - self.frame.size.height + self.contentInset.right;
}

- (CGFloat)orientationMinScrollableDistance {
  return [self layout].isVerticalLayout ? -self.contentInset.top : -self.contentInset.left;
}

- (CGFloat)orientationContentSize {
  return [self layout].isVerticalLayout ? self.contentSize.height : self.contentSize.width;
}

// Convert a position to correct target offset. Considering the max scrollable distance and list
// orientation.
- (CGPoint)orientationPointConverter:(CGFloat)position {
  CGFloat adjustedPosition = MIN([self orientationMaxScrollableDistance], position);
  adjustedPosition = MAX([self orientationMinScrollableDistance], adjustedPosition);
  return [[self layout] isVerticalLayout] ? CGPointMake(self.contentOffset.x, adjustedPosition)
                                          : CGPointMake(adjustedPosition, self.contentOffset.y);
}

#pragma mark hittest
- (id<LynxEventTarget>)findHitTestTarget:(CGPoint)point withEvent:(UIEvent *)event {
  // if the zIndex of cells are assigned according to their index
  // we then use containsPoints to test each cell form the max zIndex to the min zIndex.
  NSArray<id<LynxListCell>> *visibleCells = self.cachedCells.displayingCells;
  NSArray<id<LynxListCell>> *visibleCellsSortedByZIndexReversely =
      [visibleCells sortedArrayUsingComparator:^NSComparisonResult(
                        id<LynxListCell> _Nonnull cellA, id<LynxListCell> _Nonnull cellB) {
        NSInteger ZPositionA = ((UIView *)cellA).layer.zPosition;
        NSInteger ZPositionB = ((UIView *)cellB).layer.zPosition;
        if (ZPositionA < ZPositionB) {
          return NSOrderedDescending;
        } else {
          return NSOrderedAscending;
        }
        return NSOrderedSame;
      }];

  __block id<LynxEventTarget> hitTarget;
  [visibleCellsSortedByZIndexReversely
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        if ([obj isKindOfClass:LynxListViewCellLight.class]) {
          LynxListViewCellLightLynxUI *cell = (LynxListViewCellLightLynxUI *)obj;
          CGPoint pointInCell = [cell.ui.view convertPoint:point fromView:self];
          if ([cell.ui containsPoint:pointInCell inHitTestFrame:cell.contentView.bounds]) {
            hitTarget = [cell.ui hitTest:pointInCell withEvent:event];
            if (hitTarget) {
              *stop = YES;
            }
          }
        }
      }];
  return hitTarget;
}

#pragma mark helper
- (void)setContentOffset:(CGPoint)contentOffset {
  LYNX_LIST_DEBUG_LOG(@"setContentOffset: original offset:%@, target offset: %@",
                      NSStringFromCGPoint(self.contentOffset), NSStringFromCGPoint(contentOffset));
  [super setContentOffset:contentOffset];
}

- (LynxAnchorPolicies)makeAnchorPolicies {
  LynxAnchorPolicies anchorPolicies = {
      self.anchorPriorityFromBegin,
      self.deleteRegressPolicyToTop,
      self.insertAnchorModeInside,
  };
  return anchorPolicies;
}

- (LynxEdgeDistance)makeAnchorDistance:(id<LynxListCell>)anchorCell {
  LynxEdgeDistance anchorDistance = {
      [self orientationTopOrigin:anchorCell.frame] - [self orientationTopOrigin:self.bounds],
      [self orientationTopOrigin:anchorCell.frame] - [self orientationBottomOfRect:self.bounds],
      [self orientationBottomOfRect:anchorCell.frame] - [self orientationTopOrigin:self.bounds],
      [self orientationBottomOfRect:anchorCell.frame] - [self orientationBottomOfRect:self.bounds]};
  return anchorDistance;
}

- (BOOL)isBouncing {
  if ([self orientationBottomOfRect:self.frame] > [self orientationContentSize]) {
    return NO;
  }
  if ([self orientationOffset] > [self orientationMaxScrollableDistance] ||
      [self orientationOffset] < [self orientationMinScrollableDistance]) {
    return YES;
  }
  return NO;
}

- (LynxCellPosition)cellPosition:(LynxListLayoutModelLight *)model {
  if (model.frame.size.height * model.frame.size.width == 0) {
    return LynxCellPositionNone;
  }
  if ([self orientationTopOrigin:model.frame] > [self orientationBottomOfRect:self.bounds]) {
    return LynxCellPositionBelowBounds;
  }
  if ([self orientationBottomOfRect:model.frame] < [self orientationTopOrigin:self.bounds]) {
    return LynxCellPositionAboveBounds;
  }
  return LynxCellPositionInBounds;
}

#pragma mark event protocol
- (NSInteger)totalItemsCount {
  return [[self layout] getCount];
}

- (NSArray *)attachedCells {
  if (!self.needsVisibleCells) {
    return nil;
  }
  NSMutableArray *attachedCells = [[NSMutableArray alloc] init];
  [self.cachedCells.displayingCells
      enumerateObjectsUsingBlock:^(id<LynxListCell> _Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        CGFloat cellTop = obj.frame.origin.y - self.contentOffset.y;
        CGFloat cellLeft = obj.frame.origin.x - self.contentOffset.x;
        NSString *cellID;
        if ([obj isKindOfClass:LynxListViewCellLightLynxUI.class]) {
          cellID = ((LynxUI *)((LynxListViewCellLightLynxUI *)obj).ui).idSelector;
        }
        NSDictionary *cellMsg = @{
          @"id" : cellID ?: @"",
          @"position" : @(obj.updateToPath),
          @"top" : @(cellTop),
          @"bottom" : @(cellTop + obj.frame.size.height),
          @"left" : @(cellLeft),
          @"right" : @(cellLeft + obj.frame.size.width),
        };
        [attachedCells addObject:cellMsg];
      }];
  [attachedCells sortUsingComparator:^NSComparisonResult(id _Nonnull lhs, id _Nonnull rhs) {
    NSDictionary *lhsCellMsg = (NSDictionary *)lhs;
    NSDictionary *rhsCellMsg = (NSDictionary *)rhs;
    NSInteger lhsPosition = [lhsCellMsg[@"position"] integerValue];
    NSInteger rhsPosition = [rhsCellMsg[@"position"] integerValue];

    if (lhsPosition < rhsPosition) {
      return NSOrderedAscending;
    }

    if (lhsPosition > rhsPosition) {
      return NSOrderedDescending;
    }

    return NSOrderedSame;
  }];
  return attachedCells;
}

- (NSArray<id<LynxListCell>> *)visibleCells {
  return self.cachedCells.displayingCells;
}
@end
