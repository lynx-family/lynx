// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFlingGestureHandler.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureHandlerTrigger.h>
#import <Lynx/LynxTouchEvent.h>

@interface LynxFlingGestureHandler ()
// is onBegin invoked or not
@property(nonatomic, assign) BOOL isInvokedBegin;
// is onStart invoked or not
@property(nonatomic, assign) BOOL isInvokedStart;
// is onEnd invoked or not
@property(nonatomic, assign) BOOL isInvokedEnd;
@end

@implementation LynxFlingGestureHandler

- (instancetype)initWithSign:(NSInteger)sign
                     context:(LynxUIContext *)lynxContext
                      member:(id<LynxGestureArenaMember>)member
                    detector:(LynxGestureDetectorDarwin *)detector {
  if (self = [super initWithSign:sign context:lynxContext member:member detector:detector]) {
    [self handleConfigMap:detector.configMap];
  }
  return self;
}

- (void)handleConfigMap:(NSMutableDictionary *)config {
  if (!config) {
    return;
  }
  // TODO(luochangan.adrian): need to handle config map in future
}

- (void)onHandle:(NSString *const)touchType
         touches:(NSSet<UITouch *> *)touches
           event:(UIEvent *_Nullable)event
      touchEvent:(LynxTouchEvent *_Nullable)touchEvent
      flingPoint:(CGPoint)flingPoint {
  if (event != nil && (touchType == LynxEventTouchStart || touchType == LynxEventTouchMove)) {
    // If the event is not empty, it means the finger on the screen, no need to handle fling gesture
    [self ignore];
    return;
  }
  if (event != nil && touchType == LynxEventTouchEnd) {
    [self begin];
    [self onBegin:CGPointZero touchEvent:nil];
    return;
  }
  if ([self status] >= LYNX_STATE_FAIL && [self status] <= LYNX_STATE_END) {
    [self onEnd:CGPointZero touchEvent:nil];
    return;
  }
  if (flingPoint.x == FLT_EPSILON && flingPoint.y == FLT_EPSILON) {
    [self fail];
    [self onEnd:CGPointZero touchEvent:nil];
    return;
  }

  if ([self status] == LYNX_STATE_INIT || [self status] == LYNX_STATE_UNDETERMINED) {
    [self begin];
    [self activate];
    [self onBegin:CGPointZero touchEvent:nil];
    [self onStart:CGPointZero touchEvent:nil];
    return;
  }
  [self onUpdate:flingPoint touchEvent:nil];
}

- (void)fail {
  [super fail];
  [self onEnd:CGPointZero touchEvent:nil];
}

- (void)end {
  [super end];
  [self onEnd:CGPointZero touchEvent:nil];
}

- (void)reset {
  [super reset];
  self.isInvokedBegin = NO;
  self.isInvokedStart = NO;
  self.isInvokedEnd = NO;
}

- (BOOL)isGestureTypeMatched:(NSUInteger)typeMask {
  return (typeMask & LynxGestureHandlerOptionFling);
}

- (void)onBegin:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onBeginEnabled] || _isInvokedBegin) {
    return;
  }

  _isInvokedBegin = YES;
  [self sendGestureEvent:ON_BEGIN params:[self eventParamsInActive:point]];
}

- (void)onStart:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onStartEnabled] || _isInvokedStart || !_isInvokedBegin) {
    return;
  }
  _isInvokedStart = YES;
  [self sendGestureEvent:ON_START params:[self eventParamsInActive:point]];
}

- (void)onUpdate:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if ([self onUpdateEnabled]) {
    [self sendGestureEvent:ON_UPDATE params:[self eventParamsInActive:point]];
  }
}

- (void)onEnd:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onEndEnabled] || _isInvokedEnd || !_isInvokedBegin) {
    return;
  }
  _isInvokedEnd = YES;
  [self sendGestureEvent:ON_END params:[self eventParamsInActive:point]];
}

- (NSDictionary *)eventParamsInActive:(CGPoint)delta {
  return @{
    @"scrollX" : @([self.gestureMember getMemberScrollX]),
    @"scrollY" : @([self.gestureMember getMemberScrollY]),
    @"deltaX" : @(delta.x),
    @"deltaY" : @(delta.y),
    @"isAtStart" : @([self.gestureMember getGestureBorder:YES]),
    @"isAtEnd" : @([self.gestureMember getGestureBorder:NO]),
  };
}

@end
