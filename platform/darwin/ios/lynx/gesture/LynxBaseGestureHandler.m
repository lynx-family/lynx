// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseGestureHandler.h>
#import <Lynx/LynxDefaultGestureHandler.h>
#import <Lynx/LynxFlingGestureHandler.h>
#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxGestureDetectorDarwin.h>
#import <Lynx/LynxGestureExtraBundle.h>
#import <Lynx/LynxGestureHandlerTrigger.h>
#import <Lynx/LynxLongPressGestureHandler.h>
#import <Lynx/LynxPanGestureHandler.h>
#import <Lynx/LynxTapGestureHandler.h>
#import <Lynx/LynxUIContext.h>

@interface LynxBaseGestureHandler ()

@property(nonatomic, strong) NSMutableDictionary<const NSString *, NSNumber *> *enableFlags;

@end

@implementation LynxBaseGestureHandler

- (instancetype)initWithSign:(NSInteger)sign
                     context:(LynxUIContext *)lynxContext
                      member:(id<LynxGestureArenaMember>)member
                    detector:(LynxGestureDetectorDarwin *)detector {
  if (self = [super init]) {
    _gestureDetector = detector;
    _gestureMember = member;
    _enableFlags = [NSMutableDictionary dictionary];
    _sign = sign;
    _context = lynxContext;
    [self handleEnableGestureCallback:detector.gestureCallbackNames];
  }
  return self;
}

/**
 Handle enable gesture callback by setting enable flags for specific callback names.
 @param callbackNames  The list of callback names.
 */
- (void)handleEnableGestureCallback:(NSArray<NSString *> *)callbackNames {
  // Set initial enable flags for all callback names to false
  _enableFlags[ON_TOUCHES_DOWN] = @(NO);
  _enableFlags[ON_TOUCHES_MOVE] = @(NO);
  _enableFlags[ON_TOUCHES_UP] = @(NO);
  _enableFlags[ON_TOUCHES_CANCEL] = @(NO);
  _enableFlags[ON_BEGIN] = @(NO);
  _enableFlags[ON_START] = @(NO);
  _enableFlags[ON_UPDATE] = @(NO);
  _enableFlags[ON_END] = @(NO);

  // Iterate over the callback names
  for (NSString *callback in callbackNames) {
    if (_enableFlags[callback]) {
      _enableFlags[callback] = @(YES);
    }
  }
}

+ (NSDictionary<NSNumber *, LynxBaseGestureHandler *> *)
    convertToGestureHandler:(NSInteger)sign
                    context:(LynxUIContext *)lynxContext
                     member:(id<LynxGestureArenaMember>)member
           gestureDetectors:
               (NSDictionary<NSNumber *, LynxGestureDetectorDarwin *> *)gestureDetectors {
  // Create an empty map to store the gesture handlers
  NSMutableDictionary<NSNumber *, LynxBaseGestureHandler *> *gestureHandlerMap =
      [NSMutableDictionary dictionary];

  [gestureDetectors enumerateKeysAndObjectsUsingBlock:^(
                        NSNumber *_Nonnull key, LynxGestureDetectorDarwin *_Nonnull detector,
                        BOOL *_Nonnull stop) {
    if (detector.gestureType == LynxGestureTypePan) {
      [gestureHandlerMap setObject:[[LynxPanGestureHandler alloc] initWithSign:sign
                                                                       context:lynxContext
                                                                        member:member
                                                                      detector:detector]
                            forKey:@(LynxGestureHandlerOptionPan)];
    } else if (detector.gestureType == LynxGestureTypeDefault) {
      [gestureHandlerMap setObject:[[LynxDefaultGestureHandler alloc] initWithSign:sign
                                                                           context:lynxContext
                                                                            member:member
                                                                          detector:detector]
                            forKey:@(LynxGestureHandlerOptionDefault)];
    } else if (detector.gestureType == LynxGestureTypeFling) {
      [gestureHandlerMap setObject:[[LynxFlingGestureHandler alloc] initWithSign:sign
                                                                         context:lynxContext
                                                                          member:member
                                                                        detector:detector]
                            forKey:@(LynxGestureHandlerOptionFling)];
    } else if (detector.gestureType == LynxGestureTypeTap) {
      [gestureHandlerMap setObject:[[LynxTapGestureHandler alloc] initWithSign:sign
                                                                       context:lynxContext
                                                                        member:member
                                                                      detector:detector]
                            forKey:@(LynxGestureHandlerOptionTap)];
    } else if (detector.gestureType == LynxGestureTypeLongPress) {
      [gestureHandlerMap setObject:[[LynxLongPressGestureHandler alloc] initWithSign:sign
                                                                             context:lynxContext
                                                                              member:member
                                                                            detector:detector]
                            forKey:@(LynxGestureHandlerOptionLongPress)];
    } else if (detector.gestureType == LynxGestureTypeNative) {
      [gestureHandlerMap setObject:[[LynxPanGestureHandler alloc] initWithSign:sign
                                                                       context:lynxContext
                                                                        member:member
                                                                      detector:detector]
                            forKey:@(LynxGestureTypeNative)];
    }
  }];

  return gestureHandlerMap;
}

- (BOOL)isGestureTypeMatched:(NSUInteger)typeMask {
  return NO;
}

- (BOOL)canActiveWithCurrentGesture:(CGPoint)deltaPoint {
  return _status == LynxGestureHandlerStateActive;
}

- (BOOL)isCurrentGestureEnd {
  return _status == LynxGestureHandlerStateEnd;
}

- (void)reset {
  _status = LynxGestureHandlerStateInit;
}

- (void)activate {
  _status = LynxGestureHandlerStateActive;
  if (_gestureMember) {
    [_gestureMember onPlatformGestureStatusChanged:_status];
  }
}

- (void)fail {
  // When in the end state, the process ends directly.
  if (_status != LynxGestureHandlerStateEnd) {
    if (_status == LynxGestureHandlerStateBegin) {
      _status = LynxGestureHandlerStateCancel;
    } else {
      _status = LynxGestureHandlerStateCancel;
    }
  }
  if (_gestureMember) {
    [_gestureMember onPlatformGestureStatusChanged:_status];
  }
}

- (void)end {
  _status = LynxGestureHandlerStateEnd;
  if (_gestureMember) {
    [_gestureMember onPlatformGestureStatusChanged:_status];
  }
}

- (void)begin {
  _status = LynxGestureHandlerStateBegin;
  if (_gestureMember) {
    [_gestureMember onPlatformGestureStatusChanged:_status];
  }
}

- (void)ignore {
  _status = LynxGestureHandlerStateUndetermined;
}

- (void)begin:(NSUInteger)typeMask
         point:(CGPoint)point
       touches:(NSSet<UITouch *> *_Nullable)touches
         event:(UIEvent *_Nullable)event
    touchEvent:(LynxTouchEvent *)touchEvent {
  if ([self isGestureTypeMatched:typeMask]) {
    [self onBegin:point touchEvent:touchEvent];
  }
}

- (void)update:(NSUInteger)typeMask
          point:(CGPoint)point
        touches:(NSSet<UITouch *> *_Nullable)touches
          event:(UIEvent *_Nullable)event
     touchEvent:(LynxTouchEvent *)touchEvent
    extraBundle:(LynxGestureExtraBundle *_Nullable)extraBundle {
  if ([self isGestureTypeMatched:typeMask]) {
    [self onUpdate:point touchEvent:touchEvent extraBundle:extraBundle];
  }
}

- (void)end:(NSUInteger)typeMask
         point:(CGPoint)point
       touches:(NSSet<UITouch *> *_Nullable)touches
         event:(UIEvent *_Nullable)event
    touchEvent:(LynxTouchEvent *)touchEvent {
  if ([self isGestureTypeMatched:typeMask]) {
    [self onEnd:point touchEvent:touchEvent];
  }
}

- (void)handleConfigMap:(nullable NSMutableDictionary *)config {
}

- (void)handleUIEvent:(NSString *const)touchType
                 touches:(NSSet<UITouch *> *)touches
                   event:(UIEvent *_Nullable)event
              touchEvent:(LynxTouchEvent *_Nullable)touchEvent
              flingPoint:(CGPoint)flingPoint
    handleBySimultaneous:(BOOL)handleSimultaneous
             extraBundle:(LynxGestureExtraBundle *_Nullable)extraBundle {
  [self onHandle:touchType
                   touches:touches
                     event:event
                touchEvent:touchEvent
                flingPoint:flingPoint
      handleBySimultaneous:handleSimultaneous
               extraBundle:extraBundle];
}

- (void)onHandle:(NSString *const)touchType
                 touches:(NSSet<UITouch *> *)touches
                   event:(UIEvent *_Nullable)event
              touchEvent:(LynxTouchEvent *_Nullable)touchEvent
              flingPoint:(CGPoint)flingPoint
    handleBySimultaneous:(BOOL)handleSimultaneous
             extraBundle:(LynxGestureExtraBundle *_Nullable)extraBundle {
}

- (BOOL)isEnd {
  return _status == LynxGestureHandlerStateEnd;
}

- (BOOL)isActive {
  return _status == LynxGestureHandlerStateActive;
}

- (void)onBegin:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
}

- (void)onStart:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
}

- (void)onUpdate:(CGPoint)point
      touchEvent:(LynxTouchEvent *_Nullable)touchEvent
     extraBundle:(LynxGestureExtraBundle *_Nullable)extraBundle {
}

- (void)onEnd:(CGPoint)point touchEvent:(LynxTouchEvent *_Nullable)touchEvent {
}

- (BOOL)onBeginEnabled {
  return _enableFlags[ON_BEGIN].boolValue;
}

- (BOOL)onStartEnabled {
  return _enableFlags[ON_START].boolValue;
}

- (BOOL)onUpdateEnabled {
  return _enableFlags[ON_UPDATE].boolValue;
}

- (BOOL)onEndEnabled {
  return _enableFlags[ON_END].boolValue;
}

- (void)onTouchesDown:(LynxTouchEvent *)touchEvent {
  if ([_enableFlags[ON_TOUCHES_DOWN] boolValue]) {
    [self sendGestureEvent:ON_TOUCHES_DOWN params:[self eventParamsFromTouchEvent:touchEvent]];
  }
}

- (void)onTouchesMove:(LynxTouchEvent *)touchEvent {
  if ([_enableFlags[ON_TOUCHES_MOVE] boolValue]) {
    [self sendGestureEvent:ON_TOUCHES_MOVE params:[self eventParamsFromTouchEvent:touchEvent]];
  }
}

- (void)onTouchesUp:(LynxTouchEvent *)touchEvent {
  if ([_enableFlags[ON_TOUCHES_UP] boolValue]) {
    [self sendGestureEvent:ON_TOUCHES_UP params:[self eventParamsFromTouchEvent:touchEvent]];
  }
}

- (void)onTouchesCancel:(LynxTouchEvent *)touchEvent {
  if ([_enableFlags[ON_TOUCHES_CANCEL] boolValue]) {
    [self sendGestureEvent:ON_TOUCHES_CANCEL params:[self eventParamsFromTouchEvent:touchEvent]];
  }
}

- (void)sendGestureEvent:(NSString *)eventName params:(NSDictionary *)eventParams {
  if (_gestureDetector) {
    [self.context.eventEmitter
        dispatchGestureEvent:_gestureDetector.gestureID
                       event:[[LynxCustomEvent alloc] initWithName:eventName
                                                        targetSign:self.sign
                                                            params:eventParams]];
  }
}

- (NSDictionary *)eventParamsFromTouchEvent:(LynxTouchEvent *)touchEvent {
  NSMutableDictionary *params = [NSMutableDictionary dictionary];

  if (touchEvent) {
    params[@"timestamp"] = @(CACurrentMediaTime() * 1000);
    params[@"type"] = touchEvent.eventName ?: @"unknown";
    params[@"x"] = @([touchEvent viewPoint].x);
    params[@"y"] = @([touchEvent viewPoint].y);
    params[@"pageX"] = @([touchEvent pagePoint].x);
    params[@"pageY"] = @([touchEvent pagePoint].y);
    params[@"clientX"] = @([touchEvent clientPoint].x);
    params[@"clientY"] = @([touchEvent clientPoint].y);
  }

  return params;
}

@end
