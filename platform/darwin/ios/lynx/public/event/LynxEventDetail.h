// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import "LynxEvent.h"
#import "LynxEventTargetBase.h"
@class LynxView;

NS_ASSUME_NONNULL_BEGIN

enum EVENT_TYPE {
  TOUCH_EVENT,
  CUSTOM_EVENT,
};

#pragma mark - LynxEventDetail
@interface LynxEventDetail : NSObject

@property(nonatomic, nonnull) LynxEvent* event;
@property(nonatomic, weak, nullable) id<LynxEventTargetBase> eventTarget;
@property(nonatomic, weak, nullable) LynxView* lynxView;
@property(nonatomic, nullable) UIEvent* uiEvent;
@property(nonatomic, nullable) NSSet<UITouch*>* touches;

- (nullable instancetype)initWithEvent:(nonnull LynxEvent*)event
                                target:(nullable id<LynxEventTargetBase>)target
                              lynxView:(nullable LynxView*)lynxView;
- (nullable NSString*)eventName;
- (enum EVENT_TYPE)eventType;
- (CGPoint)targetPoint;
- (BOOL)isMultiTouch;
- (nullable NSDictionary*)targetPointMap;
- (nullable NSDictionary*)params;

@end

NS_ASSUME_NONNULL_END
