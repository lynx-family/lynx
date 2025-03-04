// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_EVENT_LYNXEVENT_H_
#define DARWIN_COMMON_LYNX_EVENT_LYNXEVENT_H_

#import <Foundation/Foundation.h>
#import "LynxEventTargetBase.h"

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxEventType) {
  kNoneEvent,
  kTouchEvent,
  kMouseEVent,
  kWheelEvent,
  kKeyboardEvent,
  kCustomEvent,
};

#pragma mark - LynxTouchPseudoState
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateNone;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateHover;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateHoverTransition;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateActive;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateActiveTransition;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateFocus;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateFocusTransition;
FOUNDATION_EXPORT int32_t const LynxTouchPseudoStateAll;

#pragma mark - LynxEvent
/**
 * The basic event with event name only. Waring: do not use LynxEvent directly.
 */
@interface LynxEvent : NSObject

@property(nonatomic, readonly) NSInteger targetSign;
@property(nonatomic, readonly) NSInteger currentTargetSign;
@property(nonatomic, weak) id<LynxEventTargetBase> eventTarget;
@property(nonatomic, copy, readonly) NSString* eventName;
@property(nonatomic, readonly) LynxEventType eventType;
@property(nonatomic) NSTimeInterval timestamp;

- (instancetype)initWithName:(NSString*)name type:(LynxEventType)type;

- (instancetype)initWithName:(NSString*)name type:(LynxEventType)type targetSign:(NSInteger)target;

- (instancetype)initWithName:(NSString*)name
                        type:(LynxEventType)type
                  targetSign:(NSInteger)target
           currentTargetSign:(NSInteger)currentTarget;

- (BOOL)canCoalesce;
- (NSMutableDictionary*)generateEventBody;

@end

/**
 * Custom event will contain detail object on the event body which can be used
 * as extra data on front-end.
 */
@interface LynxCustomEvent : LynxEvent

@property(nonatomic, nullable) NSMutableDictionary* params;

- (instancetype)initWithName:(NSString*)name targetSign:(NSInteger)target;

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
                      params:(nullable NSDictionary*)params;

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
           currentTargetSign:(NSInteger)currentTarget
                      params:(nullable NSDictionary*)params;

- (void)addDetailKey:(NSString*)key value:(NSObject*)value;

- (NSString*)paramsName;

@end

// TODO(hexionghui): Delete this later.
// __attribute__((deprecated("Use LynxCustomEvent instend.")))
@interface LynxDetailEvent : LynxCustomEvent

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
                      detail:(nullable NSDictionary*)detail;

- (instancetype)initWithName:(NSString*)name
                  targetSign:(NSInteger)target
           currentTargetSign:(NSInteger)currentTarget
                      detail:(nullable NSDictionary*)detail;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_EVENT_LYNXEVENT_H_
