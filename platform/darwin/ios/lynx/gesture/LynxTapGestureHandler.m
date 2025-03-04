// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureHandlerTrigger.h>
#import <Lynx/LynxTapGestureHandler.h>
#import <Lynx/LynxTouchEvent.h>

@interface LynxTapGestureHandler ()

// record x and y axis when action down
@property(nonatomic, assign) CGPoint startPoint;
// record x and y axis when action move / up
@property(nonatomic, assign) CGPoint lastPoint;
// is onEnd invoked or not
@property(nonatomic, assign) BOOL isInvokedEnd;
@property(nonatomic, strong) LynxTouchEvent *lastTouchEvent;
// Max distance in X and Y axis,  If the finger travels further than the defined distance
// along the X axis and the gesture will activated.
// otherwise assigned to 0
@property(nonatomic, assign) float maxDistance;
// Maximum time, expressed in milliseconds, that defines how fast a finger must be released after
// a touch. The default value is 500.
@property(nonatomic, assign) long maxDuration;

@property(nonatomic, strong) dispatch_block_t delayFailRunnable;

@end

@implementation LynxTapGestureHandler

- (instancetype)initWithSign:(NSInteger)sign
                     context:(LynxUIContext *)lynxContext
                      member:(id<LynxGestureArenaMember>)member
                    detector:(LynxGestureDetectorDarwin *)detector {
  if (self = [super initWithSign:sign context:lynxContext member:member detector:detector]) {
    _maxDuration = 500L;
    _maxDistance = 10;
    [self handleConfigMap:detector.configMap];
  }
  return self;
}

- (void)handleConfigMap:(NSMutableDictionary *)config {
  if (!config) {
    return;
  }
  _maxDuration = [[config objectForKey:@"maxDuration"] floatValue];
  _maxDistance = [[config objectForKey:@"maxDistance"] floatValue];
}

- (void)onHandle:(NSString *const)touchType
         touches:(NSSet<UITouch *> *)touches
           event:(UIEvent *_Nullable)event
      touchEvent:(LynxTouchEvent *_Nullable)touchEvent
      flingPoint:(CGPoint)flingPoint {
  _lastTouchEvent = touchEvent;
  CGPoint touchPoint = [[touches anyObject] locationInView:nil];

  if (event == nil) {
    [self ignore];
    return;
  }
  if ([self status] >= LYNX_STATE_FAIL) {
    [self endTap];
    return;
  }

  if (touchType == LynxEventTouchStart) {
    _startPoint = touchPoint;
    _isInvokedEnd = false;
    [self begin];
    [self onBegin:_startPoint touchEvent:touchEvent];
    [self startTap];
  } else if (touchType == LynxEventTouchMove) {
    _lastPoint = touchPoint;
    if ([self shouldFail]) {
      [self fail];
    }
  } else if (touchType == LynxEventTouchEnd || touchType == LynxEventTouchCancel) {
    _lastPoint = touchPoint;
    if ([self status] >= LYNX_STATE_FAIL) {
      [self fail];
    } else {
      [self activate];
      [self onStart:_lastPoint touchEvent:touchEvent];
      [self onEnd:_lastPoint touchEvent:touchEvent];
    }
    [self endTap];
  }
}

- (void)startTap {
  if (_delayFailRunnable) {
    dispatch_block_cancel(_delayFailRunnable);
  }
  __weak typeof(self) weakSelf = self;
  _delayFailRunnable = dispatch_block_create(0, ^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    if (strongSelf) {
      [strongSelf fail];
    }
  });

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(_maxDuration * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), _delayFailRunnable);
}

- (void)endTap {
  if (_delayFailRunnable) {
    dispatch_block_cancel(_delayFailRunnable);
    _delayFailRunnable = nil;
  }
}

- (BOOL)shouldFail {
  CGFloat dx = fabs(_lastPoint.x - _startPoint.x);
  CGFloat dy = fabs(_lastPoint.y - _startPoint.y);
  if (dx > _maxDistance || dy > _maxDistance) {
    return YES;
  }
  return NO;
}

- (void)fail {
  [super fail];
  [self onEnd:_lastPoint touchEvent:_lastTouchEvent];
}

- (void)end {
  [super end];
  [self onEnd:_lastPoint touchEvent:_lastTouchEvent];
}

- (void)reset {
  [super reset];
  self.isInvokedEnd = NO;
}

- (BOOL)isGestureTypeMatched:(NSUInteger)typeMask {
  return (typeMask & LynxGestureHandlerOptionTap);
}

- (void)onBegin:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onBeginEnabled]) {
    return;
  }
  [self sendGestureEvent:ON_BEGIN params:[self eventParamsFromTouchEvent:touchEvent]];
}

- (void)onStart:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onStartEnabled]) {
    return;
  }
  [self sendGestureEvent:ON_START params:[self eventParamsFromTouchEvent:touchEvent]];
}

- (void)onUpdate:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  // empty implementation, because long press gesture is not continuous gesture
}

- (void)onEnd:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onEndEnabled]) {
    return;
  }
  _isInvokedEnd = YES;
  [self sendGestureEvent:ON_END params:[self eventParamsFromTouchEvent:touchEvent]];
}

@end
