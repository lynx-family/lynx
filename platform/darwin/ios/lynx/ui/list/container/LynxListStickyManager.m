// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListStickyManager.h"

#import <Lynx/LynxScrollEventManager.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIComponent.h>

@interface LynxListStickyManager ()

@property(nonatomic, weak) id<LynxListStickyManagerOwner> owner;
@property(nonatomic, strong, nullable) NSArray<NSNumber *> *stickyStartIndexes;
@property(nonatomic, strong, nullable) NSArray<NSNumber *> *stickyEndIndexes;
@property(nonatomic, strong) NSMutableSet<NSString *> *stickyStartItemKeySet;
@property(nonatomic, strong) NSMutableSet<NSString *> *stickyEndItemKeySet;
@property(nonatomic, strong)
    NSMutableDictionary<NSString *, LynxUIComponent *> *stickyStartItemDict;
@property(nonatomic, strong) NSMutableDictionary<NSString *, LynxUIComponent *> *stickyEndItemDict;
@property(nonatomic, weak, nullable) LynxUIComponent *previousStickyStartItem;
@property(nonatomic, weak, nullable) LynxUIComponent *previousStickyEndItem;
@property(nonatomic, assign) BOOL updatingAfterLayout;

@end

@implementation LynxListStickyManager

- (instancetype)initWithOwner:(id<LynxListStickyManagerOwner>)owner {
  self = [super init];
  if (self) {
    _owner = owner;
    _stickyStartItemKeySet = [NSMutableSet set];
    _stickyEndItemKeySet = [NSMutableSet set];
    _stickyStartItemDict = [NSMutableDictionary dictionary];
    _stickyEndItemDict = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)setStickyStartIndexes:(NSArray<NSNumber *> *_Nullable)startIndexes
                   endIndexes:(NSArray<NSNumber *> *_Nullable)endIndexes {
  self.stickyStartIndexes = startIndexes;
  self.stickyEndIndexes = endIndexes;
}

- (void)propsDidUpdate {
  if (!self.enabled) {
    return;
  }
  [self.stickyStartItemKeySet removeAllObjects];
  [self.stickyEndItemKeySet removeAllObjects];
  [self updateStickyItemKeySet:self.stickyStartItemKeySet
                stickyItemDict:self.stickyStartItemDict
                   withIndexes:self.stickyStartIndexes];
  [self updateStickyItemKeySet:self.stickyEndItemKeySet
                stickyItemDict:self.stickyEndItemDict
                   withIndexes:self.stickyEndIndexes];
}

- (void)didLayoutComponent:(LynxUIComponent *)component {
  if (self.enabled) {
    NSString *itemKey = component.itemKey;
    if (itemKey != nil) {
      if ([self.stickyStartItemKeySet containsObject:itemKey]) {
        [self updateStickyItemDictWithItem:component
                            stickyItemDict:self.stickyStartItemDict
                                    sticky:YES];
      } else if ([self.stickyEndItemKeySet containsObject:itemKey]) {
        [self updateStickyItemDictWithItem:component
                            stickyItemDict:self.stickyEndItemDict
                                    sticky:YES];
      } else {
        [self updateStickyItemDictWithItem:component
                            stickyItemDict:self.stickyStartItemDict
                                    sticky:NO];
        [self updateStickyItemDictWithItem:component
                            stickyItemDict:self.stickyEndItemDict
                                    sticky:NO];
      }
    }
  }
  [self updateStickyAfterLayoutIfNeeded:component];
}

- (void)didAttachComponent:(LynxUIComponent *)component {
  if (!self.enabled) {
    return;
  }
  NSString *itemKey = component.itemKey;
  if (itemKey != nil) {
    if ([self.stickyStartItemKeySet containsObject:itemKey]) {
      self.stickyStartItemDict[itemKey] = component;
    } else if ([self.stickyEndItemKeySet containsObject:itemKey]) {
      self.stickyEndItemDict[itemKey] = component;
    }
  }
}

- (void)willDetachComponent:(LynxUIComponent *)component {
  if (!self.enabled) {
    return;
  }
  NSString *itemKey = component.itemKey;
  if (itemKey != nil) {
    if (self.stickyStartItemDict[itemKey]) {
      [self.stickyStartItemDict removeObjectForKey:itemKey];
      [self resetStickyItem:component];
    } else if (self.stickyEndItemDict[itemKey]) {
      [self.stickyEndItemDict removeObjectForKey:itemKey];
      [self resetStickyItem:component];
    }
  }
}

- (void)updateStickyItems {
  [self updateStickyStarts];
  [self updateStickyEnds];
}

- (id<LynxEventTarget>)findHitTargetAtPoint:(CGPoint)point withEvent:(UIEvent *)event {
  id<LynxEventTarget> hitTarget;
  hitTarget = [self findHitTargetInStickyItemDict:self.stickyStartItemDict
                                          atPoint:point
                                        withEvent:event];
  if (hitTarget) {
    return hitTarget;
  }
  hitTarget = [self findHitTargetInStickyItemDict:self.stickyEndItemDict
                                          atPoint:point
                                        withEvent:event];
  return hitTarget;
}

- (void)updateStickyItemKeySet:(NSMutableSet<NSString *> *)stickyItemKeySet
                stickyItemDict:(NSMutableDictionary<NSString *, LynxUIComponent *> *)stickyItemDict
                   withIndexes:(NSArray<NSNumber *> *_Nullable)stickyIndexes {
  NSArray<NSString *> *itemKeys = [self.owner itemKeysForListStickyManager];
  [stickyIndexes
      enumerateObjectsUsingBlock:^(NSNumber *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        NSInteger index = obj.integerValue;
        if (index >= 0 && (NSUInteger)index < itemKeys.count) {
          [stickyItemKeySet addObject:itemKeys[index]];
        }
      }];
  if (stickyItemDict.count) {
    [[stickyItemDict copy]
        enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, LynxUIComponent *_Nonnull obj,
                                            BOOL *_Nonnull stop) {
          if (key != nil && obj != nil && ![stickyItemKeySet containsObject:key]) {
            [self resetStickyItem:obj];
            [stickyItemDict removeObjectForKey:key];
          }
        }];
  }
}

- (void)updateStickyAfterLayoutIfNeeded:(LynxUIComponent *)component {
  if (!self.enabled || self.updatingAfterLayout) {
    return;
  }
  BOOL isStickyRelevant = NO;
  if (component.itemKey) {
    isStickyRelevant = [self.stickyStartItemKeySet containsObject:component.itemKey] ||
                       [self.stickyEndItemKeySet containsObject:component.itemKey];
  }
  if (!isStickyRelevant) {
    isStickyRelevant =
        (self.previousStickyStartItem == component) || (self.previousStickyEndItem == component);
  }
  if (!isStickyRelevant) {
    return;
  }
  self.updatingAfterLayout = YES;
  [self updateStickyItems];
  self.updatingAfterLayout = NO;
}

- (void)updateStickyItemDictWithItem:(LynxUIComponent *)component
                      stickyItemDict:
                          (NSMutableDictionary<NSString *, LynxUIComponent *> *)stickyItemDict
                              sticky:(BOOL)sticky {
  if (!component || !component.itemKey) {
    return;
  }
  if (sticky) {
    NSString *newItemKey = component.itemKey;
    if (stickyItemDict[newItemKey] == component) {
      return;
    }
    [[stickyItemDict copy]
        enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, LynxUIComponent *_Nonnull obj,
                                            BOOL *_Nonnull stop) {
          if (![newItemKey isEqualToString:key] && obj == component) {
            [stickyItemDict removeObjectForKey:key];
            stickyItemDict[newItemKey] = component;
            *stop = YES;
          }
        }];
  } else {
    [[stickyItemDict copy]
        enumerateKeysAndObjectsUsingBlock:^(NSString *_Nonnull key, LynxUIComponent *_Nonnull obj,
                                            BOOL *_Nonnull stop) {
          if (obj == component) {
            [stickyItemDict removeObjectForKey:key];
            [self resetStickyItem:component];
            *stop = YES;
          }
        }];
  }
}

- (void)resetStickyItem:(LynxUIComponent *_Nullable)component {
  if (component.view && component.view.superview) {
    component.view.superview.frame = component.frame;
    component.view.frame =
        CGRectMake(0, 0, CGRectGetWidth(component.frame), CGRectGetHeight(component.frame));
    component.view.superview.layer.zPosition = component.zIndex;
  }
}

- (LynxUIComponent *)stickyItemWithIndex:(NSNumber *)indexValue start:(BOOL)start {
  NSDictionary<NSString *, LynxUIComponent *> *stickyItemDict =
      start ? self.stickyStartItemDict : self.stickyEndItemDict;
  NSInteger index = indexValue.integerValue;
  NSArray<NSString *> *itemKeys = [self.owner itemKeysForListStickyManager];
  if (index >= 0 && (NSUInteger)index < itemKeys.count) {
    NSString *itemKey = itemKeys[index];
    if (itemKey != nil) {
      return stickyItemDict[itemKey];
    }
  }
  return nil;
}

- (void)updateStickyStarts {
  if (!self.enabled) {
    return;
  }

  // Keep list-container compatibility: bounce offsets are clamped before positioning sticky items.
  UIScrollView *scrollView = [self.owner scrollViewForListStickyManager];
  BOOL vertical = [self.owner isVerticalForListStickyManager];
  CGFloat offset =
      (vertical ? MAX(0, scrollView.contentOffset.y) : MAX(0, scrollView.contentOffset.x)) +
      self.offset;
  LynxUIComponent *stickyStartItem = nil;
  LynxUIComponent *nextStickyStartItem = nil;

  for (NSNumber *startIndex in self.stickyStartIndexes.reverseObjectEnumerator) {
    LynxUIComponent *startItem = [self stickyItemWithIndex:startIndex start:YES];
    if (!startItem) {
      continue;
    }
    CGFloat startOffset =
        vertical ? CGRectGetMinY(startItem.frame) : CGRectGetMinX(startItem.frame);
    if (startOffset > offset) {
      nextStickyStartItem = startItem;
      [self resetStickyItem:startItem];
    } else if (stickyStartItem) {
      [self resetStickyItem:startItem];
    } else {
      stickyStartItem = startItem;
    }
  }

  if (!stickyStartItem) {
    return;
  }
  if (self.previousStickyStartItem != stickyStartItem) {
    LynxScrollEventManager *eventManager = [self.owner eventManagerForListStickyManager];
    if (vertical) {
      [eventManager sendScrollEvent:LynxEventStickyTop
                         scrollView:scrollView
                             detail:@{@"top" : stickyStartItem.itemKey}];
    }
    [eventManager sendScrollEvent:LynxEventStickyStart
                       scrollView:scrollView
                           detail:@{@"start" : stickyStartItem.itemKey}];
    self.previousStickyStartItem = stickyStartItem;
  }

  CGFloat stickyStartOffset = offset;
  if (nextStickyStartItem) {
    CGFloat distance = vertical ? CGRectGetMinY(nextStickyStartItem.frame) - offset
                                : CGRectGetMinX(nextStickyStartItem.frame) - offset;
    CGFloat squash = (vertical ? CGRectGetHeight(stickyStartItem.frame)
                               : CGRectGetWidth(stickyStartItem.frame)) -
                     distance;
    if (squash > 0) {
      stickyStartOffset -= squash;
    }
  }

  if (vertical) {
    stickyStartItem.view.superview.frame =
        (CGRect){CGPointMake(CGRectGetMinX(stickyStartItem.frame), stickyStartOffset),
                 stickyStartItem.frame.size};
  } else {
    stickyStartItem.view.superview.frame =
        (CGRect){CGPointMake(stickyStartOffset, CGRectGetMinY(stickyStartItem.frame)),
                 stickyStartItem.frame.size};
  }
  stickyStartItem.view.frame = CGRectMake(0, 0, CGRectGetWidth(stickyStartItem.frame),
                                          CGRectGetHeight(stickyStartItem.frame));
  [scrollView bringSubviewToFront:stickyStartItem.view.superview];
  stickyStartItem.view.superview.layer.zPosition = NSIntegerMax;
}

- (void)updateStickyEnds {
  if (!self.enabled) {
    return;
  }
  UIScrollView *scrollView = [self.owner scrollViewForListStickyManager];
  BOOL vertical = [self.owner isVerticalForListStickyManager];
  CGFloat offset = 0;
  if (vertical) {
    offset = MIN(scrollView.contentOffset.y,
                 MAX(0, scrollView.contentSize.height - CGRectGetHeight(scrollView.frame))) +
             CGRectGetHeight(scrollView.frame) - self.offset;
  } else {
    offset = MIN(scrollView.contentOffset.x,
                 MAX(0, scrollView.contentSize.width - CGRectGetWidth(scrollView.frame))) +
             CGRectGetWidth(scrollView.frame) - self.offset;
  }

  LynxUIComponent *stickyEndItem = nil;
  LynxUIComponent *nextStickyEndItem = nil;
  for (NSNumber *endIndex in self.stickyEndIndexes) {
    LynxUIComponent *endItem = [self stickyItemWithIndex:endIndex start:NO];
    if (!endItem) {
      continue;
    }
    CGFloat currentOffset = vertical ? CGRectGetMaxY(endItem.frame) : CGRectGetMaxX(endItem.frame);
    if (currentOffset < offset) {
      nextStickyEndItem = endItem;
      [self resetStickyItem:endItem];
    } else if (stickyEndItem) {
      [self resetStickyItem:endItem];
    } else {
      stickyEndItem = endItem;
    }
  }

  if (!stickyEndItem) {
    return;
  }
  if (self.previousStickyEndItem != stickyEndItem) {
    LynxScrollEventManager *eventManager = [self.owner eventManagerForListStickyManager];
    if (vertical) {
      [eventManager sendScrollEvent:LynxEventStickyBottom
                         scrollView:scrollView
                             detail:@{@"bottom" : stickyEndItem.itemKey}];
    }
    [eventManager sendScrollEvent:LynxEventStickyEnd
                       scrollView:scrollView
                           detail:@{@"end" : stickyEndItem.itemKey}];
    self.previousStickyEndItem = stickyEndItem;
  }

  CGFloat stickyStartOffset = offset - (vertical ? CGRectGetHeight(stickyEndItem.frame)
                                                 : CGRectGetWidth(stickyEndItem.frame));
  if (nextStickyEndItem) {
    CGFloat distance = vertical ? offset - CGRectGetMaxY(nextStickyEndItem.frame)
                                : offset - CGRectGetMaxX(nextStickyEndItem.frame);
    CGFloat squash =
        (vertical ? CGRectGetHeight(stickyEndItem.frame) : CGRectGetWidth(stickyEndItem.frame)) -
        distance;
    if (squash > 0) {
      stickyStartOffset += squash;
    }
  }

  if (vertical) {
    stickyEndItem.view.superview.frame =
        (CGRect){CGPointMake(CGRectGetMinX(stickyEndItem.frame), stickyStartOffset),
                 stickyEndItem.frame.size};
  } else {
    stickyEndItem.view.superview.frame =
        (CGRect){CGPointMake(stickyStartOffset, CGRectGetMinY(stickyEndItem.frame)),
                 stickyEndItem.frame.size};
  }
  stickyEndItem.view.frame =
      CGRectMake(0, 0, CGRectGetWidth(stickyEndItem.frame), CGRectGetHeight(stickyEndItem.frame));
  [scrollView bringSubviewToFront:stickyEndItem.view.superview];
  stickyEndItem.view.superview.layer.zPosition = NSIntegerMax;
}

- (id<LynxEventTarget>)findHitTargetInStickyItemDict:(NSDictionary *)stickyItemDict
                                             atPoint:(CGPoint)point
                                           withEvent:(UIEvent *)event {
  __block id<LynxEventTarget> hitTarget;
  UIScrollView *scrollView = [self.owner scrollViewForListStickyManager];
  [stickyItemDict enumerateKeysAndObjectsUsingBlock:^(
                      id _Nonnull key, LynxUIComponent *_Nonnull component, BOOL *_Nonnull stop) {
    CGPoint pointInCell = [component.view convertPoint:point fromView:scrollView];
    if ([component containsPoint:pointInCell inHitTestFrame:component.view.bounds]) {
      hitTarget = [component hitTest:pointInCell withEvent:event];
      if (hitTarget) {
        *stop = YES;
      }
    }
  }];
  return hitTarget;
}

@end
