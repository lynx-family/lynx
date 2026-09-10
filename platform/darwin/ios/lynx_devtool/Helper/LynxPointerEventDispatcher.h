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
 * Dispatches synthetic pointer sequences to the LynxView's window as real UIKit
 * touches. It is the sole owner of the injected UITouch/UIEvent and of the
 * active pointer sequence, mirroring the Android PointerEventDispatcher.
 *
 * Threading contract: every public method must be invoked on the main thread.
 * UIKit event dispatch requires the main thread and the mutable sequence state
 * below is intentionally unsynchronized. When the native InputEventTarget drives
 * injection, the owning SyntheticGestureController must therefore be created with
 * the UI task runner so its callbacks land on this thread.
 */
@interface LynxPointerEventDispatcher : NSObject

// Live synthetic touch and its backing event for the active sequence. Shared
// with the internal injection category, which owns their UIKit lifecycle.
@property(nonatomic, strong, nullable) UITouch *activeTouch;
@property(nonatomic, strong, nullable) UIEvent *activeEvent;

- (nonnull instancetype)initWithLynxView:(nullable LynxView *)view;

// Reattaches to a new LynxView, cancelling any in-flight sequence first.
- (void)attachLynxView:(nullable LynxView *)lynxView;

// Returns YES when the private UIKit injection path is linked and available.
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
- (void)cancelCurrentPointerSequence;

@end

NS_ASSUME_NONNULL_END
