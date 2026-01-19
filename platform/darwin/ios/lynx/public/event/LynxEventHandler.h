// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxEventTarget.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxTouchHandler;
@class LynxUIOwner;
@class LynxUI;
@class LynxGestureArenaManager;
@class LynxCustomGestureRecognizer;
@class LynxWeakProxy;

@interface LynxEventHandler : NSObject

@property(nonatomic, weak, readonly) UIView *rootView;
@property(nonatomic, weak, readonly) LynxEventEmitter *eventEmitter;
@property(nonatomic, copy, readonly) LynxTouchHandler *touchRecognizer;
@property(nonatomic, copy, readonly) UIGestureRecognizer *tapRecognizer;
@property(nonatomic, copy, readonly) UIGestureRecognizer *longPressRecognizer;
@property(nonatomic, strong) LynxCustomGestureRecognizer *customPlatformGesture;
@property(nonatomic, weak, readonly) LynxGestureArenaManager *_Nullable gestureArenaManager;
@property(nonatomic, assign) BOOL enableSimultaneousTap;
@property(nonatomic, assign)
    BOOL customizedEndEditingStrategy;  // TODO: Migrate to foucsd-ui logic like we do on Android
@property(nonatomic, weak) id<LynxEventTarget>
    focusedInputTarget;  // TODO: Migrate to foucsd-ui logic like we do on Android

- (instancetype)initWithRootView:(UIView *)rootView;
- (instancetype)initWithRootView:(UIView *)rootView withRootUI:(nullable LynxUI *)rootUI;

- (void)attachContainerView:(UIView *)rootView;

- (void)updateUiOwner:(nullable LynxUIOwner *)owner eventEmitter:(LynxEventEmitter *)eventEmitter;

- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(nullable UIEvent *)event;
- (void)onPlatformGestureStatusChanged:(LynxGestureHandlerState)status;
- (void)handleFocus:(id<LynxEventTarget>)target
             onView:(UIView *)view
      withContainer:(UIView *)container
           andPoint:(CGPoint)point
           andEvent:(UIEvent *)event;

// When a PanGesture in the corresponding direction is detected, initiate the gesture interception
// process.
- (void)startInterceptPanGestures:(enum LynxPanInterceptDirection)direction;
// Get the first LynxEventTarget that sets the valid pan-intercept-direction.
- (id<LynxEventTarget>)getFirstPanInterceptDirectionTarget:
    (enum LynxPanInterceptDirection)direction;
// Get the valid and correct pan-intercept-direction of the given LynxEventTarget.
- (enum LynxPanInterceptScope)getPanInterceptScope:(id<LynxEventTarget>)target;
// Iterate through other Gestures and determine whether interception is necessary.
- (void)interceptPanGestures:(NSArray<LynxWeakProxy *> *)otherGestures
         withPlatformGesture:(UIGestureRecognizer *)platformGesture;
// Based on the relationship between pan-intercept-scope and gesture view, determine whether to
// intercept other gestures.
- (BOOL)shouldInterceptPanGesture:(UIView *)other
                         withView:(UIView *)view
                andInterceptScope:(enum LynxPanInterceptScope)scope;
// Determine the type and direction of other gestures.
- (BOOL)isPanGesture:(UIGestureRecognizer *)gesture
       withDirection:(enum LynxPanInterceptDirection)direction;

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
