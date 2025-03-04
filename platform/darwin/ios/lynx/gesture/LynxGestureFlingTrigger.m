// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxGestureFlingTrigger.h"
#import "LynxWeakProxy.h"

static const CGFloat kLynxMinVelocity = 300.0;
static const CGFloat kLynxMaxVelocity = 5000.0;

@interface LynxGestureFlingTrigger ()
@property(readonly, nonatomic, weak) id target;
@property(readonly, nonatomic, assign) SEL action;
@property(readonly, nonatomic, strong) CADisplayLink *displayLink;
@property(readonly, nonatomic, assign) NSTimeInterval beginTime;
@property(readonly, nonatomic, assign) CGFloat decelerationRate;
@property(readonly, nonatomic, assign) CGFloat dCoefficient;
@property(readonly, nonatomic, assign) CGFloat threshold;
@property(readonly, nonatomic, assign) NSTimeInterval durationX;
@property(readonly, nonatomic, assign) NSTimeInterval durationY;
@property(readonly, nonatomic, assign) BOOL flingX;
@property(readonly, nonatomic, assign) BOOL flingY;
@end

@implementation LynxGestureFlingTrigger

- (instancetype)initWithTarget:(nullable id)target action:(nullable SEL)action {
  if (self = [super init]) {
    _target = target;
    _action = action;
    _displayLink = [CADisplayLink displayLinkWithTarget:[LynxWeakProxy proxyWithTarget:self]
                                               selector:@selector(tick:)];
    [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    _displayLink.paused = YES;
    if (@available(iOS 10.0, *)) {
      _displayLink.preferredFramesPerSecond = 120;
    }
    _threshold = 0.5f / [[UIScreen mainScreen] scale];
    _state = LynxGestureFlingTriggerStateEnd;
  }
  return self;
}

- (void)tick:(CADisplayLink *)displayLink {
  IMP imp = [_target methodForSelector:_action];
  void (*invoke)(id, SEL, LynxGestureFlingTrigger *) = (void *)imp;
  if (_state == LynxGestureFlingTriggerStateStart) {
    // Start
    if (invoke) {
      invoke(_target, _action, self);
    }

    _state = LynxGestureFlingTriggerStateUpdate;
  }

  NSTimeInterval currentTime = CACurrentMediaTime();
  NSTimeInterval timeProgressX = getTimeProgress(currentTime, _beginTime, _durationX);
  NSTimeInterval timeProgressY = getTimeProgress(currentTime, _beginTime, _durationY);

  BOOL willEnd = (!_flingX || timeProgressX >= 1.0) && (!_flingY || timeProgressY >= 1.0);

  // calculate current distance. Use C function to speed up.
  _distance.x = _flingX ? updateDistance(timeProgressX, _durationX, _velocity.x, _dCoefficient,
                                         _decelerationRate)
                        : _lastDistance.x;
  _distance.y = _flingY ? updateDistance(timeProgressY, _durationY, _velocity.y, _dCoefficient,
                                         _decelerationRate)
                        : _lastDistance.y;

  // Update
  if (invoke) {
    invoke(_target, _action, self);
  }

  _lastDistance = _distance;

  if (willEnd) {
    _state = LynxGestureFlingTriggerStateEnd;
    // END
    if (invoke) {
      invoke(_target, _action, self);
    }

    [self stop];
  }
}

NSTimeInterval getTimeProgress(NSTimeInterval currentTime, NSTimeInterval beginTime,
                               CGFloat duration) {
  return MIN(1.0, (currentTime - beginTime) / duration);
}

CGFloat updateDistance(NSTimeInterval timeProgress, CGFloat duration, CGFloat velocity,
                       CGFloat dCoefficient, CGFloat decelerationRate) {
  return (pow(decelerationRate, 1000. * timeProgress * duration) - 1.) / dCoefficient * velocity;
}

- (BOOL)startWithVelocity:(CGPoint)velocity {
  // Limit the maximum initial velocity
  velocity.x = MAX(MIN(kLynxMaxVelocity, velocity.x), -kLynxMaxVelocity);
  velocity.y = MAX(MIN(kLynxMaxVelocity, velocity.y), -kLynxMaxVelocity);

  // Will not begin FLING, if the initial velocity is not enough
  BOOL flingX = ABS(velocity.x) >= kLynxMinVelocity;
  BOOL flingY = ABS(velocity.y) >= kLynxMinVelocity;

  if (!flingX && !flingY) {
    return NO;
  }

  // Stop previous FLING
  [self stop];

  _flingX = flingX;
  _flingY = flingY;

  _beginTime = CACurrentMediaTime();
  _velocity = velocity;

  // Do some math to get the duration of FLING
  _decelerationRate = 0.998f;

  _dCoefficient = 1000.f * log(_decelerationRate);

  _durationX = log(-_dCoefficient * _threshold / fabs(velocity.x)) / _dCoefficient;
  _durationY = log(-_dCoefficient * _threshold / fabs(velocity.y)) / _dCoefficient;

  _lastDistance = CGPointZero;
  _distance = CGPointZero;
  _state = LynxGestureFlingTriggerStateStart;

  // Start ticking
  _displayLink.paused = NO;
  return YES;
}

- (void)stop {
  if (!_displayLink.paused) {
    _displayLink.paused = YES;
  }
}

- (void)reset {
  _beginTime = CACurrentMediaTime();
  _lastDistance = CGPointZero;
  _distance = CGPointZero;
  _state = LynxGestureFlingTriggerStateEnd;
  _displayLink.paused = YES;
}

- (BOOL)isFinished {
  return _displayLink.paused;
}

- (void)dealloc {
  [_displayLink invalidate];
}

@end

@interface LynxGestureVelocityTracker () <UIGestureRecognizerDelegate>

@end

@implementation LynxGestureVelocityTracker

- (instancetype)initWithRootView:(UIView *)rootView {
  if (self = [super init]) {
    _tracker = [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(update:)];
    _tracker.delegate = self;
    [rootView addGestureRecognizer:_tracker];
  }
  return self;
}

- (CGPoint)velocityInView:(nullable UIView *)view {
  if (!view) {
    view = _tracker.view;
  }
  return [_tracker velocityInView:view];
}

/**
 * Our dummy `UIPanGestureRecognizer` must be detected
 */
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer *)otherGestureRecognizer {
  return YES;
}

- (void)update:(id)sender {
  // Do nothing
}

- (void)dealloc {
  [_tracker.view removeGestureRecognizer:_tracker];
}

@end
