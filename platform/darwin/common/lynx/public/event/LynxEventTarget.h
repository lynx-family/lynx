// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import "LynxEventDetail.h"
#import "LynxEventSpec.h"

NS_ASSUME_NONNULL_BEGIN

enum LynxEventPropStatus {
  kLynxEventPropEnable,
  kLynxEventPropDisable,
  kLynxEventPropUndefined,
};

@protocol LynxEventTarget <LynxEventTargetBase>

- (NSInteger)signature;

- (int32_t)pseudoStatus;

- (nullable id<LynxEventTarget>)parentTarget;

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(nullable UIEvent*)event;

- (BOOL)containsPoint:(CGPoint)point;

- (nullable NSDictionary<NSString*, LynxEventSpec*>*)eventSet;

- (nullable NSDictionary<NSNumber*, LynxGestureDetectorDarwin*>*)gestureMap;

- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent*)event;

- (BOOL)ignoreFocus;

- (BOOL)consumeSlideEvent:(CGFloat)angle;

- (BOOL)blockNativeEvent:(UIGestureRecognizer*)gestureRecognizer;

- (BOOL)eventThrough;

- (BOOL)enableTouchPseudoPropagation;

- (void)onPseudoStatusFrom:(int32_t)preStatus changedTo:(int32_t)currentStatus;

// only include touches and event, don't care Lynx frontend event
- (BOOL)dispatchTouch:(NSString* const)touchType
              touches:(NSSet<UITouch*>*)touches
            withEvent:(UIEvent*)event;

// include target point and Lynx frontend event
- (BOOL)dispatchEvent:(LynxEventDetail*)event;

- (void)onResponseChain;

- (void)offResponseChain;

- (BOOL)isOnResponseChain;

- (NSInteger)getGestureArenaMemberId;

@end

NS_ASSUME_NONNULL_END
