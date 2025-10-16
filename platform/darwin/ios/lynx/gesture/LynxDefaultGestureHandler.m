// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDefaultGestureHandler.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxGestureExtraBundle.h>
#import <Lynx/LynxGestureHandlerTrigger.h>
#import <Lynx/LynxTouchEvent.h>
#import <math.h>
#import "LynxGestureArenaManager.h"
#import "LynxGestureDetectorManager.h"
#import "LynxGestureFlingTrigger.h"

@interface LynxDefaultGestureHandler ()

@property(nonatomic, strong) NSMutableDictionary *eventParams;
@property(nonatomic, assign) CGPoint lastPoint;
// is onBegin invoked or not
@property(nonatomic, assign) BOOL isInvokedBegin;
// is onStart invoked or not
@property(nonatomic, assign) BOOL isInvokedStart;
// is onEnd invoked or not
@property(nonatomic, assign) BOOL isInvokedEnd;
@property(nonatomic, strong) LynxTouchEvent *lastTouchEvent;

@end

@implementation LynxDefaultGestureHandler

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

- (BOOL)isGestureTypeMatched:(NSUInteger)typeMask {
  return (typeMask & LynxGestureHandlerOptionDefault);
}

- (void)onHandle:(NSString *const)touchType
                 touches:(NSSet<UITouch *> *)touches
                   event:(UIEvent *_Nullable)event
              touchEvent:(LynxTouchEvent *_Nullable)touchEvent
              flingPoint:(CGPoint)flingPoint
    handleBySimultaneous:(BOOL)handleSimultaneous
             extraBundle:(LynxGestureExtraBundle *_Nullable)extraBundle {
  self.lastTouchEvent = touchEvent;
  CGPoint touchPoint = [[touches anyObject] locationInView:nil];

  if (handleSimultaneous && extraBundle != nil) {
    if (extraBundle.isNeedConsumedSimultaneousGesture) {
      if (self.gestureMember != nil) {
        [self.gestureMember onGestureScrollBy:CGPointMake(extraBundle.simultaneousDeltaX,
                                                          extraBundle.simultaneousDeltaY)];
      }
    }
    return;
  }

  if (self.status >= LynxGestureHandlerStateFail) {
    [self onEnd:_lastPoint touchEvent:_lastTouchEvent];
    return;
  }
  if (event != nil) {
    if (touchType == LynxEventTouchStart) {
      _lastPoint = touchPoint;
      _isInvokedEnd = NO;
      [self begin];
      [self onBegin:self.lastPoint touchEvent:touchEvent];
    } else if (touchType == LynxEventTouchMove) {
      CGPoint deltaPoint = CGPointMake(_lastPoint.x - touchPoint.x, _lastPoint.y - touchPoint.y);
      if (deltaPoint.x == 0 && deltaPoint.y == 0) {
        return;
      }

      if (extraBundle != nil && extraBundle.gestureDirection == DIRECTION_UNDETERMINED) {
        extraBundle.gestureDirection =
            fabs(deltaPoint.x) > fabs(deltaPoint.y) ? DIRECTION_HORIZONTAL : DIRECTION_VERTICAL;
      }

      if (extraBundle != nil && extraBundle.gestureDirection != DIRECTION_UNDETERMINED) {
        if (extraBundle.gestureDirection == DIRECTION_HORIZONTAL) {
          deltaPoint.y = 0;
        } else {
          deltaPoint.x = 0;
        }
      }

      if (self.status == LynxGestureHandlerStateInit) {
        [self onBegin:self.lastPoint touchEvent:touchEvent];
        if (self.status <= LynxGestureHandlerStateBegin) {
          [self activate];
        }
      } else {
        if ([self shouldFail:deltaPoint extraBundle:extraBundle]) {
          if (self.gestureMember != nil && (![self.gestureMember getGestureBorder:NO] ||
                                            ![self.gestureMember getGestureBorder:YES])) {
            // consume last delta to arrive start or end
            [self onUpdate:deltaPoint touchEvent:touchEvent extraBundle:extraBundle];
          }
          [self fail];
          [self onEnd:self.lastPoint touchEvent:touchEvent];
        } else {
          [self activate];
          [self onUpdate:deltaPoint touchEvent:touchEvent extraBundle:extraBundle];
        }
      }
      _lastPoint = touchPoint;
    } else if (touchType == LynxEventTouchEnd || touchType == LynxEventTouchCancel) {
      if (self.status == LynxGestureHandlerStateActive && flingPoint.x == FLT_EPSILON &&
          flingPoint.y == FLT_EPSILON) {
        [self fail];
        [self onEnd:CGPointZero touchEvent:nil];
      }
    }
  } else {
    if (self.status == LynxGestureHandlerStateActive && flingPoint.x == FLT_EPSILON &&
        flingPoint.y == FLT_EPSILON) {
      [self fail];
      [self onEnd:CGPointZero touchEvent:nil];
      return;
    }

    if (extraBundle != nil && extraBundle.gestureDirection != DIRECTION_UNDETERMINED) {
      if (extraBundle.gestureDirection == DIRECTION_HORIZONTAL) {
        flingPoint.y = 0;
      } else {
        flingPoint.x = 0;
      }
    }

    if ([self shouldFail:flingPoint extraBundle:extraBundle]) {
      if (self.gestureMember != nil && (![self.gestureMember getGestureBorder:NO] ||
                                        ![self.gestureMember getGestureBorder:YES])) {
        [self onUpdate:flingPoint touchEvent:nil extraBundle:extraBundle];
      }
      [self fail];
      [self onEnd:flingPoint touchEvent:nil];
    } else {
      if (self.status == LynxGestureHandlerStateInit) {
        [self onBegin:_lastPoint touchEvent:touchEvent];
        if (self.status <= LynxGestureHandlerStateBegin) {
          [self activate];
        }
        return;
      }
      [self onUpdate:flingPoint touchEvent:nil extraBundle:extraBundle];
    }
  }
}

- (BOOL)shouldFail:(CGPoint)point extraBundle:(LynxGestureExtraBundle *)extraBundle {
  if (extraBundle != nil) {
    if (extraBundle.gestureDirection == DIRECTION_HORIZONTAL &&
        self.gestureMember.getScrollContainerDirection != DIRECTION_HORIZONTAL) {
      return YES;
    }

    if (extraBundle.gestureDirection == DIRECTION_VERTICAL &&
        self.gestureMember.getScrollContainerDirection != DIRECTION_VERTICAL) {
      return YES;
    }
  }
  return ![self.gestureMember canConsumeGesture:point];
}

- (BOOL)canActiveWithCurrentGesture:(CGPoint)deltaPoint {
  return [super canActiveWithCurrentGesture:deltaPoint] &&
         [self.gestureMember canConsumeGesture:deltaPoint];
}

- (void)fail {
  [super fail];
  if (!self.lastTouchEvent) {
    [self onEnd:CGPointZero touchEvent:nil];
  } else {
    [self onEnd:_lastPoint touchEvent:_lastTouchEvent];
  }
}

- (void)end {
  [super end];
  if (!self.lastTouchEvent) {
    [self onEnd:CGPointZero touchEvent:nil];
  } else {
    [self onEnd:_lastPoint touchEvent:_lastTouchEvent];
  }
}

- (void)reset {
  [super reset];
  self.lastPoint = CGPointZero;
  self.isInvokedBegin = NO;
  self.isInvokedStart = NO;
  self.isInvokedEnd = NO;
}

- (void)onBegin:(CGPoint)point touchEvent:(LynxTouchEvent *)touchEvent {
  if (![self onBeginEnabled] || self.isInvokedBegin) {
    return;
  }

  self.isInvokedBegin = YES;
  [self sendGestureEvent:ON_BEGIN params:[self eventParamsInActive:CGPointZero]];
}

- (void)onStart:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
  if (![self onStartEnabled] || self.isInvokedStart || !self.isInvokedBegin) {
    return;
  }

  self.isInvokedStart = YES;
  [self sendGestureEvent:ON_BEGIN params:[self eventParamsInActive:CGPointZero]];
}

- (void)onUpdate:(CGPoint)point
      touchEvent:(LynxTouchEvent *)touchEvent
     extraBundle:(LynxGestureExtraBundle *_Nullable)extraBundle {
  [self.gestureMember onGestureScrollBy:point];

  if (extraBundle != nil) {
    extraBundle.isNeedConsumedSimultaneousGesture = YES;
    extraBundle.simultaneousDeltaX = point.x;
    extraBundle.simultaneousDeltaY = point.y;
  }

  if (![self onUpdateEnabled]) {
    return;
  }

  [self sendGestureEvent:ON_UPDATE params:[self eventParamsInActive:point]];
}

- (void)onEnd:(CGPoint)point touchEvent:(LynxTouchEvent *)touchEvent {
  if (![self onEndEnabled] || self.isInvokedEnd || !self.isInvokedBegin) {
    return;
  }

  self.isInvokedEnd = YES;
  [self sendGestureEvent:ON_END params:[self eventParamsInActive:CGPointZero]];
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
