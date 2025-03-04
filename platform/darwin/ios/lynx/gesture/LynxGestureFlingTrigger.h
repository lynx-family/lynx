// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxGestureFlingTriggerState) {
  LynxGestureFlingTriggerStateStart = 0,
  LynxGestureFlingTriggerStateUpdate,
  LynxGestureFlingTriggerStateEnd
};

/**
 * Simulate FLING effect with `CADisplayLink`
 */
@interface LynxGestureFlingTrigger : NSObject

@property(readonly, nonatomic, assign) LynxGestureFlingTriggerState state;
@property(readonly, nonatomic, assign) CGPoint velocity;
@property(readonly, nonatomic, assign) CGPoint distance;
@property(readonly, nonatomic, assign) CGPoint lastDistance;

- (instancetype)initWithTarget:(nullable id)target action:(nullable SEL)action;

/**
 * Start fling
 * @param velocity The initial velocity.
 * @return Wether to start FLING according to the initial velocity.
 */
- (BOOL)startWithVelocity:(CGPoint)velocity;

- (void)stop;

- (void)reset;

- (BOOL)isFinished;

@end

/**
 * Monitor the velocity by creating a dummy `UIPanGestureRecognizer`
 */
@interface LynxGestureVelocityTracker : NSObject

@property(readonly, nonatomic, strong) UIPanGestureRecognizer *tracker;

- (instancetype)initWithRootView:(UIView *)rootView;

- (CGPoint)velocityInView:(nullable UIView *)view;

@end

NS_ASSUME_NONNULL_END
