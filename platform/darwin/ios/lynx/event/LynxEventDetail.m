// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEventDetail.h"
#import "LynxTouchEvent.h"

@implementation LynxEventDetail

- (nullable instancetype)initWithEvent:(nonnull LynxEvent*)event
                                target:(nullable id<LynxEventTargetBase>)target
                              lynxView:(nullable LynxView*)lynxView {
  self = [super init];
  if (self) {
    _event = event;
    _eventTarget = target;
    _lynxView = lynxView;
    _uiEvent = nil;
  }
  return self;
}

- (nullable NSString*)eventName {
  return _event.eventName;
}

- (enum EVENT_TYPE)eventType {
  switch (_event.eventType) {
    case kTouchEvent:
      return TOUCH_EVENT;
    default:
      return CUSTOM_EVENT;
  }
}

- (CGPoint)targetPoint {
  if (_event.eventType != kTouchEvent) {
    return CGPointZero;
  }
  return ((LynxTouchEvent*)_event).viewPoint;
}

- (BOOL)isMultiTouch {
  if (_event.eventType != kTouchEvent || !((LynxTouchEvent*)_event).isMultiTouch) {
    return NO;
  }
  return YES;
}

- (nullable NSDictionary*)targetPointMap {
  if (_event.eventType != kTouchEvent || !((LynxTouchEvent*)_event).isMultiTouch) {
    return nil;
  }
  return ((LynxTouchEvent*)_event).touchMap;
}

- (nullable NSDictionary*)params {
  if (_event.eventType != kCustomEvent) {
    return nil;
  }
  return [(LynxCustomEvent*)_event generateEventBody];
}

@end
