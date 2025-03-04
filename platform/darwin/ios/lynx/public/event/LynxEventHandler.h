// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxEventEmitter.h"
#import "LynxEventTarget.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxTouchHandler;
@class LynxUIOwner;
@class LynxUI;
@class LynxGestureArenaManager;

@interface LynxEventHandler : NSObject

@property(nonatomic, weak, readonly) UIView *rootView;
@property(nonatomic, weak, readonly) LynxEventEmitter *eventEmitter;
@property(nonatomic, copy, readonly) LynxTouchHandler *touchRecognizer;
@property(nonatomic, copy, readonly) UIGestureRecognizer *tapRecognizer;
@property(nonatomic, copy, readonly) UIGestureRecognizer *longPressRecognizer;
@property(nonatomic, weak, readonly) LynxGestureArenaManager *_Nullable gestureArenaManager;
@property(nonatomic) BOOL enableSimultaneousTap;

- (instancetype)initWithRootView:(UIView *)rootView;
- (instancetype)initWithRootView:(UIView *)rootView withRootUI:(nullable LynxUI *)rootUI;

- (void)attachLynxView:(UIView *)rootView;

- (void)updateUiOwner:(nullable LynxUIOwner *)owner eventEmitter:(LynxEventEmitter *)eventEmitter;

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(nullable UIEvent *)event;

- (void)onGestureRecognized;
- (void)onGestureRecognizedByEventTarget:(id<LynxEventTarget>)ui;
- (void)onPropsChangedByEventTarget:(id<LynxEventTarget>)ui;
- (void)resetEventEnv;
- (NSInteger)canRespondTapOrClickEvent:(id<LynxEventTarget>)ui;
- (NSInteger)canRespondTapOrClickWhenUISlideByProps:(id<LynxEventTarget>)ui;

- (void)dispatchTapEvent:(UITapGestureRecognizer *)sender;

- (void)dispatchLongPressEvent:(UILongPressGestureRecognizer *)sender;

- (NSInteger)setGestureArenaManagerAndGetIndex:(nullable LynxGestureArenaManager *)manager;

- (void)removeGestureArenaManager:(NSInteger)index;

- (id<LynxEventTarget>)touchTarget;

- (LynxUIOwner *)uiOwner;
@end

NS_ASSUME_NONNULL_END
