// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LynxPointerEventDispatcher.h>
#include <cmath>

// Implemented by an optional platform-specific category. When that implementation
// is not linked, isPointerEventInjectionAvailable returns NO and injection is
// skipped.
@interface LynxPointerEventDispatcher (InternalPointerInjection)

- (BOOL)lynx_isPointerEventInjectionAvailable;
- (BOOL)lynx_dispatchPointerEvent:(LynxDevToolPointerEventType)type
                         inWindow:(UIWindow *)window
                        hitTarget:(nullable UIView *)target
                       pointerIds:(NSArray<NSNumber *> *)pointerIds
                      coordinates:(NSArray<NSValue *> *)coordinates
                  actionPointerId:(int32_t)actionPointerId;
- (void)lynx_cancelCurrentPointerSequence;

@end

@interface LynxPointerEventDispatcher ()

@property(nonatomic, weak, nullable) LynxView *lynxView;
@property(nonatomic, assign) BOOL pointerSequenceActive;
@property(nonatomic, strong) NSMutableSet<NSNumber *> *activePointerIds;
@property(nonatomic, weak, nullable) UIWindow *activePointerWindow;

@end

#pragma mark - LynxPointerEventDispatcher
@implementation LynxPointerEventDispatcher

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view {
  self = [super init];
  if (self != nil) {
    _lynxView = view;
    _activePointerIds = [NSMutableSet set];
    _activeTouches = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)dealloc {
  if ([NSThread isMainThread]) {
    [self cancelCurrentPointerSequence];
  } else {
    NSAssert(
        !self.pointerSequenceActive && self.activeTouches.count == 0 && self.activeEvent == nil,
        @"Cancel the active pointer sequence before releasing the dispatcher off-main.");
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
                pointerIds:(NSArray<NSNumber *> *)pointerIds
               coordinates:(NSArray<NSValue *> *)coordinates
           actionPointerId:(int32_t)actionPointerId
                    deltaX:(CGFloat)deltaX
                    deltaY:(CGFloat)deltaY
                 modifiers:(int32_t)modifiers
               timestampUs:(int64_t)timestampUs {
  // deltaX/deltaY/modifiers/timestampUs are reserved for future scroll support.
  if (![NSThread isMainThread]) {
    return NO;
  }
  if (![self isPointerEventInjectionAvailable] || pointerIds.count == 0 || pointerIds.count > 2 ||
      pointerIds.count != coordinates.count ||
      [NSSet setWithArray:pointerIds].count != pointerIds.count ||
      ![pointerIds containsObject:@(actionPointerId)]) {
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

  if (CGRectIsEmpty(window.bounds)) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }
  for (NSValue *coordinate in coordinates) {
    CGPoint point = coordinate.CGPointValue;
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        !CGRectContainsPoint(window.bounds, point)) {
      if (self.pointerSequenceActive) {
        [self cancelCurrentPointerSequence];
      }
      return NO;
    }
  }

  NSSet<NSNumber *> *pointerIdSet = [NSSet setWithArray:pointerIds];
  BOOL validTransition = NO;
  if (type == LynxDevToolPointerEventTypeDown) {
    if (!self.pointerSequenceActive) {
      validTransition = pointerIds.count == 1;
    } else {
      NSMutableSet<NSNumber *> *expected = [self.activePointerIds mutableCopy];
      [expected addObject:@(actionPointerId)];
      validTransition = ![self.activePointerIds containsObject:@(actionPointerId)] &&
                        [expected isEqualToSet:pointerIdSet];
    }
  } else if (self.pointerSequenceActive && [self.activePointerIds isEqualToSet:pointerIdSet]) {
    validTransition = type == LynxDevToolPointerEventTypeMove ||
                      type == LynxDevToolPointerEventTypeUp ||
                      type == LynxDevToolPointerEventTypeCancel;
  }
  if (!validTransition) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }

  NSUInteger actionIndex = [pointerIds indexOfObject:@(actionPointerId)];
  CGPoint actionPoint = coordinates[actionIndex].CGPointValue;
  UIView *target =
      type == LynxDevToolPointerEventTypeDown ? [window hitTest:actionPoint withEvent:nil] : nil;
  if (type == LynxDevToolPointerEventTypeDown && target == nil) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }

  BOOL injected = [self lynx_dispatchPointerEvent:type
                                         inWindow:window
                                        hitTarget:target
                                       pointerIds:pointerIds
                                      coordinates:coordinates
                                  actionPointerId:actionPointerId];
  if (!injected) {
    if (self.pointerSequenceActive) {
      [self cancelCurrentPointerSequence];
    }
    return NO;
  }

  if (type == LynxDevToolPointerEventTypeDown) {
    self.pointerSequenceActive = YES;
    [self.activePointerIds addObject:@(actionPointerId)];
    self.activePointerWindow = window;
  } else if (type == LynxDevToolPointerEventTypeUp) {
    [self.activePointerIds removeObject:@(actionPointerId)];
    if (self.activePointerIds.count == 0) {
      self.pointerSequenceActive = NO;
      self.activePointerWindow = nil;
    }
  } else if (type == LynxDevToolPointerEventTypeCancel) {
    self.pointerSequenceActive = NO;
    [self.activePointerIds removeAllObjects];
    self.activePointerWindow = nil;
  }
  return YES;
}

- (void)cancelCurrentPointerSequence {
  if (![NSThread isMainThread]) {
    dispatch_sync(dispatch_get_main_queue(), ^{
      [self cancelCurrentPointerSequence];
    });
    return;
  }

  if (self.pointerSequenceActive || self.activeTouches.count != 0 || self.activeEvent != nil) {
    if ([self respondsToSelector:@selector(lynx_cancelCurrentPointerSequence)]) {
      [self lynx_cancelCurrentPointerSequence];
    }
  }
  self.pointerSequenceActive = NO;
  [self.activePointerIds removeAllObjects];
  self.activePointerWindow = nil;
}

@end
