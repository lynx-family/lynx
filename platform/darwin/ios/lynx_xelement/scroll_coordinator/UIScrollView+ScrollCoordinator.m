// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/NSObject+LynxScrollCoordinatorKVO.h>
#import <XElement/UIScrollView+ScrollCoordinator.h>
#import <objc/runtime.h>

static void LynxScrollCoordinatorSwizzleInstanceMethod(Class cls, SEL originalSelector,
                                                       SEL swizzledSelector) {
  Method originalMethod = class_getInstanceMethod(cls, originalSelector);
  Method swizzledMethod = class_getInstanceMethod(cls, swizzledSelector);
  BOOL didAddMethod =
      class_addMethod(cls, originalSelector, method_getImplementation(swizzledMethod),
                      method_getTypeEncoding(swizzledMethod));
  if (didAddMethod) {
    class_replaceMethod(cls, swizzledSelector, method_getImplementation(originalMethod),
                        method_getTypeEncoding(originalMethod));
  } else {
    method_exchangeImplementations(originalMethod, swizzledMethod);
  }
}

@implementation UIScrollView (ScrollCoordinator)

- (void)scrollCoordinator_dealloc {
  // Remove only the observer registered by this helper to keep dealloc lightweight.
  [self lynx_scrollCoordinator_removeObserverBlocksForKeyPath:@"contentOffset"];
  [self scrollCoordinator_dealloc];
}

- (void)scrollCoordinator_addObserverBlockForKeyPath:(NSString *)keyPath
                                               block:(void (^)(__weak id obj, id oldVal,
                                                               id newVal))block {
  if (@available(iOS 11.0, *)) {
    // KVO restore weak reference after iOS 11
  } else {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
      LynxScrollCoordinatorSwizzleInstanceMethod(UIScrollView.class, sel_registerName("dealloc"),
                                                 @selector(scrollCoordinator_dealloc));
    });
  }
  [self lynx_scrollCoordinator_removeObserverBlocksForKeyPath:keyPath];
  [self lynx_scrollCoordinator_addObserverBlockForKeyPath:keyPath block:block];
}

- (void)scrollCoordinator_removeObserverBlocksForKeyPath:(NSString *)keyPath {
  [self lynx_scrollCoordinator_removeObserverBlocksForKeyPath:keyPath];
}

@end
