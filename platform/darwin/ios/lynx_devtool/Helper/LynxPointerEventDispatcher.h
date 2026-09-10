// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

// Mirrors lynx::devtool::input::PointerEventType. Only Down/Up/Cancel are driven
// by tap gestures today; Move and Scroll are part of the typed contract and are
// reserved for future scroll support.
typedef NS_ENUM(NSInteger, LynxDevToolPointerEventType) {
  LynxDevToolPointerEventTypeDown,
  LynxDevToolPointerEventTypeMove,
  LynxDevToolPointerEventTypeUp,
  LynxDevToolPointerEventTypeCancel,
  LynxDevToolPointerEventTypeScroll,
};

/**
 * Dispatches synthetic pointer sequences to the LynxView's window as UIKit
 * touch events. It is the sole owner of the injected UITouch/UIEvent and of the
 * active pointer sequence.
 *
 * Threading contract: attachment, capability checks, and event injection must
 * run on the main thread. Cancellation may be requested from any thread and
 * completes synchronously on the main thread. An owner that may release the
 * dispatcher off the main thread must cancel the active sequence first.
 */
@interface LynxPointerEventDispatcher : NSObject

// Live synthetic touch and its backing event for the active sequence. The
// optional platform-specific injection implementation manages their lifecycle.
@property(nonatomic, strong, nullable) UITouch *activeTouch;
@property(nonatomic, strong, nullable) UIEvent *activeEvent;

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view;

// Reattaches to a new LynxView, cancelling any in-flight sequence first.
- (void)attachLynxView:(nullable LynxView *)lynxView;

// Returns YES when the platform-specific injection path is linked and available.
- (BOOL)isPointerEventInjectionAvailable;

// Injects one pointer event. Coordinates are UIWindow logical points, matching
// the CDP Input domain contract. deltaX/deltaY/modifiers/timestampUs are carried
// to match the native InputEventTarget signature and are reserved for future
// scroll support; tap ignores them. Returns YES when the event was dispatched.
- (BOOL)injectPointerEvent:(LynxDevToolPointerEventType)type
               coordinateX:(CGFloat)x
               coordinateY:(CGFloat)y
                    deltaX:(CGFloat)deltaX
                    deltaY:(CGFloat)deltaY
                 pointerId:(int32_t)pointerId
                 modifiers:(int32_t)modifiers
               timestampUs:(int64_t)timestampUs;

// Cancels the active sequence, sending a cancel touch when one is in flight.
// This method may be called from any thread.
- (void)cancelCurrentPointerSequence;

@end

NS_ASSUME_NONNULL_END
