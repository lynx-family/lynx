// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUI+Accessibility.h"
#import "LynxUIMethodProcessor.h"
#import "LynxUIOwner.h"
#import "LynxView.h"
#import "LynxWeakProxy.h"

@implementation LynxUI (Accessibility)

- (void)handleAccessibility:(UIView*)accessibilityAttachedCell
                 autoScroll:(BOOL)accessibilityAutoScroll {
  if (UIAccessibilityIsVoiceOverRunning() && self.accessibilityElementsA11yIds) {
    // find the UIView with a11y-id
    NSArray* customAccessibilityElements = [self accessibilityElementsWithA11yID];
    [self updateAccessibilityElements:customAccessibilityElements
                         attachedCell:accessibilityAttachedCell
                           autoScroll:accessibilityAutoScroll];

  } else if (UIAccessibilityIsVoiceOverRunning() && self.accessibilityElementsIds) {
    // find the UIView with DOM id
    NSMutableArray* customAccessibilityElements = [NSMutableArray array];
    for (NSString* uiID in self.accessibilityElementsIds) {
      UIView* view = [self.context.uiOwner uiWithIdSelector:uiID].view;
      if (view.isAccessibilityElement) {
        [customAccessibilityElements addObject:view];
      }
    }
    [self updateAccessibilityElements:customAccessibilityElements
                         attachedCell:accessibilityAttachedCell
                           autoScroll:accessibilityAutoScroll];
  } else {
    if (accessibilityAutoScroll) {
      if (@available(iOS 9.0, *)) {
        [[NSNotificationCenter defaultCenter]
            removeObserver:self
                      name:UIAccessibilityElementFocusedNotification
                    object:nil];
      }
    }
  }
}

/**
 While `accessibility-elements-a11y` contains both `sign` and `a11y-id`:
 1. find UIs with `sign`
 2. find UIs with `a11y-id`
   2.1 filter out the ui was already there in the step 1
   2.2 if there are more than one ui which have the same `a11y-id`, they should be ordered by the
 Dom flow
 3. reorder the UIs by the `accessibility-elements-a11y`
 */
- (NSArray*)accessibilityElementsWithA11yID {
  NSMutableDictionary<NSNumber*, UIView*>* viewFoundBySign = [NSMutableDictionary dictionary];
  NSMutableDictionary<NSString*, NSMutableArray<UIView*>*>* viewFoundByA11y =
      [NSMutableDictionary dictionary];
  NSMutableDictionary<NSString*, NSNumber*>* viewFoundByA11yCount =
      [NSMutableDictionary dictionary];
  NSMutableArray* orderedList = [NSMutableArray array];

  // Find out all the UIs with `sign` and `a11y-id`
  for (NSString* uiID in self.accessibilityElementsA11yIds) {
    NSScanner* scanner = [NSScanner scannerWithString:uiID];
    if ([scanner scanInt:NULL] && [scanner isAtEnd]) {
      // If it is a number, we treat it as the sign
      UIView* view = [self.context.uiOwner findUIBySign:[uiID intValue]].view;
      if (view) {
        viewFoundBySign[@([uiID intValue])] = view;
      }
      [orderedList addObject:@([uiID intValue])];
    } else {
      // If it is a string, we treat it as a a11y-id
      if (!viewFoundByA11y[uiID]) {
        NSArray<UIView*>* views = [self.context.uiOwner viewsWithA11yID:uiID];
        viewFoundByA11y[uiID] = [views mutableCopy];
      }
      NSInteger number = [viewFoundByA11yCount[uiID] integerValue];
      number++;
      viewFoundByA11yCount[uiID] = @(number);
      [orderedList addObject:uiID];
    }
  }

  UIView* rootView = self.context.rootView;

  [viewFoundByA11y
      enumerateKeysAndObjectsUsingBlock:^(
          NSString* _Nonnull key, NSMutableArray<UIView*>* _Nonnull a11yList, BOOL* _Nonnull stop) {
        // Step 2.1, get rid of the views which is already found by the sign
        [viewFoundBySign enumerateKeysAndObjectsUsingBlock:^(NSNumber* _Nonnull key,
                                                             UIView* _Nonnull singleViewFoundBySign,
                                                             BOOL* _Nonnull stop) {
          if ([a11yList containsObject:singleViewFoundBySign]) {
            [a11yList removeObject:singleViewFoundBySign];
          }
        }];

        // Step 2.2, reorder the a11yList by DOM-order
        [a11yList sortUsingComparator:^NSComparisonResult(UIView* obj1, UIView* obj2) {
          CGRect frame1 = [obj1 convertRect:obj1.bounds toView:rootView];
          CGRect frame2 = [obj2 convertRect:obj2.bounds toView:rootView];
          if (frame1.origin.y < frame2.origin.y) {
            return NSOrderedAscending;
          } else if (frame1.origin.y > frame2.origin.y) {
            return NSOrderedDescending;
          } else {
            return frame1.origin.x < frame2.origin.x ? NSOrderedAscending : NSOrderedDescending;
          }
        }];
      }];

  NSMutableArray* customAccessibilityElements = [NSMutableArray array];
  // Step 3, order all the UIs by the `accessibility-elements-a11y`
  [orderedList enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
    if ([obj isKindOfClass:NSNumber.class]) {
      // It is a ui with `sign`, which is described as Step 1
      UIView* view = viewFoundBySign[obj];
      if (view) {
        [customAccessibilityElements addObject:view];
      }
    } else {
      // There are the UIs with same `a11y-id`, which is described as Step 2
      NSMutableArray<UIView*>* a11yList = viewFoundByA11y[obj];
      NSInteger left = [viewFoundByA11yCount[obj] integerValue];
      left--;
      if (a11yList.count) {
        if (left > 0) {
          [customAccessibilityElements addObject:[a11yList firstObject]];
          [a11yList removeObjectAtIndex:0];
        } else {
          [customAccessibilityElements addObjectsFromArray:a11yList];
          [a11yList removeAllObjects];
        }
      }
    }
  }];
  return customAccessibilityElements;
}

- (void)updateAccessibilityElements:(NSArray*)customAccessibilityElements
                       attachedCell:(UIView*)accessibilityAttachedCell
                         autoScroll:(BOOL)accessibilityAutoScroll {
  // If we are in a cell, it is needed to set accessibilityElements to the cell
  UIView* cell = accessibilityAttachedCell;
  if (cell) {
    cell.accessibilityElements = customAccessibilityElements;
  } else {
    self.view.accessibilityElements = customAccessibilityElements;
  }
  if (accessibilityAutoScroll) {
    if (@available(iOS 9.0, *)) {
      [[NSNotificationCenter defaultCenter] removeObserver:self
                                                      name:UIAccessibilityElementFocusedNotification
                                                    object:nil];
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundeclared-selector"
      [[NSNotificationCenter defaultCenter] addObserver:self
                                               selector:@selector(autoScrollIfFocusedChanged:)
                                                   name:UIAccessibilityElementFocusedNotification
                                                 object:nil];
#pragma clang diagnostic pop
    }
  }
}

LYNX_UI_METHOD(requestAccessibilityFocus) {
  UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification,
                                  params[@"withoutUpdate"] ? nil : [self accessibilityFocusedView]);
  if (callback) {
    callback(kUIMethodSuccess, nil);
  }
}

LYNX_UI_METHOD(fetchAccessibilityTargets) {
  NSArray* array = [self accessibilityTargets];
  if (callback) {
    callback(kUIMethodSuccess, array);
  }
}

LYNX_UI_METHOD(innerText) {
  if (callback) {
    callback(kUIMethodSuccess, [self findInnerText]);
  }
}

- (NSArray*)accessibilityTargets {
  NSMutableArray* ret = [NSMutableArray array];
  [ret addObject:@{@"element-id" : @(self.sign), @"a11y-id" : self.a11yID ?: @"unknown"}];
  [self.children
      enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        NSArray* array = [obj accessibilityTargets];
        [ret addObjectsFromArray:array];
      }];
  return ret;
}

- (NSArray<NSString*>*)findInnerText {
  NSMutableArray* ret = [NSMutableArray array];
  NSString* text = [self accessibilityText];
  if (text) {
    [ret addObject:text];
  }
  [self.children
      enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        NSArray<NSString*>* subTextArray = [obj findInnerText];
        [ret addObjectsFromArray:subTextArray];
      }];
  return ret;
}

- (NSString*)accessibilityText {
  return nil;
}

- (UIView*)accessibilityFocusedView {
  return self.view;
}

/**
 * Mark all the a11y elements invisible, except the nodes inside the sub-tree
 */
- (NSArray*)setExclusiveAccessibilityElements:(BOOL)exclusive
                                      subTree:(LynxUI*)subTree
                                previousNodes:(NSArray*)previousNodes {
  // clear previous `accessibility-hidden` nodes which have been set as
  // `accessibilityElementsHidden`
  [self clearExclusiveAccessibilityElements:previousNodes];

  if (exclusive) {
    // enumrate from current node to the root, mark all the exclueded node as
    // `accessibilityElementsHidden`
    NSMutableArray<LynxWeakProxy*>* accessibilityExclusiveFocusedNodes = [NSMutableArray array];
    while (subTree) {
      LynxUI* parent = subTree.parent;
      [parent.children
          enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
            if (obj != subTree && [obj isKindOfClass:LynxUI.class]) {
              obj.view.accessibilityElementsHidden = exclusive;
              [accessibilityExclusiveFocusedNodes addObject:[LynxWeakProxy proxyWithTarget:obj]];
            }
          }];
      subTree = parent;
    }

    // update focus
    UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                    self.context.rootView);

    // return current excluded nodes
    return accessibilityExclusiveFocusedNodes;
  } else {
    UIAccessibilityPostNotification(UIAccessibilityScreenChangedNotification,
                                    self.context.rootView);
    return nil;
  }
}

- (void)clearExclusiveAccessibilityElements:(NSArray*)nodes {
  [nodes enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
    LynxWeakProxy* proxy = (LynxWeakProxy*)obj;
    if ([proxy.target isKindOfClass:LynxUI.class]) {
      ((LynxUI*)(proxy.target)).view.accessibilityElementsHidden = NO;
    }
  }];
}

@end
