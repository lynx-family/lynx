// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxPointerEventDispatcher.h>
#include <cmath>

// Implemented by the internal injection category in the root platform tree. When
// that category is not linked (open-source builds without the private UIKit
// helpers), isPointerEventInjectionAvailable returns NO and injection is skipped.
@interface LynxPointerEventDispatcher (InternalPointerInjection)

- (BOOL)lynx_isPointerEventInjectionAvailable;
- (BOOL)lynx_dispatchPointerEvent:(LynxDevToolPointerEventType)type
                         inWindow:(UIWindow *)window
                        hitTarget:(nullable UIView *)target
                       coordinate:(CGPoint)point;
- (void)lynx_cancelCurrentPointerSequence;

@end

@interface LynxPointerEventDispatcher ()

@property(nonatomic, weak, nullable) LynxView *lynxView;
@property(nonatomic, assign) BOOL pointerSequenceActive;
@property(nonatomic, assign) int32_t activePointerId;
@property(nonatomic, weak, nullable) UIWindow *activePointerWindow;

@end

#pragma mark - LynxPointerEventDispatcher
@implementation LynxPointerEventDispatcher

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view {
  self = [super init];
  if (self != nil) {
    _lynxView = view;
  }
  return self;
}

- (void)dealloc {
  if ([NSThread isMainThread]) {
    [self cancelCurrentPointerSequence];
  }
}

- (void)attachLynxView:(nullable LynxView *)lynxView {
  [self cancelCurrentPointerSequence];
  _lynxView = lynxView;
}

- (BOOL)isPointerEventInjectionAvailable {
  return [self respondsToSelector:@selector(lynx_isPointerEventInjectionAvailable)] &&
         [self lynx_isPointerEventInjectionAvailable];
}

- (BOOL)injectPointerEvent:(LynxDevToolPointerEventType)type
               coordinateX:(CGFloat)x
               coordinateY:(CGFloat)y
                    deltaX:(CGFloat)deltaX
                    deltaY:(CGFloat)deltaY
                 pointerId:(int32_t)pointerId
                 modifiers:(int32_t)modifiers
               timestampUs:(int64_t)timestampUs {
  // deltaX/deltaY/modifiers/timestampUs are unused for tap down/up/cancel; they
  // are kept to match the native InputEventTarget signature and reserved for
  // future scroll support (LynxDevToolPointerEventTypeScroll).
  if (![NSThread isMainThread]) {
    return NO;
  }
  if (![self isPointerEventInjectionAvailable] || !std::isfinite(x) || !std::isfinite(y)) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }

  __strong typeof(self.lynxView) lynxView = self.lynxView;
  UIWindow *window = lynxView.window;
  if (self.pointerSequenceActive && (window == nil || window != self.activePointerWindow)) {
    [self cancelCurrentPointerSequence];
    return NO;
  }
  if (window == nil) {
    return NO;
  }

  CGPoint point = CGPointMake(x, y);
  if (CGRectIsEmpty(window.bounds) || !CGRectContainsPoint(window.bounds, point)) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }

  UIView *target = [window hitTest:point withEvent:nil];
  if (type == LynxDevToolPointerEventTypeDown) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
      return NO;
    }
    if (target == nil || ![self lynx_dispatchPointerEvent:type
                                                 inWindow:window
                                                hitTarget:target
                                               coordinate:point]) {
      return NO;
    }
    self.pointerSequenceActive = YES;
    self.activePointerId = pointerId;
    self.activePointerWindow = window;
    return YES;
  }

  if (!self.pointerSequenceActive || pointerId != self.activePointerId) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }

  BOOL injected = [self lynx_dispatchPointerEvent:type
                                         inWindow:window
                                        hitTarget:target
                                       coordinate:point];
  if (!injected) {
    [self cancelCurrentPointerSequence];
  } else if (type == LynxDevToolPointerEventTypeUp || type == LynxDevToolPointerEventTypeCancel) {
    self.pointerSequenceActive = NO;
    self.activePointerId = 0;
    self.activePointerWindow = nil;
  }
  return injected;
}

- (void)cancelCurrentPointerSequence {
  if (![NSThread isMainThread]) {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [self cancelCurrentPointerSequence];
    });
    return;
  }

  if (self.pointerSequenceActive || self.activeTouch != nil || self.activeEvent != nil) {
    if ([self respondsToSelector:@selector(lynx_cancelCurrentPointerSequence)]) {
      [self lynx_cancelCurrentPointerSequence];
    }
  }
  self.pointerSequenceActive = NO;
  self.activePointerId = 0;
  self.activePointerWindow = nil;
}

@end
