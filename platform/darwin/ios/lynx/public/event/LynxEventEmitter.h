// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEvent.h"
#import "LynxEventTarget.h"
#import "LynxTouchEvent.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxEngineProxy;

typedef NS_ENUM(NSInteger, LynxInnerEventType) {
  LynxEventTypeTouchEvent,
  LynxEventTypeCustomEvent,
  LynxEventTypeLayoutEvent,
};

typedef BOOL (^onLynxEvent)(LynxEvent *event);

@protocol LynxEventObserver

- (void)onLynxEvent:(LynxInnerEventType)type event:(LynxEvent *)event;

@end

/**
 * Emit event to front-end
 */
@interface LynxEventEmitter : NSObject

- (instancetype)initWithLynxEngineProxy:(LynxEngineProxy *)engineProxy;

- (void)setEventReporterBlock:(onLynxEvent)eventReporter;
- (void)setIntersectionObserverBlock:(dispatch_block_t)intersectionObserver;

// The return value indicates whether the client intercepted the event.
- (BOOL)dispatchTouchEvent:(LynxTouchEvent *)event;

- (void)dispatchMultiTouchEvent:(LynxTouchEvent *)event;

- (void)dispatchCustomEvent:(LynxCustomEvent *)event;

- (void)sendCustomEvent:(LynxCustomEvent *)event;

- (BOOL)onLynxEvent:(LynxEvent *)detail;

- (void)dispatchGestureEvent:(int)gestureId event:(LynxCustomEvent *)event;

// TODO(songshourui.null): use this interface to handle touch status change
- (void)onPseudoStatusChanged:(int32_t)tag
                fromPreStatus:(int32_t)preStatus
              toCurrentStatus:(int32_t)currentStatus;

- (void)dispatchLayoutEvent;

- (void)addObserver:(id<LynxEventObserver>)observer;
- (void)removeObserver:(id<LynxEventObserver>)observer;
- (void)notifyIntersectionObserver;

@end

NS_ASSUME_NONNULL_END
