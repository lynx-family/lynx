// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIListLoader.h"
#include <numeric>
#import "LynxTemplateRender+Internal.h"
#import "LynxUI.h"
#import "LynxUIComponent.h"
#import "LynxView+Internal.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/shell/lynx_shell.h"

static const CGFloat kLynxUIListLoaderScreenWidthInRpx = 750.;

#pragma mark - LynxCollectionDiffResult

@implementation LynxUIListDiffResult

- (instancetype)initWithNumberOfInsertions:(NSUInteger)count {
  auto diffResult = lynx::tasm::myers_diff::DiffResult{};
  auto &insertions = diffResult.insertions_;
  insertions.resize<false>(count);
  std::iota(insertions.begin(), insertions.end(), 0);
  self = [self initWithDiffResult:diffResult];
  return self;
}

- (instancetype)initWithDictionary:(NSDictionary *)diffResult {
  if (self = [super init]) {
    NSArray *insertions = diffResult[@"insertions"];
    NSMutableArray<NSIndexPath *> *insertPaths = [[NSMutableArray alloc] init];
    for (NSNumber *indexPath in insertions) {
      [insertPaths addObject:[NSIndexPath indexPathForItem:indexPath.unsignedIntegerValue
                                                 inSection:0]];
    }
    _insertPaths = insertPaths;

    NSArray *removals = diffResult[@"removals"];
    NSMutableArray<NSIndexPath *> *removePaths = [[NSMutableArray alloc] init];
    for (NSNumber *indexPath in removals) {
      [removePaths addObject:[NSIndexPath indexPathForItem:indexPath.unsignedIntegerValue
                                                 inSection:0]];
    }
    _removePaths = removePaths;

    NSArray *update_from = diffResult[@"updateFrom"];
    NSMutableArray<NSIndexPath *> *updateFromPaths = [[NSMutableArray alloc] init];
    for (NSNumber *indexPath in update_from) {
      [updateFromPaths addObject:[NSIndexPath indexPathForItem:indexPath.unsignedIntegerValue
                                                     inSection:0]];
    }
    _updateFromPaths = updateFromPaths;

    NSArray *update_to = diffResult[@"updateTo"];
    NSMutableArray<NSIndexPath *> *updateToPaths = [[NSMutableArray alloc] init];
    for (NSNumber *indexPath in update_to) {
      [updateToPaths addObject:[NSIndexPath indexPathForItem:indexPath.unsignedIntegerValue
                                                   inSection:0]];
    }
    _updateToPaths = updateToPaths;

    NSArray *move_from = diffResult[@"moveFrom"];
    NSMutableArray<NSIndexPath *> *moveFromPaths = [[NSMutableArray alloc] init];
    for (NSNumber *indexPath in move_from) {
      [moveFromPaths addObject:[NSIndexPath indexPathForItem:indexPath.unsignedIntegerValue
                                                   inSection:0]];
    }
    _moveFromPaths = moveFromPaths;

    NSArray *move_to = diffResult[@"moveTo"];
    NSMutableArray<NSIndexPath *> *moveToPaths = [[NSMutableArray alloc] init];
    for (NSNumber *indexPath in move_to) {
      [moveToPaths addObject:[NSIndexPath indexPathForItem:indexPath.unsignedIntegerValue
                                                 inSection:0]];
    }
    _moveToPaths = moveToPaths;

    _empty = ([removePaths count] + [insertPaths count] + [updateFromPaths count] +
              [moveFromPaths count]) == 0;
  }
  return self;
}

- (instancetype)initWithDiffResult:(const lynx::tasm::myers_diff::DiffResult &)diffResult {
  self = [super init];
  if (self) {
    NSMutableArray *removePaths = [[NSMutableArray alloc] init];
    NSMutableArray *insertPaths = [[NSMutableArray alloc] init];
    NSMutableArray *moveFromPaths = [[NSMutableArray alloc] init];
    NSMutableArray *moveToPaths = [[NSMutableArray alloc] init];
    NSMutableArray *updateFromPaths = [[NSMutableArray alloc] init];
    NSMutableArray *updateToPaths = [[NSMutableArray alloc] init];

    auto fill = [](const auto &from, auto &to) {
      for (auto op_idx : from) {
        [to addObject:[NSIndexPath indexPathForRow:op_idx inSection:0]];
      }
    };

    fill(diffResult.removals_, removePaths);
    fill(diffResult.insertions_, insertPaths);
    fill(diffResult.move_from_, moveFromPaths);
    fill(diffResult.move_to_, moveToPaths);
    fill(diffResult.update_from_, updateFromPaths);
    fill(diffResult.update_to_, updateToPaths);

    _removePaths = removePaths;
    _insertPaths = insertPaths;
    _updateFromPaths = updateFromPaths;
    _updateToPaths = updateToPaths;
    _moveFromPaths = moveFromPaths;
    _moveToPaths = moveToPaths;
    _empty = ([removePaths count] + [insertPaths count] + [updateFromPaths count] +
              [moveFromPaths count]) == 0;
  }
  return self;
}

- (instancetype)initWithUpdateFromPath:(NSArray *)updateFromPath
                          updateToPath:(NSArray *)updateToPath
                            removePath:(NSArray *)removePath
                            insertPath:(NSArray *)insertPath
                          moveFromPath:(NSArray *)moveFromPath
                            moveToPath:(NSArray *)moveToPath {
  if (self = [super init]) {
    _updateFromPaths = updateFromPath;
    _updateToPaths = updateToPath;
    _removePaths = removePath;
    _insertPaths = insertPath;
    _moveFromPaths = moveFromPath;
    _moveToPaths = moveToPath;
    _empty = ([removePath count] + [insertPath count] + [updateFromPath count] +
              [moveFromPath count]) == 0;
  }
  return self;
}

#if defined(LYNX_LIST_DEBUG)
- (NSString *)description {
  NSMutableString *removals = [[NSMutableString alloc] init];
  if ([_removePaths count]) {
    [removals appendString:@"REMOVE: "];
    for (NSIndexPath *remove in _removePaths) {
      [removals appendFormat:@"%@ ", @(remove.row)];
    }
  }

  NSMutableString *insertions = [[NSMutableString alloc] init];
  if ([_insertPaths count]) {
    [insertions appendString:@"INSERTION: "];
    for (NSIndexPath *insert in _insertPaths) {
      [insertions appendFormat:@"%@ ", @(insert.row)];
    }
  }

  NSMutableString *updatePairs = [[NSMutableString alloc] init];
  if ([_updateFromPaths count]) {
    [updatePairs appendString:@"UPDATE: "];
    for (NSUInteger i = 0; i < [_updateFromPaths count]; ++i) {
      [updatePairs
          appendFormat:@"(%@ -> %@)", @(_updateFromPaths[i].row), @(_updateToPaths[i].row)];
    }
  }

  NSMutableString *movePairs = [[NSMutableString alloc] init];
  if ([_moveFromPaths count]) {
    [movePairs appendString:@"MOVE: "];
    for (NSUInteger i = 0; i < [_moveFromPaths count]; ++i) {
      [updatePairs appendFormat:@"(%@ -> %@)", @(_moveFromPaths[i].row), @(_moveToPaths[i].row)];
    }
  }
  return [NSString stringWithFormat:@"%@ %@ %@ %@", removals, insertions, updatePairs, movePairs];
}
#endif

@end

#pragma mark - LynxUIListLoader

@interface LynxUIListLoader ()
@property(nonatomic, readonly, nullable) lynx::tasm::ListNode *listNode;
@property(nonatomic, nullable) LynxUI *currentUI;
@end

@implementation LynxUIListLoader

#pragma mark - Public

- (id)init {
  if (self = [super init]) {
    _newArch = NO;
  }
  return self;
}

- (void)markIsNewArch {
  _newArch = YES;
}

- (LynxUI *)renderLynxUIAtIndexPath:(NSIndexPath *)indexPath {
  [self validateIndexPath:indexPath];
  uint32_t nativeIndex = static_cast<uint32_t>(indexPath.row);
  auto *listNode = self.listNode;
  if (!listNode) {
    return nil;
  }
  listNode->RenderComponentAtIndex(nativeIndex);
  LynxUI *lynxUI = _currentUI;
  _currentUI = nil;
  return lynxUI;
}

- (void)updateLynxUI:(LynxUI *)lynxUI toIndexPath:(NSIndexPath *)indexPath {
  [self validateLynxUI:lynxUI indexPath:indexPath];
  uint32_t nativeIndex = static_cast<uint32_t>(indexPath.row);
  uint32_t uiSign = static_cast<uint32_t>(lynxUI.sign);
  auto *listNode = self.listNode;
  if (!listNode) {
    return;
  }
  listNode->UpdateComponent(uiSign, nativeIndex);
}

#pragma mark - Public, New List Arch APIs

- (LynxUI *)uiAtIndexPath:(NSIndexPath *)indexPath {
  auto *listNode = self.listNode;
  if (!listNode) {
    return nil;
  }
  int32_t uiSign = listNode->ComponentAtIndex(static_cast<int32_t>(indexPath.row), 0,
                                              self.needsInternalCellPrepareForReuseNotification);
  LynxUI *ui = [self.context.uiOwner findUIBySign:uiSign];
  return ui;
}

- (void)asyncUIAtIndexPath:(NSIndexPath *)indexPath operationID:(int64_t)operationID {
  auto shellPtr = super.context.shellPtr;
  if (shellPtr) {
    reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->LoadListNode(
        static_cast<int32_t>(self.sign), static_cast<int32_t>(indexPath.row), operationID,
        self.needsInternalCellPrepareForReuseNotification);
  }
}

- (void)recycleLynxUI:(LynxUI *)ui {
  auto *listNode = self.listNode;
  if (!listNode) {
    return;
  }
  listNode->EnqueueComponent(static_cast<int32_t>(ui.sign));
}

- (void)asyncRecycleLynxUI:(LynxUI *)ui {
  auto shellPtr = super.context.shellPtr;
  if (shellPtr) {
    reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->EnqueueListNode(
        static_cast<int32_t>(self.sign), static_cast<int32_t>(ui.sign));
  }
}

#pragma mark - Private, LynxUI Validation

- (void)validateLynxUI:(LynxUI *)lynxUI indexPath:(NSIndexPath *)indexPath {
  if (lynxUI.parent != self) {
    [NSException raise:@"LynxUIListLoaderException"
                format:@"LynxUIListLoader received an LynxUI that was not created by "
                       @"LynxUIListLoader(sign: %@)",
                       @(self.sign)];
  }
  [self validateIndexPath:indexPath];
}

- (void)validateIndexPath:(NSIndexPath *)indexPath {
  if (!((NSUInteger)indexPath.row < self.count)) {
    [NSException raise:@"LynxUIListLoaderException"
                format:@"LynxUIListLoader received an indexPath %@ which exceeds the number of "
                       @"cells loaded: %@",
                       indexPath, @(self.count)];
  }
}

#pragma mark - Private, getters

- (lynx::tasm::ListNode *)listNode {
  auto shellPtr = super.context.shellPtr;
  if (shellPtr) {
    return reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->GetListNode(
        static_cast<int32_t>(self.sign));
  }
  return nullptr;
}

// return true,when the threadStrategy is not ALL_ON_UI and the swith of "enableAsyncList" is on
- (BOOL)isAsync {
  auto shellPtr = super.context.shellPtr;
  if (shellPtr) {
    int threadStrategy = reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->ThreadStrategy();
    return threadStrategy != LynxThreadStrategyForRenderAllOnUI &&
           threadStrategy != LynxThreadStrategyForRenderPartOnLayout;
  } else {
    // shellPtr has been destoryed, but LynxView is not.
    // If shellPtr has been released, the ret value will not be consumed, we just return NO here.
    return NO;
  }
}

// return true,when the threadStrategy is PartOnLayout
- (BOOL)isPartOnLayout {
  auto shellPtr = super.context.shellPtr;
  if (shellPtr) {
    int threadStrategy = reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->ThreadStrategy();
    return threadStrategy == LynxThreadStrategyForRenderPartOnLayout;
  } else {
    // shellPtr has been destoryed, but LynxView is not.
    // If shellPtr has been released, the ret value will not be consumed, we just return NO here.
    return NO;
  }
}

- (NSUInteger)count {
  return [_reuseIdentifiers count];
}

#pragma mark - Private, Override LynxUI

- (void)onComponentLayoutUpdated:(nonnull LynxUIComponent *)component {
}

- (void)onAsyncComponentLayoutUpdated:(nonnull LynxUIComponent *)component
                          operationID:(int64_t)operationID {
}

- (void)insertChild:(LynxUI *)child atIndex:(NSInteger)index {
  if (child != nil) {
    child.parent = self;
    if ((NSUInteger)index > self.children.count) {
      [self.children addObject:child];
    } else {
      [self.children insertObject:child atIndex:index];
    }
  }
  LynxUIComponent *componentChild = (LynxUIComponent *)child;
  componentChild.layoutObserver = self;
  _currentUI = child;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  if (!self.isNewArch) {
    [self loadCellInfos];
  }
}

- (void)loadCellInfos {
  auto *listNode = self.listNode;
  if (!listNode) {
    return;
  }

  // a set for current item-keys
  const auto &itemKeysRef = listNode->item_keys();
  NSMutableArray<NSString *> *itemKeys = [[NSMutableArray alloc] init];
  for (std::size_t i = 0; i < itemKeysRef.size(); ++i) {
    auto &itemKey = itemKeysRef[i];
    NSString *itemKeyNSString = [NSString stringWithUTF8String:itemKey.data()];
    [itemKeys addObject:itemKeyNSString];
  }
  _currentItemKeys = itemKeys;

  // a mapping from data index to data type, for reuse component
  const auto &reuseIdentifiersRef = listNode->component_info();
  NSMutableArray<NSString *> *reuseIdentifiers = [[NSMutableArray alloc] init];
  for (std::size_t i = 0; i < reuseIdentifiersRef.size(); ++i) {
    auto &identifier = reuseIdentifiersRef[i];
    NSString *identifierNSString = [NSString stringWithUTF8String:identifier.data()];
    [reuseIdentifiers addObject:identifierNSString];
  }
  _reuseIdentifiers = reuseIdentifiers;

  const auto &estimatedHeightsRef = listNode->estimated_height();
  const auto &estimatedHeightsPxRef = listNode->estimated_height_px();
  bool enableEstimateHeight = std::any_of(estimatedHeightsRef.begin(), estimatedHeightsRef.end(),
                                          [](auto estimatedHeight) { return estimatedHeight > 0; });
  bool enableEstimateHeightPx =
      std::any_of(estimatedHeightsPxRef.begin(), estimatedHeightsPxRef.end(),
                  [](auto estimatedHeight) { return estimatedHeight > 0; });
  if (enableEstimateHeight || enableEstimateHeightPx) {
    // if any one of the estimatedHeight of cells are provided, we enable the estimatedHeight mode
    // estimatedHeight provided by props must be a number, indicating the height for corresponding
    // component, with unit as `rpx` the screen width is defined as `750rpx`, compute the scale of
    // the unit of current iOS device to `rpx`
    CGFloat scale = [UIScreen mainScreen].bounds.size.width / kLynxUIListLoaderScreenWidthInRpx;
    NSMutableDictionary<NSIndexPath *, NSNumber *> *estimatedHeights =
        [[NSMutableDictionary alloc] initWithCapacity:estimatedHeightsRef.size()];
    for (std::size_t i = 0; i < estimatedHeightsRef.size(); ++i) {
      auto estimatedHeightDouble =
          estimatedHeightsRef[i] > 0 ? estimatedHeightsRef[i] : estimatedHeightsPxRef[i];
      // apply the scale to the `rpx` to get the height in the current iOS device.
      NSNumber *estimatedHeightNumber = [NSNumber numberWithDouble:scale * estimatedHeightDouble];
      NSIndexPath *indexPath = [NSIndexPath indexPathForRow:i inSection:0];
      [estimatedHeights setObject:estimatedHeightNumber forKey:indexPath];
    }
    _estimatedHeights = estimatedHeights;
  } else {
    _estimatedHeights = nil;
  }

  // lamda for translate vector to NSArray
  NSMutableArray<NSIndexPath *> * (^translate)(const std::vector<uint32_t> &) =
      ^(const std::vector<uint32_t> &vec) {
        NSMutableArray<NSIndexPath *> *result =
            [[NSMutableArray alloc] initWithCapacity:vec.size()];
        for (auto nativeIndex : vec) {
          [result addObject:[NSIndexPath indexPathForRow:nativeIndex inSection:0]];
        }
        return result;
      };
  _fullSpanItems = translate(listNode->fullspan());
  _stickyTopItems = translate(listNode->sticky_top());
  _stickyBottomItems = translate(listNode->sticky_bottom());

  if (listNode->Diffable()) {
    _diffable = YES;
    _diffResult = [[LynxUIListDiffResult alloc] initWithDiffResult:listNode->DiffResult()];
    listNode->ClearDiffResult();
  } else {
    _diffable = NO;
    _diffResult = [[LynxUIListDiffResult alloc] initWithNumberOfInsertions:self.count];
  }

  _newArch = listNode->NewArch();
}

- (void)loadListInfo:(NSDictionary *)diffResult
          components:(NSDictionary<NSString *, NSMutableArray *> *)components {
  if (diffResult == nil) {
    _diffable = NO;
    return;
  }
  _diffable = ([diffResult count] > 0);

  NSMutableDictionary<NSIndexPath *, NSNumber *> *estimatedHeights =
      [[NSMutableDictionary alloc] init];
  NSMutableArray *fullSpan = [[NSMutableArray alloc] init];
  NSMutableArray *stickyTop = [[NSMutableArray alloc] init];
  NSMutableArray *stickyBottom = [[NSMutableArray alloc] init];

  _currentItemKeys = components[@"itemkeys"];

  for (NSUInteger i = 0; i < _currentItemKeys.count; i++) {
    NSIndexPath *index = [NSIndexPath indexPathForItem:i inSection:0];
    CGFloat estimatedHeight = [components[@"estimatedHeight"][i] doubleValue];
    CGFloat estimatedHeightPX = [components[@"estimatedHeightPx"][i] doubleValue];
    if (estimatedHeightPX > 0) {
      estimatedHeights[index] = @(estimatedHeightPX);
    } else if (estimatedHeight > 0) {
      estimatedHeights[index] = @(estimatedHeight);
    }
    if (i < components[@"fullspan"].count) {
      [fullSpan addObject:[NSIndexPath indexPathForItem:[components[@"fullspan"][i] integerValue]
                                              inSection:0]];
    }

    if (i < components[@"stickyTop"].count) {
      [stickyTop addObject:[NSIndexPath indexPathForItem:[components[@"stickyTop"][i] integerValue]
                                               inSection:0]];
    }

    if (i < components[@"stickyBottom"].count) {
      [stickyBottom
          addObject:[NSIndexPath indexPathForItem:[components[@"stickyBottom"][i] integerValue]
                                        inSection:0]];
    }
  }

  if (![_fullSpanItems isEqualToArray:fullSpan] || ![_stickyTopItems isEqualToArray:stickyTop] ||
      ![_stickyBottomItems isEqualToArray:stickyBottom]) {
    _elementTypeUpdate = YES;
  } else {
    _elementTypeUpdate = NO;
  }
  _fullSpanItems = fullSpan;
  _stickyTopItems = stickyTop;
  _stickyBottomItems = stickyBottom;
  _estimatedHeights = estimatedHeights.count ? estimatedHeights : nil;
  _reuseIdentifiers = components[@"viewTypes"];

  _diffResult = [[LynxUIListDiffResult alloc] initWithDictionary:diffResult];
}

- (void)updateListActionInfo:(NSDictionary *)noDiffResult {
  NSArray *updateAction = noDiffResult[@"updateAction"];
  NSArray *removeAction = noDiffResult[@"removeAction"];
  NSArray *insertAction = noDiffResult[@"insertAction"];

  if (updateAction == nil && removeAction == nil && insertAction == nil) {
    _diffable = NO;
    return;
  }
  _diffable = YES;

  // init
  if (_currentItemKeys == nil) {
    _currentItemKeys = [[NSMutableArray alloc] init];
  }

  if (_reuseIdentifiers == nil) {
    _reuseIdentifiers = [[NSMutableArray alloc] init];
  }

  if (_fullSpanItems == nil) {
    _fullSpanItems = [[NSMutableArray alloc] init];
  }
  if (_stickyTopItems == nil) {
    _stickyTopItems = [[NSMutableArray alloc] init];
  }
  if (_stickyBottomItems == nil) {
    _stickyBottomItems = [[NSMutableArray alloc] init];
  }
  if (_estimatedHeights == nil) {
    _estimatedHeights = [[NSMutableDictionary alloc] init];
  }

  if (_fiberFullSpanItems == nil) {
    _fiberFullSpanItems = [[NSMutableArray alloc] init];
  }

  if (_fiberStickyTopItems == nil) {
    _fiberStickyTopItems = [[NSMutableArray alloc] init];
  }

  if (_fiberStickyBottomItems == nil) {
    _fiberStickyBottomItems = [[NSMutableArray alloc] init];
  }

  NSMutableArray<NSIndexPath *> *updateFromPath = [[NSMutableArray alloc] init];
  NSMutableArray<NSIndexPath *> *updateToPath = [[NSMutableArray alloc] init];
  NSMutableArray<NSIndexPath *> *removePath = [[NSMutableArray alloc] init];
  NSMutableArray<NSIndexPath *> *insertPath = [[NSMutableArray alloc] init];
  NSMutableArray<NSIndexPath *> *moveFromPath = [[NSMutableArray alloc] init];
  NSMutableArray<NSIndexPath *> *moveToPath = [[NSMutableArray alloc] init];
  // remove list component according to "remove" data
  if (removeAction != nil) {
    int count = (int)removeAction.count;
    for (int i = count - 1; i >= 0; i--) {
      NSInteger position = [[removeAction objectAtIndex:i] integerValue];
      if (position < 0) {
        continue;
      }
      NSIndexPath *path = [NSIndexPath indexPathForItem:position inSection:0];
      [_currentItemKeys removeObjectAtIndex:position];
      [_reuseIdentifiers removeObjectAtIndex:position];
      [_fiberFullSpanItems removeObjectAtIndex:position];
      [_fiberStickyBottomItems removeObjectAtIndex:position];
      [_fiberStickyTopItems removeObjectAtIndex:position];
      [removePath addObject:path];
    }
  }
  // insert list component according to "insertAction" data
  if (insertAction != nil) {
    for (id component in insertAction) {
      if (component == nil) {
        continue;
      }
      NSInteger position = [component[@"position"] integerValue];
      NSString *itemKey = component[@"item-key"];
      NSString *reuseType = component[@"type"];
      bool isFullSpan = [component[@"full-span"] boolValue];
      bool isStickyTop = [component[@"sticky-top"] boolValue];
      bool isStickyBottom = [component[@"sticky-bottom"] boolValue];
      CGFloat estimatedHeightPx = [component[@"estimated-height-px"] doubleValue];
      NSIndexPath *path = [NSIndexPath indexPathForItem:position inSection:0];
      [_currentItemKeys insertObject:itemKey atIndex:position];
      [_reuseIdentifiers insertObject:reuseType atIndex:position];
      [_fiberFullSpanItems insertObject:[NSNumber numberWithBool:isFullSpan] atIndex:position];
      [_fiberStickyTopItems insertObject:[NSNumber numberWithBool:isStickyTop] atIndex:position];
      [_fiberStickyBottomItems insertObject:[NSNumber numberWithBool:isStickyBottom]
                                    atIndex:position];

      if (estimatedHeightPx > 0) {
        _estimatedHeights[path] = @(estimatedHeightPx);
      }
      [insertPath addObject:path];
    }
  }
  // update list component according to "updateAction" data
  if (updateAction != nil) {
    for (id component in updateAction) {
      if (component == nil) {
        continue;
      }
      NSInteger fromPos = [component[@"from"] integerValue];
      NSInteger toPos = [component[@"to"] integerValue];
      NSString *itemKey = component[@"item-key"];
      NSString *reuseType = component[@"type"];
      bool isFullSpan = [component[@"full-span"] boolValue];
      bool isStickyTop = [component[@"sticky-top"] boolValue];
      bool isStickyBottom = [component[@"sticky-bottom"] boolValue];
      bool isFlush = [component[@"flush"] boolValue];
      CGFloat estimatedHeightPx = [component[@"estimated-height-px"] doubleValue];
      NSIndexPath *fromPath = [NSIndexPath indexPathForItem:fromPos inSection:0];
      NSIndexPath *toPath = [NSIndexPath indexPathForItem:toPos inSection:0];

      if (isFullSpan) {
        [_fullSpanItems addObject:fromPath];
      } else {
        [_fullSpanItems removeObject:fromPath];
      }
      if (isStickyTop) {
        [_stickyTopItems addObject:fromPath];
      } else {
        [_stickyTopItems removeObject:fromPath];
      }
      if (isStickyBottom) {
        [_stickyBottomItems addObject:fromPath];
      } else {
        [_stickyBottomItems removeObject:fromPath];
      }
      [_currentItemKeys replaceObjectAtIndex:fromPos withObject:itemKey];
      [_reuseIdentifiers replaceObjectAtIndex:fromPos withObject:reuseType];
      [_fiberFullSpanItems replaceObjectAtIndex:fromPos
                                     withObject:[NSNumber numberWithBool:isFullSpan]];
      [_fiberStickyTopItems replaceObjectAtIndex:fromPos
                                      withObject:[NSNumber numberWithBool:isStickyTop]];
      [_fiberStickyBottomItems replaceObjectAtIndex:fromPos
                                         withObject:[NSNumber numberWithBool:isStickyBottom]];
      if (estimatedHeightPx > 0) {
        _estimatedHeights[fromPath] = @(estimatedHeightPx);
      }
      if (isFlush) {
        [updateFromPath addObject:fromPath];
        [updateToPath addObject:toPath];
      }
    }
  }
  [self transformExtraData];
  _diffResult = [[LynxUIListDiffResult alloc] initWithUpdateFromPath:updateFromPath
                                                        updateToPath:updateToPath
                                                          removePath:removePath
                                                          insertPath:insertPath
                                                        moveFromPath:moveFromPath
                                                          moveToPath:moveToPath];
}

// transform extra data ,such as full-span、sticky-top、sticky-bottom
- (void)transformExtraData {
  [_fullSpanItems removeAllObjects];
  [_stickyTopItems removeAllObjects];
  [_stickyBottomItems removeAllObjects];

  for (int i = 0; i < (int)_fiberFullSpanItems.count; i++) {
    BOOL value = [_fiberFullSpanItems[i] boolValue];
    if (value) {
      [_fullSpanItems addObject:[NSIndexPath indexPathForItem:i inSection:0]];
    }
  }

  for (int i = 0; i < (int)_fiberStickyTopItems.count; i++) {
    BOOL value = [_fiberStickyTopItems[i] boolValue];
    if (value) {
      [_stickyTopItems addObject:[NSIndexPath indexPathForItem:i inSection:0]];
    }
  }

  for (int i = 0; i < (int)_fiberStickyBottomItems.count; i++) {
    BOOL value = [_fiberStickyBottomItems[i] boolValue];
    if (value) {
      [_stickyBottomItems addObject:[NSIndexPath indexPathForItem:i inSection:0]];
    }
  }
}

@end
