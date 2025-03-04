// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEvent.h"

int32_t const LynxTouchPseudoStateNone = 0;
int32_t const LynxTouchPseudoStateHover = 1;
int32_t const LynxTouchPseudoStateHoverTransition = 1 << 1;
int32_t const LynxTouchPseudoStateActive = 1 << 3;
int32_t const LynxTouchPseudoStateActiveTransition = 1 << 4;
int32_t const LynxTouchPseudoStateFocus = 1 << 6;
int32_t const LynxTouchPseudoStateFocusTransition = 1 << 7;
int32_t const LynxTouchPseudoStateAll = ~0;

@implementation LynxEvent

- (instancetype)initWithName:(NSString*)name type:(LynxEventType)type {
  self = [super init];
  if (self) {
    _eventName = name;
    _eventType = type;
    _timestamp = [[NSDate date] timeIntervalSince1970];
  }
  return self;
}

- (instancetype)initWithName:(NSString*)name type:(LynxEventType)type targetSign:(NSInteger)target {
  return [self initWithName:name type:type targetSign:target currentTargetSign:target];
}

- (instancetype)initWithName:(NSString*)name
                        type:(LynxEventType)type
                  targetSign:(NSInteger)target
           currentTargetSign:(NSInteger)currentTarget {
  self = [self initWithName:name type:type];
  if (self) {
    _currentTargetSign = currentTarget;
    _targetSign = target;
  }
  return self;
}

- (BOOL)canCoalesce {
  return NO;
}

- (NSMutableDictionary*)generateEventBody {
  NSMutableDictionary* body = [NSMutableDictionary new];
  body[@"type"] = _eventName;
  body[@"target"] = [NSNumber numberWithInteger:_targetSign];
  body[@"currentTarget"] = [NSNumber numberWithInteger:_currentTargetSign];
  return body;
}

@end

@implementation LynxCustomEvent

- (instancetype)initWithName:(NSString*)name targetSign:(NSInteger)target {
  return [super initWithName:name type:kCustomEvent targetSign:target];
}

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
                      params:(NSMutableDictionary*)params {
  return [self initWithName:name targetSign:target currentTargetSign:target params:params];
}

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
           currentTargetSign:(NSInteger)currentTarget
                      params:(NSMutableDictionary*)params {
  self = [super initWithName:name
                        type:kCustomEvent
                  targetSign:target
           currentTargetSign:currentTarget];
  if (self) {
    // TODO(hexionghui): Delete this later.
    _params = [params mutableCopy];
  }
  return self;
}

- (NSDictionary*)generateEventBody {
  NSMutableDictionary* body = [super generateEventBody];
  body[[self paramsName]] = _params;
  return body;
}

/**
 * On front-end, detail can be access by event.params.xx
 */
- (NSString*)paramsName {
  // TODO(hexionghui): Change it to 'detail'.
  return @"params";
}

- (void)addDetailKey:(NSString*)key value:(NSObject*)value {
  [_params setObject:value forKey:key];
}

- (NSString*)description {
  NSMutableString* description = [NSMutableString stringWithString:[super description]];
  [description appendFormat:@"; eventName:%@; ", self.eventName];
  [description appendFormat:@"%@;", self.params.description];
  return description.copy;
}

@end

@implementation LynxDetailEvent

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
                      detail:(NSMutableDictionary*)detail {
  return [super initWithName:name targetSign:target currentTargetSign:target params:detail];
}

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
           currentTargetSign:(NSInteger)currentTarget
                      detail:(nullable NSMutableDictionary*)detail {
  return [super initWithName:name targetSign:target currentTargetSign:currentTarget params:detail];
}

/**
 * On front-end, detail can be access by event.detail.xx
 */
- (NSString*)paramsName {
  return @"detail";
}

@end
