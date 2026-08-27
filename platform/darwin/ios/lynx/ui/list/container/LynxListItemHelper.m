// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListItemHelper.h"

#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIComponent.h>

@interface LynxListItemHelper ()

@property(nonatomic, weak) id<LynxListItemHelperOwner> owner;

- (void)updateComponent:(LynxUIComponent *)component
              inWrapper:(UIView<LynxListItemWrapper> *)wrapper
           addSubLayers:(BOOL)addSubLayers
      adjustLayersFrame:(BOOL)adjustLayersFrame;

@end

@implementation LynxListItemHelper

- (instancetype)initWithOwner:(id<LynxListItemHelperOwner>)owner {
  self = [super init];
  if (self) {
    _owner = owner;
  }
  return self;
}

- (void)attachComponent:(LynxUIComponent *)component
              toWrapper:(UIView<LynxListItemWrapper> *)wrapper {
  [self updateComponent:component inWrapper:wrapper addSubLayers:YES adjustLayersFrame:NO];
}

- (void)updateLayoutForComponent:(LynxUIComponent *)component
                       inWrapper:(UIView<LynxListItemWrapper> *)wrapper {
  [self updateComponent:component inWrapper:wrapper addSubLayers:NO adjustLayersFrame:YES];
}

- (void)updateComponent:(LynxUIComponent *)component
              inWrapper:(UIView<LynxListItemWrapper> *)wrapper
           addSubLayers:(BOOL)addSubLayers
      adjustLayersFrame:(BOOL)adjustLayersFrame {
  UIView *listItemView = component.view;
  CGRect frame = component.frame;
  wrapper.frame = frame;
  listItemView.frame = CGRectMake(0, 0, frame.size.width, frame.size.height);
  [wrapper addSubview:listItemView];

  // Keep layer ownership tied to the component currently held by the wrapper. This preserves
  // list-container's behavior when a wrapper is being reused while its layout is updated.
  LynxUIComponent *holdingUI = wrapper.holdingUI;
  if (!holdingUI.backgroundManager) {
    return;
  }

  LynxBackgroundManager *backgroundManager = holdingUI.backgroundManager;
  // Move the borderLayer and backgroundLayer of the ListItemView to the wrapperView.
  if (addSubLayers) {
    // Note: If list-item is newly created, all layers are added to WrapperView's layer in
    // OnNodeReady(), but if list-item is reused we need to execute add sub layers.
    CALayer *listItemViewLayer = holdingUI.view.layer;
    if (backgroundManager.borderLayer) {
      [backgroundManager.borderLayer removeFromSuperlayer];
      [wrapper.layer insertSublayer:backgroundManager.borderLayer above:listItemViewLayer];
    }
    if (backgroundManager.backgroundLayer) {
      [backgroundManager.backgroundLayer removeFromSuperlayer];
      [wrapper.layer insertSublayer:backgroundManager.backgroundLayer below:listItemViewLayer];
    }
  }

  // Adjust all related layers (background, border and mask layers).
  if (adjustLayersFrame) {
    NSValue *value = [NSValue valueWithCGRect:listItemView.frame];
    [holdingUI setLayerValue:value forKeyPath:@"frame" forAllLayers:YES];
  }
}

- (NSInteger)indexForItemKey:(NSString *)itemKey {
  __block NSInteger target = -1;
  [[self.owner itemKeysForListItemHelper]
      enumerateObjectsUsingBlock:^(NSString *_Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        if ([obj isEqualToString:itemKey]) {
          target = idx;
          *stop = YES;
        }
      }];
  return target;
}

- (BOOL)isVisibleItemWrapper:(UIView<LynxListItemWrapper> *)wrapper {
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  if ([self.owner isVerticalForListItemHelper]) {
    CGFloat minY = CGRectGetMinY(wrapper.frame);
    CGFloat maxY = CGRectGetMaxY(wrapper.frame);
    CGFloat offsetMinY = scrollView.contentOffset.y;
    CGFloat offsetMaxY = offsetMinY + CGRectGetHeight(scrollView.frame);
    return ((minY <= offsetMinY && maxY >= offsetMinY) ||
            (minY <= offsetMaxY && maxY >= offsetMaxY) ||
            (minY >= offsetMinY && maxY <= offsetMaxY));
  }

  CGFloat minX = CGRectGetMinX(wrapper.frame);
  CGFloat maxX = CGRectGetMaxX(wrapper.frame);
  CGFloat offsetMinX = scrollView.contentOffset.x;
  CGFloat offsetMaxX = offsetMinX + CGRectGetWidth(scrollView.frame);
  return ((minX <= offsetMinX && maxX >= offsetMinX) ||
          (minX <= offsetMaxX && maxX >= offsetMaxX) || (minX >= offsetMinX && maxX <= offsetMaxX));
}

- (NSArray<UIView<LynxListItemWrapper> *> *)visibleItemWrappers {
  NSMutableArray<UIView<LynxListItemWrapper> *> *visibleWrappers = [NSMutableArray array];
  for (UIView *subview in [self.owner scrollViewForListItemHelper].subviews) {
    if (![subview conformsToProtocol:@protocol(LynxListItemWrapper)]) {
      continue;
    }
    UIView<LynxListItemWrapper> *wrapper = (UIView<LynxListItemWrapper> *)subview;
    if ([self isVisibleItemWrapper:wrapper]) {
      [visibleWrappers addObject:wrapper];
    }
  }

  [visibleWrappers
      sortUsingComparator:^NSComparisonResult(UIView<LynxListItemWrapper> *_Nonnull lhs,
                                              UIView<LynxListItemWrapper> *_Nonnull rhs) {
        NSInteger lhsPosition = [self indexForItemKey:lhs.holdingUI.itemKey];
        NSInteger rhsPosition = [self indexForItemKey:rhs.holdingUI.itemKey];
        if (lhsPosition < rhsPosition) {
          return NSOrderedAscending;
        }
        if (lhsPosition > rhsPosition) {
          return NSOrderedDescending;
        }
        return NSOrderedSame;
      }];

  return visibleWrappers;
}

- (NSArray<NSDictionary *> *)visibleItemInfo {
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  NSMutableArray<NSDictionary *> *visibleItemInfo = [NSMutableArray array];
  for (UIView<LynxListItemWrapper> *wrapper in self.visibleItemWrappers) {
    LynxUIComponent *holdingUI = wrapper.holdingUI;
    CGFloat cellTop = wrapper.frame.origin.y - scrollView.contentOffset.y;
    CGFloat cellLeft = wrapper.frame.origin.x - scrollView.contentOffset.x;
    NSInteger position = [self indexForItemKey:holdingUI.itemKey];
    [visibleItemInfo addObject:@{
      @"id" : (holdingUI.idSelector ?: @"unknown"),
      @"position" : @(position),
      @"index" : @(position),
      @"itemKey" : holdingUI.itemKey ?: @"",
      @"top" : @(cellTop),
      @"bottom" : @(cellTop + wrapper.frame.size.height),
      @"left" : @(cellLeft),
      @"right" : @(cellLeft + wrapper.frame.size.width),
    }];
  }
  return visibleItemInfo;
}

- (id<LynxEventTarget>)findHitTargetAtPoint:(CGPoint)point withEvent:(UIEvent *)event {
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  NSArray<UIView *> *subviews = scrollView.subviews;
  NSMapTable<UIView *, NSNumber *> *subviewIndexes = [NSMapTable
      mapTableWithKeyOptions:NSPointerFunctionsOpaqueMemory | NSPointerFunctionsOpaquePersonality
                valueOptions:NSPointerFunctionsStrongMemory];
  [subviews
      enumerateObjectsUsingBlock:^(UIView *_Nonnull view, NSUInteger index, BOOL *_Nonnull stop) {
        [subviewIndexes setObject:@(index) forKey:view];
      }];
  NSArray<UIView *> *subviewsSortedByDescendingZIndex = [subviews
      sortedArrayUsingComparator:^NSComparisonResult(UIView *_Nonnull lhs, UIView *_Nonnull rhs) {
        NSInteger lhsZPosition = lhs.layer.zPosition;
        NSInteger rhsZPosition = rhs.layer.zPosition;
        if (lhsZPosition < rhsZPosition) {
          return NSOrderedDescending;
        }
        if (lhsZPosition > rhsZPosition) {
          return NSOrderedAscending;
        }
        // Compatibility with list-container: equal-z subviews are checked in their existing
        // scroll-view order. Use an explicit tie-break so the comparator remains well-formed.
        NSUInteger lhsIndex = [[subviewIndexes objectForKey:lhs] unsignedIntegerValue];
        NSUInteger rhsIndex = [[subviewIndexes objectForKey:rhs] unsignedIntegerValue];
        if (lhsIndex < rhsIndex) {
          return NSOrderedAscending;
        }
        if (lhsIndex > rhsIndex) {
          return NSOrderedDescending;
        }
        return NSOrderedSame;
      }];

  for (UIView *subview in subviewsSortedByDescendingZIndex) {
    if (![subview conformsToProtocol:@protocol(LynxListItemWrapper)]) {
      continue;
    }
    UIView<LynxListItemWrapper> *wrapper = (UIView<LynxListItemWrapper> *)subview;
    LynxUIComponent *holdingUI = wrapper.holdingUI;
    CGPoint pointInCell = [holdingUI.view convertPoint:point fromView:scrollView];
    if ([holdingUI containsPoint:pointInCell inHitTestFrame:wrapper.bounds]) {
      id<LynxEventTarget> hitTarget = [holdingUI hitTest:pointInCell withEvent:event];
      if (hitTarget) {
        return hitTarget;
      }
    }
  }
  return nil;
}

- (UIView *)firstSubviewAtPoint:(CGPoint)point {
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  for (UIView *subview in scrollView.subviews) {
    CGPoint pointInLayer = [scrollView convertPoint:point toView:subview];
    if ([subview.layer containsPoint:pointInLayer]) {
      return subview;
    }
  }
  return nil;
}

@end
