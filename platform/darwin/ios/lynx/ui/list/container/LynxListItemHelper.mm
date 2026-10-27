// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxListItemHelper.h"

#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxRendererHost.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIComponent.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIOwner.h>

@interface LynxListItemHelper ()

@property(nonatomic, weak) id<LynxListItemHelperOwner> owner;

- (void)updateComponent:(LynxUIComponent *)component
              inWrapper:(UIView<LynxListItemWrapper> *)wrapper
           addSubLayers:(BOOL)addSubLayers
      adjustLayersFrame:(BOOL)adjustLayersFrame;

@end

@implementation LynxListItemHelper

+ (BOOL)isCUIListItem:(LynxUI *)listItem {
  return [listItem isKindOfClass:LynxUIComponent.class] && [self isCUIListItemView:listItem.view];
}

+ (BOOL)isCUIListItemView:(UIView *)itemView {
  return [itemView respondsToSelector:@selector(renderer)] &&
         [(id<LynxRendererHost>)itemView renderer] != nil;
}

+ (BOOL)isListItemWrapperView:(UIView *)itemView {
  return [itemView conformsToProtocol:@protocol(LynxListItemWrapper)];
}

+ (LynxUIComponent *)getComponentForListItemView:(UIView *)listItemView {
  LynxUIComponent *component = nil;
  if ([self isListItemWrapperView:listItemView]) {
    component = ((UIView<LynxListItemWrapper> *)listItemView).holdingUI;
  } else if ([self isCUIListItemView:listItemView]) {
    LynxRenderer *renderer = [(id<LynxRendererHost>)listItemView renderer];
    LynxUIOwner *uiOwner = renderer.context.uiContext.uiOwner;
    LynxUI *ui = [uiOwner findUIBySign:renderer.sign];
    if ([ui isKindOfClass:LynxUIComponent.class] && ui.view == listItemView) {
      component = (LynxUIComponent *)ui;
    }
  }
  return component;
}

+ (void)applyLayoutForComponent:(LynxUIComponent *)component frame:(CGRect)frame {
  if ([self isCUIListItem:component]) {
    CALayer *layer = component.view.layer;
    [CATransaction begin];
    [CATransaction setDisableActions:YES];
    if (CATransform3DIsIdentity(layer.transform)) {
      component.view.frame = frame;
    } else {
      layer.bounds = (CGRect){layer.bounds.origin, frame.size};
      layer.position = CGPointMake(frame.origin.x + frame.size.width * layer.anchorPoint.x,
                                   frame.origin.y + frame.size.height * layer.anchorPoint.y);
    }
    LynxRenderer *renderer = [(id<LynxRendererHost>)component.view renderer];
    [renderer updateRendererLayoutFrame:frame];
    [renderer syncHostDecorationLayers];
    [CATransaction commit];
  } else {
    component.view.superview.frame = frame;
    component.view.frame = (CGRect){CGPointZero, frame.size};
  }
}

+ (void)bringComponentToFront:(LynxUIComponent *)component inScrollView:(UIScrollView *)scrollView {
  if ([self isCUIListItem:component]) {
    [scrollView bringSubviewToFront:component.view];
    [[(id<LynxRendererHost>)component.view renderer] reattachHostDecorationLayers];
  } else {
    [scrollView bringSubviewToFront:component.view.superview];
  }
}

+ (void)setZPosition:(CGFloat)zPosition forComponent:(LynxUIComponent *)component {
  if (![self isCUIListItem:component]) {
    component.view.superview.layer.zPosition = zPosition;
  }
}

- (instancetype)initWithOwner:(id<LynxListItemHelperOwner>)owner {
  self = [super init];
  if (self) {
    _owner = owner;
  }
  return self;
}

- (void)moveStickyItem:(LynxUIComponent *_Nullable)stickyItem
      toEndOfDrawOrder:(NSMutableArray<LynxUI *> *)drawOrder
          inScrollView:(UIScrollView *)scrollView {
  if ([[self class] isCUIListItem:stickyItem] && stickyItem.view.superview == scrollView &&
      [drawOrder indexOfObjectIdenticalTo:stickyItem] != NSNotFound) {
    [drawOrder removeObjectIdenticalTo:stickyItem];
    [drawOrder addObject:stickyItem];
  }
}

- (void)attachCUIComponent:(LynxUIComponent *)component
           orderedChildren:(NSArray<LynxUI *> *)children
           stickyStartItem:(LynxUIComponent *_Nullable)stickyStartItem
             stickyEndItem:(LynxUIComponent *_Nullable)stickyEndItem {
  if (![[self class] isCUIListItem:component]) {
    return;
  }
  // Get the attachment container from the owning list.
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  if (scrollView == nil) {
    return;
  }
  UIView *itemView = component.view;
  LynxRenderer *renderer = [(id<LynxRendererHost>)itemView renderer];
  // Attach the item if it is not already in the target list.
  if (itemView.superview != scrollView) {
    [renderer detachHostDecorationLayers];
    [scrollView addSubview:itemView];
  }
  // Keep active sticky items above ordinary items without changing the logical children order.
  // Sticky end is last, matching updateStickyItems bringing start to front before end.
  NSArray<LynxUI *> *orderedChildren = children;
  if (stickyStartItem || stickyEndItem) {
    NSMutableArray<LynxUI *> *drawOrder = [children mutableCopy];
    [self moveStickyItem:stickyStartItem toEndOfDrawOrder:drawOrder inScrollView:scrollView];
    [self moveStickyItem:stickyEndItem toEndOfDrawOrder:drawOrder inScrollView:scrollView];
    orderedChildren = drawOrder;
  }
  // Apply the list's CUI draw order even if the item is already attached.
  NSUInteger index = [orderedChildren indexOfObjectIdenticalTo:component];
  if (index == NSNotFound) {
    // Reattach decoration layers even if the item is not found in children.
    [renderer reattachHostDecorationLayers];
    return;
  }
  for (NSUInteger next = index + 1; next < orderedChildren.count; ++next) {
    LynxUI *child = orderedChildren[next];
    if (![child isKindOfClass:LynxUIComponent.class]) {
      continue;
    }
    LynxUIComponent *siblingChild = (LynxUIComponent *)child;
    if ([[self class] isCUIListItem:siblingChild] && siblingChild.view.superview == scrollView) {
      [scrollView insertSubview:itemView belowSubview:siblingChild.view];
      [[(id<LynxRendererHost>)siblingChild.view renderer] reattachHostDecorationLayers];
      [renderer reattachHostDecorationLayers];
      return;
    }
  }
  [scrollView bringSubviewToFront:itemView];
  [renderer reattachHostDecorationLayers];
}

- (void)detachCUIComponent:(LynxUIComponent *)component {
  if (![[self class] isCUIListItem:component]) {
    return;
  }
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  UIView *itemView = component.view;
  if (scrollView == nil || itemView.superview != scrollView) {
    return;
  }
  [[(id<LynxRendererHost>)itemView renderer] detachHostDecorationLayers];
  [itemView removeFromSuperview];
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

- (BOOL)isVisibleItemView:(UIView *)itemView {
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  if ([self.owner isVerticalForListItemHelper]) {
    CGFloat minY = CGRectGetMinY(itemView.frame);
    CGFloat maxY = CGRectGetMaxY(itemView.frame);
    CGFloat offsetMinY = scrollView.contentOffset.y;
    CGFloat offsetMaxY = offsetMinY + CGRectGetHeight(scrollView.frame);
    return ((minY <= offsetMinY && maxY >= offsetMinY) ||
            (minY <= offsetMaxY && maxY >= offsetMaxY) ||
            (minY >= offsetMinY && maxY <= offsetMaxY));
  }

  CGFloat minX = CGRectGetMinX(itemView.frame);
  CGFloat maxX = CGRectGetMaxX(itemView.frame);
  CGFloat offsetMinX = scrollView.contentOffset.x;
  CGFloat offsetMaxX = offsetMinX + CGRectGetWidth(scrollView.frame);
  return ((minX <= offsetMinX && maxX >= offsetMinX) ||
          (minX <= offsetMaxX && maxX >= offsetMaxX) || (minX >= offsetMinX && maxX <= offsetMaxX));
}

- (NSArray<UIView *> *)visibleItemViews {
  NSMutableArray<UIView *> *visibleViews = [NSMutableArray array];
  for (UIView *subview in [self.owner scrollViewForListItemHelper].subviews) {
    if (([[self class] isListItemWrapperView:subview] ||
         [[self class] isCUIListItemView:subview]) &&
        [self isVisibleItemView:subview]) {
      [visibleViews addObject:subview];
    }
  }
  [visibleViews
      sortUsingComparator:^NSComparisonResult(UIView *_Nonnull lhs, UIView *_Nonnull rhs) {
        LynxUIComponent *lhsComponent = [[self class] getComponentForListItemView:lhs];
        LynxUIComponent *rhsComponent = [[self class] getComponentForListItemView:rhs];
        NSInteger lhsPosition = [self indexForItemKey:lhsComponent.itemKey];
        NSInteger rhsPosition = [self indexForItemKey:rhsComponent.itemKey];
        if (lhsPosition < rhsPosition) {
          return NSOrderedAscending;
        }
        if (lhsPosition > rhsPosition) {
          return NSOrderedDescending;
        }
        return NSOrderedSame;
      }];

  return visibleViews;
}

- (NSArray<NSDictionary *> *)visibleItemInfo {
  UIScrollView *scrollView = [self.owner scrollViewForListItemHelper];
  NSMutableArray<NSDictionary *> *visibleItemInfo = [NSMutableArray array];
  for (UIView *itemView in self.visibleItemViews) {
    LynxUIComponent *holdingUI = [[self class] getComponentForListItemView:itemView];
    CGFloat cellTop = itemView.frame.origin.y - scrollView.contentOffset.y;
    CGFloat cellLeft = itemView.frame.origin.x - scrollView.contentOffset.x;
    NSInteger position = [self indexForItemKey:holdingUI.itemKey];
    [visibleItemInfo addObject:@{
      @"id" : (holdingUI.idSelector ?: @"unknown"),
      @"position" : @(position),
      @"index" : @(position),
      @"itemKey" : holdingUI.itemKey ?: @"",
      @"top" : @(cellTop),
      @"bottom" : @(cellTop + itemView.frame.size.height),
      @"left" : @(cellLeft),
      @"right" : @(cellLeft + itemView.frame.size.width),
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
    LynxUIComponent *holdingUI = [[self class] getComponentForListItemView:subview];
    if (holdingUI == nil) {
      continue;
    }
    CGPoint pointInCell = [holdingUI.view convertPoint:point fromView:scrollView];
    if ([holdingUI containsPoint:pointInCell inHitTestFrame:subview.bounds]) {
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
