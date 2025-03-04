// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureHandlerTrigger.h>
#import <Lynx/LynxPanGestureHandler.h>
#import <Lynx/LynxTouchEvent.h>

@interface LynxPanGestureHandler ()

// record x and y axis when action down
@property(nonatomic, assign) CGPoint startPoint;
// record x and y axis when action move / up
@property(nonatomic, assign) CGPoint lastPoint;
// is onBegin invoked or not
@property(nonatomic, assign) BOOL isInvokedBegin;
// is onStart invoked or not
@property(nonatomic, assign) BOOL isInvokedStart;
// is onEnd invoked or not
@property(nonatomic, assign) BOOL isInvokedEnd;
@property(nonatomic, strong) LynxTouchEvent *lastTouchEvent;
// Min distance in X and Y axis,  If the finger travels further than the defined distance
// along the X axis and the gesture will activated.
// otherwise assigned to 0
@property(nonatomic) float minDistance;

@end

@implementation LynxPanGestureHandler

- (instancetype)initWithSign:(NSInteger)sign
                     context:(LynxUIContext *)lynxContext
                      member:(id<LynxGestureArenaMember>)member
                    detector:(LynxGestureDetectorDarwin *)detector {
  if (self = [super initWithSign:sign context:lynxContext member:member detector:detector]) {
    _minDistance = 0;
    [self handleConfigMap:detector.configMap];
  }
  return self;
}

- (void)handleConfigMap:(NSMutableDictionary *)config {
  if (!config) {
    return;
  }
  _minDistance = [[config objectForKey:@"minDistance"] floatValue];
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
    return;
  }

  if (touchType == LynxEventTouchStart) {
    _startPoint = touchPoint;
    _isInvokedEnd = NO;
    [self begin];
    [self onBegin:_startPoint touchEvent:touchEvent];
  } else if (touchType == LynxEventTouchMove) {
    _lastPoint = touchPoint;
    if ([self status] == LYNX_STATE_INIT) {
      [self begin];
      [self onBegin:_lastPoint touchEvent:touchEvent];
    }
    if ([self shouldActive]) {
      [self onStart:_lastPoint touchEvent:touchEvent];
      [self activate];
    }
    if ([self status] == LYNX_STATE_ACTIVE) {
      [self onUpdate:_lastPoint touchEvent:touchEvent];
    } else if ([self status] >= LYNX_STATE_FAIL) {
      [self onEnd:_lastPoint touchEvent:touchEvent];
    }
  } else if (touchType == LynxEventTouchEnd || touchType == LynxEventTouchCancel) {
    [self fail];
    [self onEnd:_lastPoint touchEvent:touchEvent];
  }
}

- (BOOL)shouldActive {
  if ([self status] >= LYNX_STATE_FAIL) {
    return false;
  }
  CGFloat dx = fabs(_lastPoint.x - _startPoint.x);
  CGFloat dy = fabs(_lastPoint.y - _startPoint.y);
  if (dx > _minDistance || dy > _minDistance) {
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
  self.isInvokedBegin = NO;
  self.isInvokedStart = NO;
  self.isInvokedEnd = NO;
}

- (BOOL)isGestureTypeMatched:(NSUInteger)typeMask {
  return (typeMask & LynxGestureHandlerOptionPan);
}

- (void)onBegin:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onBeginEnabled] || _isInvokedBegin) {
    return;
  }
  _isInvokedBegin = YES;
  [self sendGestureEvent:ON_BEGIN params:[self eventParamsInActive:touchEvent]];
}

- (void)onStart:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onStartEnabled] || _isInvokedStart || !_isInvokedBegin) {
    return;
  }
  _isInvokedStart = YES;
  [self sendGestureEvent:ON_START params:[self eventParamsInActive:touchEvent]];
}

- (void)onUpdate:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if ([self onUpdateEnabled]) {
    [self sendGestureEvent:ON_UPDATE params:[self eventParamsInActive:touchEvent]];
  }
}

- (void)onEnd:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onEndEnabled] || _isInvokedEnd || !_isInvokedBegin) {
    return;
  }
  _isInvokedEnd = YES;
  [self sendGestureEvent:ON_END params:[self eventParamsInActive:touchEvent]];
}

- (NSDictionary *)eventParamsInActive:(LynxTouchEvent *)touchEvent {
  NSMutableDictionary *params = [@{
    @"scrollX" : @([self.gestureMember getMemberScrollX]),
    @"scrollY" : @([self.gestureMember getMemberScrollY]),
    @"isAtStart" : @([self.gestureMember getGestureBorder:YES]),
    @"isAtEnd" : @([self.gestureMember getGestureBorder:NO]),
  } mutableCopy];

  [params addEntriesFromDictionary:[self eventParamsFromTouchEvent:touchEvent]];
  return params;
}

@end
