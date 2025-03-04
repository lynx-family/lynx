// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxListLayoutManager.h>
#import <Lynx/LynxListVerticalLayoutManager.h>
#import <Lynx/LynxListViewCellLight.h>
#import <Lynx/LynxListViewLight.h>
#import <Lynx/LynxUIListDataSource.h>
#import <Lynx/LynxUIListLight.h>
#import <Lynx/LynxUIListProtocol.h>
#import "LynxUI+Internal.h"
#import "LynxUIMethodProcessor.h"
#include "core/shell/lynx_shell.h"

@interface LynxUIListLight ()
@property(nonatomic, strong) LynxUIListDataSource *dataSource;
@property(nonatomic, strong, nullable) NSMutableArray<NSString *> *reuseIdentifiers;
@property(nonatomic, strong, nullable, readonly) NSMutableArray<NSString *> *currentItemKeys;
@property(nonatomic, strong, nullable, readonly) NSMutableArray<NSNumber *> *fullSpanItems;
@property(nonatomic, strong, nullable, readonly) NSMutableArray<NSNumber *> *stickyTopItems;
@property(nonatomic, strong, nullable, readonly) NSMutableArray<NSNumber *> *stickyBottomItems;
@property(nonatomic, strong, nullable, readonly) NSMutableArray *fiberFullSpanItems;
@property(nonatomic, strong, nullable, readonly) NSMutableArray *fiberStickyTopItems;
@property(nonatomic, strong, nullable, readonly) NSMutableArray *fiberStickyBottomItems;
@property(nonatomic, strong, nullable, readonly)
    NSMutableDictionary<NSNumber *, NSNumber *> *estimatedHeights;
@property(nonatomic, strong) LynxUIListInvalidationContext *currentNodiffContext;
@end

@implementation LynxUIListLight
- (UIView *)createView {
  LynxListViewLight *recycleScrollView = [[LynxListViewLight alloc] init];
  recycleScrollView.numberOfColumns = 1;
  return recycleScrollView;
}

- (LynxUIListDataSource *)dataSource {
  if (_dataSource) {
    return _dataSource;
  }
  _dataSource = [[LynxUIListDataSource alloc] init];
  return _dataSource;
}

- (void)setSign:(NSInteger)sign {
  [super setSign:sign];
  [(LynxListViewLight *)self.view setSign:sign];
  [(LynxListViewLight *)self.view setLayout:self.context.customizedListLayout];
  [(LynxListViewLight *)self.view setDataSource:self.dataSource];
  [(LynxListViewLight *)self.view setUIContext:self.context];
  [_dataSource setLynxUIContext:self.context];
  [_dataSource setLynxSign:self.sign];
  [(LynxListViewLight *)self.view setEventEmitter:self.context.eventEmitter];
}

- (void)updateFrame:(CGRect)frame
            withPadding:(UIEdgeInsets)padding
                 border:(UIEdgeInsets)border
                 margin:(UIEdgeInsets)margin
    withLayoutAnimation:(BOOL)with {
  [super updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:with];
  [((LynxListViewLight *)self.view) updateFrame:frame
                                    withPadding:padding
                                         border:border
                                         margin:margin
                            withLayoutAnimation:with];
}

- (void)setVerticalOrientation:(BOOL)value {
  [(LynxListViewLight *)self.view setVerticalOrientation:value];
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
  ((LynxUIComponent *)child).layoutObserver = self;
  [super insertChild:child atIndex:index];
}

- (LynxUIListScrollThresholds *)scrollThreSholds {
  if (!_scrollThreSholds) {
    _scrollThreSholds = [[LynxUIListScrollThresholds alloc] init];
  }
  return _scrollThreSholds;
}

#pragma mark load diff
- (LynxUIListInvalidationContext *)loadListInfo:(NSDictionary *)diffResult
                                     components:(NSDictionary<NSString *, NSArray *> *)components {
  // The platformInfo could be nil in some cases, so we ignore it and do not trigger the
  // invalidation updates.
  if (!diffResult || diffResult.count == 0 || !components) {
    return nil;
  }
  LynxUIListInvalidationContext *context = [[LynxUIListInvalidationContext alloc] init];

  NSMutableArray<NSString *> *reuseIdentifiers =
      [[NSMutableArray alloc] initWithCapacity:components.count];
  NSMutableDictionary<NSNumber *, NSNumber *> *estimatedHeights =
      [[NSMutableDictionary alloc] init];
  NSMutableArray *fullSpan = [[NSMutableArray alloc] init];
  NSMutableArray *stickyTop = [[NSMutableArray alloc] init];
  NSMutableArray *stickyBottom = [[NSMutableArray alloc] init];
  NSArray<NSString *> *itemKeys = components[@"itemkeys"];

  [itemKeys
      enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        CGFloat estimatedHeight = [components[@"estimatedHeight"][idx] doubleValue];
        CGFloat estimatedHeightPX = [components[@"estimatedHeightPx"][idx] doubleValue];
        if (estimatedHeightPX > 0) {
          estimatedHeights[@(idx)] = @(estimatedHeightPX);
        } else if (estimatedHeight > 0) {
          estimatedHeights[@(idx)] = @(estimatedHeight);
        }
        if (idx < components[@"fullspan"].count) {
          [fullSpan addObject:components[@"fullspan"][idx]];
        }

        if (idx < components[@"stickyTop"].count) {
          [stickyTop addObject:components[@"stickyTop"][idx]];
        }

        if (idx < components[@"stickyBottom"].count) {
          [stickyBottom addObject:components[@"stickyBottom"][idx]];
        }

        [reuseIdentifiers addObject:components[@"viewTypes"][idx]];
      }];

  [self registerAllReuseIdentifiers:reuseIdentifiers];
  self.reuseIdentifiers = reuseIdentifiers;
  context.fullSpanItems = fullSpan;
  context.stickyTopItems = stickyTop;
  context.stickyBottomItems = stickyBottom;
  context.estimatedHeights = estimatedHeights;
  context.insertions = diffResult[@"insertions"];
  context.removals = diffResult[@"removals"];
  context.moveFrom = diffResult[@"moveFrom"];
  context.moveTo = diffResult[@"moveTo"];
  context.updateTo = diffResult[@"updateTo"];
  context.updateFrom = diffResult[@"updateFrom"];
  context.listUpdateType = LynxListUpdateTypeDataUpdate;
  return context;
}

- (void)registerAllReuseIdentifiers:(NSArray<NSString *> *)reuseIdentifiers {
  [(LynxListViewLight *)self.view updateReuseIdentifiers:reuseIdentifiers];
  [(LynxListViewLight *)self.view registerCellClass:[LynxListViewCellLightLynxUI class]
                                   reuseIdentifiers:reuseIdentifiers];
}

#pragma mark nodiff
- (void)updateListActionInfo:(NSDictionary *)noDiffResult {
  NSArray *updateAction = noDiffResult[@"updateAction"];
  NSArray *removeAction = noDiffResult[@"removeAction"];
  NSArray *insertAction = noDiffResult[@"insertAction"];

  if (updateAction == nil && removeAction == nil && insertAction == nil) {
    return;
  }

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

  NSMutableArray<NSNumber *> *updateFrom = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber *> *updateTo = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber *> *removals = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber *> *insertions = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber *> *moveFrom = [[NSMutableArray alloc] init];
  NSMutableArray<NSNumber *> *moveTo = [[NSMutableArray alloc] init];
  // remove list component according to "remove" data
  if (removeAction != nil) {
    int count = (int)removeAction.count;
    for (int i = 0; i < count; i++) {
      NSInteger position = [[removeAction objectAtIndex:i] integerValue];
      if (position < 0) {
        continue;
      }
      [_currentItemKeys removeObjectAtIndex:position];
      [_reuseIdentifiers removeObjectAtIndex:position];
      [_fiberFullSpanItems removeObjectAtIndex:position];
      [_fiberStickyBottomItems removeObjectAtIndex:position];
      [_fiberStickyTopItems removeObjectAtIndex:position];
      [removals addObject:@(position)];
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
      [_currentItemKeys insertObject:itemKey atIndex:position];
      [_reuseIdentifiers insertObject:reuseType atIndex:position];
      [_fiberFullSpanItems insertObject:[NSNumber numberWithBool:isFullSpan] atIndex:position];
      [_fiberStickyTopItems insertObject:[NSNumber numberWithBool:isStickyTop] atIndex:position];
      [_fiberStickyBottomItems insertObject:[NSNumber numberWithBool:isStickyBottom]
                                    atIndex:position];

      if (estimatedHeightPx > 0) {
        _estimatedHeights[@(position)] = @(estimatedHeightPx);
      }
      [insertions addObject:@(position)];
    }
  }
  // update list component according to "updateAction" data
  if (updateAction != nil) {
    for (id component in updateAction) {
      if (component == nil) {
        continue;
      }
      NSInteger fromPos = [component[@"from"] integerValue];
      NSString *itemKey = component[@"item-key"];
      NSString *reuseType = component[@"type"];
      bool isFullSpan = [component[@"full-span"] boolValue];
      bool isStickyTop = [component[@"sticky-top"] boolValue];
      bool isStickyBottom = [component[@"sticky-bottom"] boolValue];
      bool isFlush = [component[@"flush"] boolValue];
      CGFloat estimatedHeightPx = [component[@"estimated-height-px"] doubleValue];

      if (isFullSpan) {
        [_fullSpanItems addObject:component[@"from"]];
      } else {
        [_fullSpanItems removeObject:component[@"from"]];
      }
      if (isStickyTop) {
        [_stickyTopItems addObject:component[@"from"]];
      } else {
        [_stickyTopItems removeObject:component[@"from"]];
      }
      if (isStickyBottom) {
        [_stickyBottomItems addObject:component[@"from"]];
      } else {
        [_stickyBottomItems removeObject:component[@"from"]];
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
        _estimatedHeights[component[@"from"]] = @(estimatedHeightPx);
      }
      if (isFlush) {
        [updateFrom addObject:component[@"from"]];
        [updateTo addObject:component[@"to"]];
      }
    }
  }
  [self transformExtraData];
  _currentNodiffContext = [[LynxUIListInvalidationContext alloc] init];
  NSMutableDictionary<NSString *, NSArray<NSNumber *> *> *diffResult =
      [NSMutableDictionary dictionary];
  [diffResult setValue:insertions forKey:@"insertions"];
  [diffResult setValue:removals forKey:@"removals"];
  [diffResult setValue:moveFrom forKey:@"moveFrom"];
  [diffResult setValue:moveTo forKey:@"moveTo"];
  [diffResult setValue:updateTo forKey:@"updateTo"];
  [diffResult setValue:updateFrom forKey:@"updateFrom"];
  [self registerAllReuseIdentifiers:_reuseIdentifiers];
  _currentNodiffContext.fullSpanItems = _fullSpanItems;
  _currentNodiffContext.stickyTopItems = _stickyTopItems;
  _currentNodiffContext.stickyBottomItems = _stickyBottomItems;
  _currentNodiffContext.insertions = diffResult[@"insertions"];
  _currentNodiffContext.removals = diffResult[@"removals"];
  _currentNodiffContext.moveFrom = diffResult[@"moveFrom"];
  _currentNodiffContext.moveTo = diffResult[@"moveTo"];
  _currentNodiffContext.updateTo = diffResult[@"updateTo"];
  _currentNodiffContext.updateFrom = diffResult[@"updateFrom"];
  _currentNodiffContext.listUpdateType = LynxListUpdateTypeDataUpdate;
}

- (void)transformExtraData {
  [_fullSpanItems removeAllObjects];
  [_stickyTopItems removeAllObjects];
  [_stickyBottomItems removeAllObjects];

  for (int i = 0; i < (int)_fiberFullSpanItems.count; i++) {
    BOOL value = [_fiberFullSpanItems[i] boolValue];
    if (value) {
      [_fullSpanItems addObject:@(i)];
    }
  }

  for (int i = 0; i < (int)_fiberStickyTopItems.count; i++) {
    BOOL value = [_fiberStickyTopItems[i] boolValue];
    if (value) {
      [_stickyTopItems addObject:@(i)];
    }
  }

  for (int i = 0; i < (int)_fiberStickyBottomItems.count; i++) {
    BOOL value = [_fiberStickyBottomItems[i] boolValue];
    if (value) {
      [_stickyBottomItems addObject:@(i)];
    }
  }
}

#pragma mark UI_Method
LYNX_UI_METHOD(scrollToPosition) {
  NSInteger position = ((NSNumber *)[params objectForKey:@"position"]).intValue;
  __block CGFloat offset = ((NSNumber *)[params objectForKey:@"offset"]).doubleValue;
  BOOL smooth = [[params objectForKey:@"smooth"] boolValue];
  NSString *alignTo = [params objectForKey:@"alignTo"];
  if (position < 0 || position >= [(LynxListViewLight *)self.view totalItemsCount]) {
    if (callback) {
      callback(kUIMethodParamInvalid, @"position < 0 or position >= data count");
    }
    return;
  }
  [(LynxListViewLight *)self.view dispatchInvalidationContext:[[LynxUIListInvalidationContext alloc]
                                                                  initWithScrollToInfo:position
                                                                                offset:offset
                                                                               alignTo:alignTo
                                                                                smooth:smooth]];
}

#pragma mark component render callback
- (void)onComponentLayoutUpdated:(LynxUIComponent *)component {
  [(LynxListViewLight *)self.view onComponentLayoutUpdated:component];
}

- (LynxUIListInvalidationContext *)generalPropsInfo {
  if (!_generalPropsInfo) {
    _generalPropsInfo = [[LynxUIListInvalidationContext alloc] initWithGeneralPropsUpdate];
  }
  return _generalPropsInfo;
}

- (void)propsDidUpdate {
  [super propsDidUpdate];
  // generalPropsInfo including list-type/column-count/list-cross-axis-gap/...
  // If propsDidUpdate triggered by these changes, trigger invalidation here
  if (_generalPropsInfo) {
    [(LynxListViewLight *)self.view dispatchInvalidationContext:_generalPropsInfo];
    _generalPropsInfo = nil;
  }
}

#pragma mark async
- (BOOL)isAsync {
  auto shellPtr = self.context.shellPtr;
  if (!shellPtr) {
    return NO;
  }
  return reinterpret_cast<lynx::shell::LynxShell *>(shellPtr)->ThreadStrategy() != 0;
}

- (void)onAsyncComponentLayoutUpdated:(LynxUIComponent *)component
                          operationID:(int64_t)operationID {
  [(LynxListViewLight *)self.view onAsyncComponentLayoutUpdated:component operationID:operationID];
}

- (void)onNodeReady {
  [super onNodeReady];
  ((LynxListViewLight *)self.view).isAsync = [self isAsync];
  if (self.initialScrollIndex != nil) {
    [(LynxListViewLight *)self.view
        dispatchInvalidationContext:[[LynxUIListInvalidationContext alloc]
                                        initWithInitialScrollIndex:self.initialScrollIndex
                                                                       .integerValue]];
    self.initialScrollIndex = nil;
  }
  if (_scrollThreSholds) {
    [(LynxListViewLight *)self.view
        dispatchInvalidationContext:[[LynxUIListInvalidationContext alloc]
                                        initWithScrollThresholdsUpdate:_scrollThreSholds]];
    _scrollThreSholds = nil;
  }
  if (self.listNoDiffInfo) {
    [self updateListActionInfo:self.listNoDiffInfo];
    [(LynxListViewLight *)self.view dispatchInvalidationContext:_currentNodiffContext];
    self.listNoDiffInfo = nil;
    _currentNodiffContext = nil;
  } else {
    LynxUIListInvalidationContext *context = [self loadListInfo:self.diffResultFromTasm
                                                     components:self.curComponents];
    self.diffResultFromTasm = nil;
    self.curComponents = nil;
    [(LynxListViewLight *)self.view dispatchInvalidationContext:context];
  }
}

#pragma mark hittest
- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  if (self.context.enableEventRefactor) {
    return [((LynxListViewLight *)self.view) findHitTestTarget:point withEvent:event] ?: self;
  } else {
    id<LynxListCell> cell = [(LynxListViewLight *)self.view visibleCellAtPoint:point];
    if (cell == nil) return self;
    if ([cell isKindOfClass:LynxListViewCellLightLynxUI.class]) {
      LynxListViewCellLightLynxUI *uiCell = (LynxListViewCellLightLynxUI *)cell;
      CGPoint offset = [(LynxListViewLight *)self.view contentOffset];
      point = CGPointMake(point.x - uiCell.frame.origin.x + offset.x,
                          point.y - uiCell.frame.origin.y + offset.y);
      return [uiCell.ui hitTest:point withEvent:event];
    }
    return self;
  }
}
@end
