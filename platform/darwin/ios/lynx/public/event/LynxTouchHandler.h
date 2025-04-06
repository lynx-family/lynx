// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxUIOwner.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxEventHandler;

@interface LynxTouchHandler : UIGestureRecognizer

@property(nonatomic, weak) _Nullable id<LynxEventTarget> target;
@property(nonatomic, weak) _Nullable id<LynxEventTarget> preTarget;

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler;

- (void)onGestureRecognized;

- (void)touchesBeganInner:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event;
- (void)touchesMovedInner:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event;
- (void)touchesEndedInner:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event;
- (void)touchesCancelledInner:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event;

@end

NS_ASSUME_NONNULL_END
