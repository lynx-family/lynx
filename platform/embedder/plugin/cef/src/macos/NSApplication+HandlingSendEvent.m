// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "NSApplication+HandlingSendEvent.h"

@implementation NSApplication (HandlingSendEvent)

static const void *kIsHandlingSendEventKey = &kIsHandlingSendEventKey;

- (BOOL)isHandlingSendEvent {
  NSNumber *number = objc_getAssociatedObject(self, kIsHandlingSendEventKey);
  if (number) {
    return [number boolValue];
  }
  return NO;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  objc_setAssociatedObject(self, kIsHandlingSendEventKey, @(handlingSendEvent),
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end
