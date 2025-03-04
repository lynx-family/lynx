// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "LUIBodyView.h"
#import "LynxRootUI.h"
#import "LynxUIOwner+Accessibility.h"
#import "UIView+Lynx.h"

#define LYNX_A11Y_TAG(pointer) ((NSInteger)pointer + 11)

@implementation LynxUIOwner (Accessibility)

- (void)addA11yMutation:(NSString *_Nonnull)action
                   sign:(NSNumber *_Nonnull)sign
                 a11yID:(NSString *_Nullable)a11yID
                toArray:(NSMutableArray *)array {
  [array addObject:@{@"target" : sign, @"action" : action, @"a11y-id" : (a11yID ?: @"")}];
}

- (void)addA11yPropsMutation:(NSString *_Nonnull)property
                        sign:(NSNumber *_Nonnull)sign
                      a11yID:(NSString *_Nullable)a11yID
                     toArray:(NSMutableArray *)array {
  if ([self.a11yFilter containsObject:property]) {
    [array addObject:@{
      @"target" : sign,
      @"action" : @"style_update",
      @"a11y-id" : (a11yID ?: @""),
      @"style" : property
    }];
  }
}

- (void)flushMutations:(NSMutableArray *)array withBodyView:(id<LUIBodyView>)lynxView {
  if (array.count) {
    [lynxView sendGlobalEvent:@"a11y-mutations" withParams:@[ [array copy] ]];
    [array removeAllObjects];
  }
}

- (void)listenAccessibilityFocused {
  if (@available(iOS 9.0, *)) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(lynxAccessibilityElementDidBecomeFocused:)
               name:UIAccessibilityElementFocusedNotification
             object:nil];
  }
}

- (void)lynxAccessibilityElementDidBecomeFocused:(NSNotification *)info {
  if (@available(iOS 9.0, *)) {
    if (self.rootUI.context.enableA11yIDMutationObserver) {
      UIView *view = info.userInfo[UIAccessibilityFocusedElementKey];
      if ([view isKindOfClass:UIView.class] && [view isDescendantOfView:self.rootUI.view]) {
        NSInteger sign = view.lynxSign.integerValue;
        LynxUI *ui = [self findUIBySign:sign];
        if (ui) {
          [self.rootUI.view sendGlobalEvent:@"activeElement"
                                 withParams:@[ @{
                                   @"a11y-id" : ui.a11yID ?: @"unknown",
                                   @"element-id" : @(ui.sign) ?: @"unknown"
                                 } ]];
        }
      }
    }
  }
}

- (NSSet<NSString *> *)a11yFilter {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setA11yFilter:(NSSet<NSString *> *)filter {
  objc_setAssociatedObject(self, @selector(a11yFilter), filter, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

/**
 * An accessibility element can not have any child accessibility element, by default.
 * The behavior on iOS and Android are totally different.
 * This function will create an dummy accessibility element and insert it at the backend of the
 * parent accessibility element to make the `nested` a11y elements possible.
 */
- (BOOL)checkNestedAccessibilityElements:(LynxUI *)subTree {
  __block BOOL childrenHasAccessibilityElements = NO;

  // handle nested recursively
  [subTree.children
      enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL *_Nonnull stop) {
        LynxUI *child = (LynxUI *)obj;
        if ([child isKindOfClass:LynxUI.class]) {
          if ([self checkNestedAccessibilityElements:child]) {
            childrenHasAccessibilityElements = YES;
          }
        }
      }];

  // use the pointer address to mark the dummy view
  UIView *dummyAccessibilityView = [subTree.view viewWithTag:LYNX_A11Y_TAG(subTree)];

  // create a dummy accessibility element to enable the nested accessibility
  if (childrenHasAccessibilityElements && subTree.view.isAccessibilityElement &&
      !dummyAccessibilityView) {
    // nested a11y elements found, create the dummy one
    dummyAccessibilityView = [[UIView alloc] initWithFrame:subTree.view.bounds];
    dummyAccessibilityView.tag = LYNX_A11Y_TAG(subTree);
  } else if (!childrenHasAccessibilityElements && subTree.view.isAccessibilityElement) {
    // nested a11y elements no longer exisits
    [dummyAccessibilityView removeFromSuperview];
    dummyAccessibilityView = nil;
  }

  if (dummyAccessibilityView) {
    // disable the parent a11y element and insert the dummy one to the bottom
    subTree.view.isAccessibilityElement = NO;
    dummyAccessibilityView.isAccessibilityElement = YES;
    dummyAccessibilityView.frame = subTree.view.bounds;
    dummyAccessibilityView.accessibilityLabel = subTree.view.accessibilityLabel;
    dummyAccessibilityView.accessibilityTraits = subTree.view.accessibilityTraits;
    [subTree.view insertSubview:dummyAccessibilityView atIndex:0];
  }

  return childrenHasAccessibilityElements || subTree.view.isAccessibilityElement;
}

@end
