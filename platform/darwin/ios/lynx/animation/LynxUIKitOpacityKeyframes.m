// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIKitOpacityKeyframes.h"

typedef struct {
  double first;
  double second;
} LynxCubicAxis;

static double CubicValue(LynxCubicAxis axis, double parameter) {
  const double inverse = 1 - parameter;
  return parameter * (3 * inverse * inverse * axis.first + 3 * inverse * parameter * axis.second +
                      parameter * parameter);
}

static double CubicDerivative(LynxCubicAxis axis, double parameter) {
  const double inverse = 1 - parameter;
  return 3 *
         (inverse * inverse * axis.first + 2 * inverse * parameter * (axis.second - axis.first) +
          parameter * parameter * (1 - axis.second));
}

static double CubicParameter(LynxCubicAxis axis, double value) {
  if (value <= 0 || value >= 1) {
    return value <= 0 ? 0 : 1;
  }
  double low = 0;
  double high = 1;
  for (int step = 0; step < 32; ++step) {
    double middle = (low + high) / 2;
    if (CubicValue(axis, middle) < value) {
      low = middle;
    } else {
      high = middle;
    }
  }
  return (low + high) / 2;
}

static LynxCubicAxis ReversedCubicAxis(LynxCubicAxis axis) {
  return (LynxCubicAxis){1 - axis.second, 1 - axis.first};
}

// Restrict a cubic to the active interval and normalize it for UIKit.
static LynxCubicAxis NormalizeCubicInterval(LynxCubicAxis axis, double start, double end) {
  const double span = CubicValue(axis, end) - CubicValue(axis, start);
  if (span <= DBL_EPSILON) {
    return (LynxCubicAxis){1.0 / 3, 2.0 / 3};
  }
  const double scale = (end - start) / (3 * span);
  return (LynxCubicAxis){CubicDerivative(axis, start) * scale,
                         1 - CubicDerivative(axis, end) * scale};
}

@implementation LynxUIKitOpacityKeyframes {
  CAKeyframeAnimation* _animation;
  __weak UIView* _view;
  NSTimeInterval _activeDuration;
  CGFloat _underlyingOpacity;
  CGFloat _segmentStartOpacity;
  LynxCubicAxis _curveX;
  LynxCubicAxis _curveY;
  NSTimer* _boundaryTimer;
  UIViewPropertyAnimator* _segmentAnimator;
  void (^_completion)(BOOL);
  BOOL _didStart;
}

+ (BOOL)canAnimate:(CAKeyframeAnimation*)animation {
  if (animation.duration <= 0 || !isfinite(animation.duration) || !isfinite(animation.beginTime) ||
      animation.values.count < 2 || animation.values.count != animation.keyTimes.count ||
      ![animation.calculationMode isEqualToString:kCAAnimationLinear] ||
      animation.timingFunctions.count != 0) {
    return NO;
  }
  // This renderer slices monotone cubic timing curves. Curves that can move
  // across a key time more than once keep their CA interpolation.
  if (animation.timingFunction) {
    for (int i = 1; i <= 2; ++i) {
      float control[2];
      [animation.timingFunction getControlPointAtIndex:i values:control];
      if (!(control[0] >= 0 && control[0] <= 1 && control[1] >= 0 && control[1] <= 1)) {
        return NO;
      }
    }
  }
  double previous = -1;
  for (NSNumber* time in animation.keyTimes) {
    if (![time isKindOfClass:[NSNumber class]] || !(time.doubleValue > previous)) {
      return NO;
    }
    previous = time.doubleValue;
  }
  for (NSNumber* value in animation.values) {
    if (![value isKindOfClass:[NSNumber class]] ||
        !(value.doubleValue >= 0 && value.doubleValue <= 1)) {
      return NO;
    }
  }
  return animation.keyTimes.firstObject.doubleValue == 0 &&
         animation.keyTimes.lastObject.doubleValue == 1;
}

- (instancetype)initWithAnimation:(CAKeyframeAnimation*)animation
                             view:(UIView*)view
                   activeDuration:(NSTimeInterval)activeDuration
                underlyingOpacity:(CGFloat)underlyingOpacity
                       completion:(void (^)(BOOL))completion {
  self = [super init];
  if (self) {
    _animation = animation;
    _view = view;
    _activeDuration = activeDuration;
    _underlyingOpacity = underlyingOpacity;
    _segmentStartOpacity = view.alpha;
    _completion = [completion copy];
    CAMediaTimingFunction* timing =
        animation.timingFunction
            ?: [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionLinear];
    float first[2], second[2];
    [timing getControlPointAtIndex:1 values:first];
    [timing getControlPointAtIndex:2 values:second];
    _curveX = (LynxCubicAxis){first[0], second[0]};
    _curveY = (LynxCubicAxis){first[1], second[1]};
  }
  return self;
}

- (void)dealloc {
  [_boundaryTimer invalidate];
}

- (NSTimeInterval)elapsedTime {
  return [_view.layer convertTime:CACurrentMediaTime() fromLayer:nil] - _animation.beginTime;
}

- (BOOL)fillsBackwards {
  return [_animation.fillMode isEqualToString:kCAFillModeBackwards] ||
         [_animation.fillMode isEqualToString:kCAFillModeBoth];
}

- (void)setOpacityWithoutAnimation:(CGFloat)opacity {
  _segmentStartOpacity = opacity;
  [UIView performWithoutAnimation:^{
    self->_view.alpha = opacity;
  }];
}

- (void)setUnderlyingOpacity:(CGFloat)opacity {
  _underlyingOpacity = opacity;
  if ([self elapsedTime] < 0 && ![self fillsBackwards]) {
    [self setOpacityWithoutAnimation:opacity];
  }
}

- (NSNumber*)currentOpacity {
  CALayer* presentation = _view.layer.presentationLayer;
  return @(_segmentAnimator && presentation ? presentation.opacity : _segmentStartOpacity);
}

- (void)stopSegment {
  UIViewPropertyAnimator* animator = _segmentAnimator;
  _segmentAnimator = nil;
  if (animator.state == UIViewAnimatingStateActive) {
    [animator stopAnimation:YES];
  }
}

- (void)stop {
  [_boundaryTimer invalidate];
  _boundaryTimer = nil;
  [self stopSegment];
  _completion = nil;
}

- (void)complete:(BOOL)finished {
  void (^completion)(BOOL) = _completion;
  [self stop];
  if (completion) {
    completion(finished);
  }
}

- (void)scheduleBoundaryAfter:(NSTimeInterval)delay {
  __weak LynxUIKitOpacityKeyframes* weakSelf = self;
  _boundaryTimer =
      [NSTimer timerWithTimeInterval:MAX(delay, 0)
                             repeats:NO
                               block:^(NSTimer* timer) {
                                 LynxUIKitOpacityKeyframes* strongSelf = weakSelf;
                                 if (strongSelf && strongSelf->_boundaryTimer == timer) {
                                   [strongSelf advance];
                                 }
                               }];
  [[NSRunLoop mainRunLoop] addTimer:_boundaryTimer forMode:NSRunLoopCommonModes];
}

- (void)start {
  // Run after the keyframe owner sets its playback state, but ahead of an
  // initial pause posted by that owner to the same main queue.
  __weak LynxUIKitOpacityKeyframes* weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    LynxUIKitOpacityKeyframes* strongSelf = weakSelf;
    if (strongSelf && strongSelf->_completion) {
      [strongSelf advance];
    }
  });
}

- (void)advance {
  _boundaryTimer = nil;
  [self stopSegment];
  UIView* view = _view;
  if ((_isCurrent && !_isCurrent()) || !view.window ||
      [UIApplication sharedApplication].applicationState == UIApplicationStateBackground) {
    [self complete:NO];
    return;
  }
  const NSTimeInterval elapsed = [self elapsedTime];
  if (elapsed >= 0 && !_didStart) {
    _didStart = YES;
    [_animation.delegate animationDidStart:_animation];
    if (!_completion) {
      return;
    }
  }
  if (elapsed >= _activeDuration) {
    [self complete:YES];
    return;
  }
  NSArray<NSNumber*>* values = _animation.values;
  if (elapsed < 0) {
    [self setOpacityWithoutAnimation:[self fillsBackwards] ? values.firstObject.doubleValue
                                                           : _underlyingOpacity];
    [self scheduleBoundaryAfter:-elapsed];
    return;
  }

  NSArray<NSNumber*>* times = _animation.keyTimes;
  const NSTimeInterval duration = _animation.duration;
  const double iteration = floor(elapsed / duration);
  const BOOL reverse = _animation.autoreverses && fmod(iteration, 2) == 1;
  const double timeProgress = fmod(elapsed, duration) / duration;
  const LynxCubicAxis curveX = reverse ? ReversedCubicAxis(_curveX) : _curveX;
  const LynxCubicAxis curveY = reverse ? ReversedCubicAxis(_curveY) : _curveY;
  const double parameter = CubicParameter(curveX, timeProgress);
  const double progress = CubicValue(curveY, parameter);
  NSUInteger index = 0;
  double start = 0;
  double end;
  do {
    ++index;
    end = reverse ? 1 - times[times.count - 1 - index].doubleValue : times[index].doubleValue;
    if (progress < end || index == times.count - 1) {
      break;
    }
    start = end;
  } while (index < times.count - 1);

  const NSUInteger fromIndex = reverse ? values.count - index : index - 1;
  const NSUInteger toIndex = reverse ? fromIndex - 1 : fromIndex + 1;
  const CGFloat from = values[fromIndex].doubleValue;
  const CGFloat delta = values[toIndex].doubleValue - from;
  // CA applies timingFunction to the entire iteration before looking up keyTimes.
  // Find the next key time on that curve, then clip the cubic to the part still
  // playing. This also covers negative delay, resume and fractional iterations.
  double boundary = CubicParameter(curveY, end);
  // Halving a cubic with reversed control points gives each half ordered
  // controls. Any slice of a half then has UIKit-compatible control points.
  if (parameter < 0.5 && (curveX.first > curveX.second || curveY.first > curveY.second)) {
    boundary = MIN(boundary, 0.5);
  }
  const double timeAtBoundary = CubicValue(curveX, boundary);
  const NSTimeInterval remaining =
      MAX(0, MIN((timeAtBoundary - timeProgress) * duration, _activeDuration - elapsed));
  const double targetParameter =
      MIN(boundary, CubicParameter(curveX, timeProgress + remaining / duration));
  const double fraction = MAX(0, MIN(1, (progress - start) / (end - start)));
  const double targetFraction =
      MAX(0, MIN(1, (CubicValue(curveY, targetParameter) - start) / (end - start)));
  const CGFloat startOpacity = from + delta * fraction;
  const CGFloat targetOpacity = from + delta * targetFraction;
  [self setOpacityWithoutAnimation:startOpacity];

  if (startOpacity != targetOpacity && remaining > 0) {
    LynxCubicAxis segmentX = NormalizeCubicInterval(curveX, parameter, targetParameter);
    LynxCubicAxis segmentY = NormalizeCubicInterval(curveY, parameter, targetParameter);
    UICubicTimingParameters* timing = [[UICubicTimingParameters alloc]
        initWithControlPoint1:CGPointMake(MAX(0, MIN(1, segmentX.first)),
                                          MAX(0, MIN(1, segmentY.first)))
                controlPoint2:CGPointMake(MAX(0, MIN(1, segmentX.second)),
                                          MAX(0, MIN(1, segmentY.second)))];
    UIViewPropertyAnimator* animator = [[UIViewPropertyAnimator alloc] initWithDuration:remaining
                                                                       timingParameters:timing];
    [animator addAnimations:^{
      view.alpha = targetOpacity;
    }];
    animator.userInteractionEnabled = YES;
    _segmentAnimator = animator;
    __weak LynxUIKitOpacityKeyframes* weakSelf = self;
    __weak UIViewPropertyAnimator* weakAnimator = animator;
    [animator addCompletion:^(UIViewAnimatingPosition position) {
      LynxUIKitOpacityKeyframes* strongSelf = weakSelf;
      if (strongSelf && strongSelf->_segmentAnimator == weakAnimator &&
          position != UIViewAnimatingPositionEnd) {
        [strongSelf complete:NO];
      }
    }];
    [animator startAnimation];
  }
  // Advance from the original clock so segment and loop boundaries don't drift.
  if (_completion) {
    [self scheduleBoundaryAfter:remaining];
  }
}

@end
