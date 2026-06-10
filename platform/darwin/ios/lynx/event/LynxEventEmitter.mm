// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxEventDetail.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import "LynxUIIntersectionObserver.h"

#include <limits>

using namespace lynx::tasm;
using namespace lynx::lepus;

static constexpr int64_t kCurrentLynxPageOnlyEventID = std::numeric_limits<int64_t>::min();

@interface LynxEventEmitter ()
- (BOOL)consumeDispatchInCurrentLynxPageOnly:(LynxTouchEvent*)event;
@end

@implementation LynxEventEmitter {
  LynxEngineProxy* _engineProxy;
  NSMutableArray* eventObservers_;
  onLynxEvent eventReporter_;
  dispatch_block_t intersectionObserver_;
  int64_t eventID_;
}

- (instancetype)initWithLynxEngineProxy:(LynxEngineProxy*)engineProxy {
  self = [super init];
  if (self) {
    eventObservers_ = [[NSMutableArray alloc] init];
    _engineProxy = engineProxy;
    eventID_ = 0;
  }
  return self;
}

- (void)setEventReporterBlock:(onLynxEvent)eventReporter {
  eventReporter_ = eventReporter;
}

- (void)setIntersectionObserverBlock:(dispatch_block_t)intersectionObserver {
  intersectionObserver_ = intersectionObserver;
}

- (BOOL)dispatchTouchEvent:(LynxTouchEvent*)event {
  if (_engineProxy == nil) {
    _LogE(@"dispatchTouchEvent event: %@ failed since engineProxy is nil", event.eventName);
    return NO;
  }
  BOOL dispatchInCurrentLynxPageOnly = [self consumeDispatchInCurrentLynxPageOnly:event];
  if ([self onLynxEvent:event]) {
    return YES;
  }
  id<LynxEventTarget> target = (id<LynxEventTarget>)event.eventTarget;
  BOOL hasParentLynxPageUI = !dispatchInCurrentLynxPageOnly && [target parentLynxPageUI] != nil;
  NSMutableDictionary* childrenLynxPageUI = [target childrenLynxPageUI];
  BOOL hasTargetChildLynxPageUI =
      childrenLynxPageUI[[NSString stringWithFormat:@"%p", target]] != nil;
  BOOL shouldDispatchThroughLynxPage = dispatchInCurrentLynxPageOnly
                                           ? hasTargetChildLynxPageUI
                                           : hasParentLynxPageUI || childrenLynxPageUI != nil;
  if (shouldDispatchThroughLynxPage) {
    if (!hasParentLynxPageUI) {
      eventID_ = (eventID_ + 1) % (std::numeric_limits<int64_t>::max() - 1);
    }
    event.eventID = eventID_;
    [target setEventID:eventID_];
    _LogI(@"LynxEventEmitter TouchEventHandler %@ %lld %p", event.eventName, (long long)eventID_,
          self);
    [self startEventGenerate:event];

    if (!hasTargetChildLynxPageUI) {
      [target.rootLynxPageUI startEventCapture:eventID_];
    }
  } else {
    [_engineProxy sendSyncTouchEvent:event];
  }
  return NO;
}

- (BOOL)onLynxEvent:(LynxEvent*)detail {
  if (eventReporter_ == nil) {
    _LogE(@"onLynxEvent event: %@ failed since eventReporter is nil", detail.eventName);
    return NO;
  }
  return eventReporter_(detail);
}

- (void)dispatchMultiTouchEvent:(LynxTouchEvent*)event {
  if (_engineProxy == nil) {
    LLogError(@"dispatchMultiTouchEvent event: %@ failed since engineProxy is nil",
              event.eventName);
    return;
  }
  BOOL dispatchInCurrentLynxPageOnly = [self consumeDispatchInCurrentLynxPageOnly:event];
  if ([self onLynxEvent:event]) {
    return;
  }
  id<LynxEventTarget> target = (id<LynxEventTarget>)event.eventTarget;
  BOOL hasParentLynxPageUI = !dispatchInCurrentLynxPageOnly && [target parentLynxPageUI] != nil;
  NSMutableDictionary* childrenLynxPageUI = [target childrenLynxPageUI];
  BOOL hasTargetChildLynxPageUI =
      childrenLynxPageUI[[NSString stringWithFormat:@"%p", target]] != nil;
  BOOL shouldDispatchThroughLynxPage = dispatchInCurrentLynxPageOnly
                                           ? hasTargetChildLynxPageUI
                                           : hasParentLynxPageUI || childrenLynxPageUI != nil;
  if (shouldDispatchThroughLynxPage) {
    if (!hasParentLynxPageUI) {
      eventID_ = (eventID_ + 1) % (std::numeric_limits<int64_t>::max() - 1);
    }
    event.eventID = eventID_;
    [target setEventID:eventID_];
    _LogI(@"LynxEventEmitter TouchEventHandler %@ %lld %p", event.eventName, (long long)eventID_,
          self);
    [self startEventGenerate:event];

    if (!hasTargetChildLynxPageUI) {
      [target.rootLynxPageUI startEventCapture:eventID_];
    }
  } else {
    [_engineProxy sendSyncMultiTouchEvent:event];
  }
}

- (void)dispatchCustomEvent:(LynxCustomEvent*)event {
  if (_engineProxy == nil) {
    _LogE(@"dispatchCustomEvent event: %@ failed since engineProxy is nil", event.eventName);
    return;
  }
  if ([self onLynxEvent:event]) {
    return;
  }
  [event addDetailKey:@"timestamp" value:@((int64_t)(event.timestamp * 1000))];
  [_engineProxy sendCustomEvent:event];
  [self notifyEventObservers:LynxEventTypeCustomEvent event:event];
}

// dispatch gesture event to template render
- (void)dispatchGestureEvent:(int)gestureId event:(LynxCustomEvent*)event {
  if (_engineProxy == nil) {
    _LogE(@"dispatchGestureEvent event: %@ failed since engineProxy is nil", event.eventName);
    return;
  }
  [_engineProxy sendGestureEvent:gestureId event:event];
}

- (void)sendCustomEvent:(LynxCustomEvent*)event {
  [self dispatchCustomEvent:event];
}

- (void)onPseudoStatusChanged:(int32_t)tag
                fromPreStatus:(int32_t)preStatus
              toCurrentStatus:(int32_t)currentStatus {
  if (_engineProxy == nil) {
    _LogE(@"onPseudoStatusChanged id: %d failed since engineProxy is nil", tag);
    return;
  }
  if (preStatus == currentStatus) {
    return;
  }
  [_engineProxy onPseudoStatusChanged:tag fromPreStatus:preStatus toCurrentStatus:currentStatus];
}

- (void)dispatchLayoutEvent {
  [self notifyEventObservers:LynxEventTypeLayoutEvent event:nil];
}

- (void)addObserver:(id<LynxEventObserver>)observer {
  if (![eventObservers_ containsObject:observer]) {
    [eventObservers_ addObject:observer];
  }
}

- (void)removeObserver:(id<LynxEventObserver>)observer {
  if ([eventObservers_ containsObject:observer]) {
    [eventObservers_ removeObject:observer];
  }
}

- (void)notifyEventObservers:(LynxInnerEventType)type event:(LynxEvent*)event {
  [eventObservers_
      enumerateObjectsUsingBlock:^(id _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        // if use new IntersectionObserver, don't notify observer here.
        if ([obj isKindOfClass:[LynxUIIntersectionObserverManager class]] &&
            ((LynxUIIntersectionObserverManager*)obj).enableNewIntersectionObserver) {
          return;
        }
        [(id<LynxEventObserver>(obj)) onLynxEvent:type event:event];
      }];
}

- (void)notifyIntersectionObserver {
  if (strcmp(dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL),
             dispatch_queue_get_label(dispatch_get_main_queue())) == 0) {
    // if on main thread, exec block
    intersectionObserver_();
  } else {
    // if not on main thread, post to main thread
    dispatch_async(dispatch_get_main_queue(), intersectionObserver_);
  }
}

- (void)startEventGenerate:(LynxEvent*)event {
  [_engineProxy startEventGenerate:(LynxTouchEvent*)event];
}

- (void)setEventID:(int64_t)eventID {
  eventID_ = eventID;
}

- (void)startEventCapture:(int64_t)eventID {
  [_engineProxy startEventCapture:eventID];
}

- (void)startEventBubble:(int64_t)eventID {
  [_engineProxy startEventBubble:eventID];
}

- (void)startEventFire:(BOOL)isStop withEventID:(int64_t)eventID {
  [_engineProxy startEventFire:isStop withEventID:eventID];
}

- (BOOL)consumeDispatchInCurrentLynxPageOnly:(LynxTouchEvent*)event {
  if (event.eventID != kCurrentLynxPageOnlyEventID) {
    return NO;
  }
  event.eventID = 0;
  return YES;
}

@end
