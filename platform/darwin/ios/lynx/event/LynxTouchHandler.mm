// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTouchHandler.h"
#import <UIKit/UIGestureRecognizerSubclass.h>
#import "LynxTouchHandler+Internal.h"

#import "LynxBaseInspectorOwner.h"
#import "LynxBaseLogBoxProxy.h"
#import "LynxEnv.h"
#import "LynxEventEmitter.h"
#import "LynxEventHandler+Internal.h"
#import "LynxGestureArenaManager.h"
#import "LynxGestureFlingTrigger.h"
#import "LynxGestureHandlerTrigger.h"
#import "LynxLog.h"
#import "LynxTouchEvent.h"
#import "LynxUI+Internal.h"
#import "LynxUI.h"
#import "LynxWeakProxy.h"

#include <deque>
#include <map>
#include <set>

@interface EventTargetDetail : NSObject

@property(nonatomic) CGPoint downPoint;
@property(nonatomic) CGPoint preTouchPoint;
@property(nonatomic) NSNumber* identifier;
@property(nonatomic, weak) id<LynxEventTarget> ui;

@end

@implementation EventTargetDetail

- (instancetype)initWithUI:(id<LynxEventTarget>)ui {
  if (self = [super init]) {
    _ui = ui;
    _preTouchPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    _downPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    _identifier = @0;
  }
  return self;
}

@end

@interface LynxTouchHandler () <UIGestureRecognizerDelegate>
@end

@implementation LynxTouchHandler {
  __weak LynxEventHandler* _eventHandler;
  std::deque<__weak id<LynxEventTarget>> deque_;
  CGPoint _preTouchPoint;
  CGPoint _downPoint;
  BOOL _touchMoved;
  BOOL _touchMoving;
  BOOL _touchBegin;
  BOOL _touchEndOrCancel;
  BOOL _touchOutSide;
  BOOL _gestureRecognized;
  BOOL _enableTouchRefactor;
  BOOL _enableEndGestureAtLastFingerUp;
  BOOL _enableTouchPseudo;
  BOOL _enableMultiTouch;
  NSMutableSet<UITouch*>* _touches;
  UIEvent* _event;
  // Touch -> Target, a target corresponds to multiple touches.
  std::map<UITouch*, EventTargetDetail*> touches_map_;
  NSMutableDictionary<NSString*, id<LynxEventTargetBase>>* active_target_map_;
  // Align the Android side so that each finger has a corresponding id,
  // which is generated when the finger is pressed and recycled when the
  // finger is lifted, and the period remains unchanged.
  std::set<uint32_t> reuse_id_pool_;
  LynxGestureVelocityTracker* _velocityTracker;
  NSMutableDictionary<NSString*, LynxWeakProxy*>* _outerGestures;
  NSTimeInterval _timestamp;
  NSString* _preTargetInlineCSSText;
}

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler {
  if (self = [super init]) {
    _eventHandler = eventHandler;
    deque_ = std::deque<__weak id<LynxEventTarget>>();
    _touchDeque = [[NSMutableArray alloc] init];
    self.cancelsTouchesInView = NO;
    self.delaysTouchesBegan = NO;
    self.delaysTouchesEnded = NO;
    self.delegate = self;
    _preTouchPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    _downPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    _touchMoved = NO;
    _touchBegin = NO;
    // After manual testing, the default value on iOS is close to 45.
    _tapSlop = 45;
    _touchEndOrCancel = NO;
    _touches = [NSMutableSet set];
    _event = nil;
    _target = nil;
    _preTarget = nil;
    _enableTouchPseudo = NO;
    _enableTouchRefactor = NO;
    _enableEndGestureAtLastFingerUp = NO;
    touches_map_ = std::map<UITouch*, EventTargetDetail*>();
    active_target_map_ = [NSMutableDictionary new];
    reuse_id_pool_ = std::set<uint32_t>();
    _outerGestures = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)setupVelocityTracker:(UIView*)rootView {
  _velocityTracker = [[LynxGestureVelocityTracker alloc] initWithRootView:rootView];
}

- (void)onGestureRecognized {
  _gestureRecognized = YES;
}

- (void)setEnableTouchRefactor:(BOOL)enable {
  _enableTouchRefactor = enable;
}

- (void)setEnableEndGestureAtLastFingerUp:(BOOL)enable {
  _enableEndGestureAtLastFingerUp = enable;
}

- (void)setEnableTouchPseudo:(BOOL)enable {
  // When disable fiber arch, setHasTouchPseudo will be exec twice.
  // Normally, it will exec setHasTouchPseudo in onPageConfigDecoded first.
  // In case not following this order in the future and exec setHasTouchPseudo in updateEventInfo
  // first, let _enableTouchPseudo = _enableTouchPseudo || enable;
  _enableTouchPseudo = _enableTouchPseudo || enable;
}

- (void)setEnableMultiTouch:(BOOL)enable {
  _enableMultiTouch = enable;
}

- (BOOL)isEnableAndGetMultiTouch {
  return _enableMultiTouch && _hasMultiTouch;
}

- (void)initTouchEnv {
  _touchBegin = YES;
  _touchMoved = NO;
  _touchMoving = NO;
  _tapSlop = 45;
  _touchOutSide = NO;
  _touchEndOrCancel = NO;
  _gestureRecognized = NO;
  [_eventHandler resetEventEnv];
  [_touchDeque removeAllObjects];
  _hasMultiTouch = NO;
  touches_map_.clear();
  [active_target_map_ removeAllObjects];
  reuse_id_pool_.clear();
}

- (void)initClickEnv {
  for (auto& target : deque_) {
    [target offResponseChain];
  }
  deque_.clear();
  if (_eventHandler == nil || _eventHandler.touchRecognizer == nil) {
    _touchOutSide = YES;
    return;
  }
  id<LynxEventTarget> ui = _eventHandler.touchTarget;
  while (ui != nil) {
    deque_.push_front(ui);
    ui = ui.parentTarget;
  }
  for (int i = static_cast<int>(deque_.size() - 1); i >= 0; --i) {
    id<LynxEventTarget> ui = deque_[i];
    if (ui == nil) {
      deque_.clear();
      break;
    } else if (ui.eventSet == nil || [ui.eventSet objectForKey:LynxEventClick] == nil) {
      deque_.pop_back();
    } else {
      break;
    }
  }
  for (auto& target : deque_) {
    [target onResponseChain];
  }
  if (deque_.empty()) {
    _touchOutSide = YES;
  } else {
    _touchOutSide = NO;
  }
}

- (void)resetTouchEnv {
  // Add reentrancy prevention logic to resetTouchEnv to prevent both _target and _preTarget from
  // being set to nil.
  if (_touchEndOrCancel) {
    return;
  }
  _touchBegin = NO;
  _touchMoved = NO;
  _touchMoving = NO;
  _tapSlop = 45;
  _touchEndOrCancel = YES;
  _gestureRecognized = NO;
  [_touches removeAllObjects];
  _event = nil;
  _preTarget = _target;
  _target = nil;
  _hasMultiTouch = NO;
  touches_map_.clear();
  [active_target_map_ removeAllObjects];
  reuse_id_pool_.clear();
  [_outerGestures removeAllObjects];
}

- (BOOL)isTouchMoving {
  return _touchMoving;
}

- (void)setEnableNewGesture:(BOOL)enableNewGesture {
  _enableNewGesture = enableNewGesture;
  if (!enableNewGesture) {
    // the config could be set on tasm thread, release the objects on UI thread
    // the config can not be changed after being set
    [self mainThreadDisableGesture];
  }
}

- (void)mainThreadDisableGesture {
  if ([NSThread isMainThread]) {
    _velocityTracker = nil;
  } else {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong typeof(weakSelf) strongSelf = weakSelf;
      if (strongSelf) {
        strongSelf->_velocityTracker = nil;
      }
    });
  }
}

- (NSInteger)setGestureArenaManagerAndGetIndex:(LynxGestureArenaManager*)gestureArenaManager {
  _gestureArenaManager = gestureArenaManager;
  [_gestureArenaManager.gestureHandlerTrigger addVelocityTracker:_velocityTracker];
  NSInteger index = [_gestureArenaManager.gestureHandlerTrigger addEventHandler:_eventHandler];
  return index;
}

- (void)removeGestureArenaManager:(NSInteger)index {
  [_gestureArenaManager.gestureHandlerTrigger removeVelocityTracker:index];
  [_gestureArenaManager.gestureHandlerTrigger removeEventHandler:index];
  _gestureArenaManager = nil;
}

// dispatch event for touch* .
// TODO(hexionghui): Merge two dispatchEvent interfaces into one.
- (void)dispatchTouchAndEvent:(NSString*)eventName
                       params:(NSMutableDictionary<NSString*, NSMutableArray<NSMutableArray*>*>*)
                                  params {
  LynxTouchEvent* event = [[LynxTouchEvent alloc] initWithName:eventName uiTouchMap:params];
  event.activeUIMap = active_target_map_;
  event.timestamp = _timestamp;
  [_eventHandler.eventEmitter dispatchMultiTouchEvent:event];
}

- (LynxTouchEvent*)initialTouchEvent:(NSString*)eventName
                            toTarget:(id<LynxEventTarget>)target
                               touch:(UITouch*)touch {
  CGPoint clientPoint = [touch locationInView:nil];
  CGPoint pagePoint = [touch locationInView:_eventHandler.rootView];
  CGPoint targetViewPoint = pagePoint;
  if ([_target isKindOfClass:[LynxUI class]]) {
    LynxUI* ui = (LynxUI*)_target;
    targetViewPoint = [touch locationInView:ui.view];
  }

  LynxTouchEvent* event = [[LynxTouchEvent alloc] initWithName:eventName
                                                     targetTag:target.signature
                                                   clientPoint:clientPoint
                                                     pagePoint:pagePoint
                                                     viewPoint:targetViewPoint];
  return event;
}

// dispatch event for tap, click, longpress.
- (LynxTouchEvent*)dispatchEvent:(NSString*)eventName
                        toTarget:(id<LynxEventTarget>)target
                           touch:(UITouch*)touch {
  CGPoint clientPoint = [touch locationInView:nil];
  CGPoint pagePoint = [touch locationInView:_eventHandler.rootView];
  CGPoint viewPoint = pagePoint;
  if ([_target isKindOfClass:[LynxUI class]]) {
    LynxUI* ui = (LynxUI*)_target;
    viewPoint = [touch locationInView:ui.view];
  }

  return [self dispatchEvent:eventName
                    toTarget:target
                       phase:touch.phase
                 clientPoint:clientPoint
                   pagePoint:pagePoint
                   viewPoint:viewPoint];
}

- (LynxTouchEvent*)dispatchEvent:(NSString*)eventName
                        toTarget:(id<LynxEventTarget>)target
                           phase:(UITouchPhase)phase
                     clientPoint:(CGPoint)clientPoint
                       pagePoint:(CGPoint)pagePoint
                       viewPoint:(CGPoint)viewPoint {
  LynxTouchEvent* event = [[LynxTouchEvent alloc] initWithName:eventName
                                                     targetTag:target.signature
                                                   clientPoint:clientPoint
                                                     pagePoint:pagePoint
                                                     viewPoint:viewPoint];
  event.eventTarget = target;
  event.timestamp = _timestamp;
  [_eventHandler.eventEmitter dispatchTouchEvent:event];

  if (eventName == LynxEventTouchStart) {
    _preTouchPoint = clientPoint;
    _downPoint = clientPoint;
    [self inspectHitTarget];
    if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
      [self showMessageOnConsole:
                [NSString stringWithFormat:@"LynxTouchHandler: hit the target with sign = %ld",
                                           target.signature]
                       withLevel:LynxLogBoxLevelInfo];
    }
  } else if (eventName == LynxEventTouchEnd || eventName == LynxEventTouchCancel) {
    _preTouchPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
  }

  return event;
}

/**
 * Used to output logs to the console of DevTool. This function is effective only when DevTool is
 * connected.
 * @param msg Information related to event processing.
 * @param level The log level.
 */
- (void)showMessageOnConsole:(NSString*)msg withLevel:(int32_t)level {
  id<LynxBaseInspectorOwner> inspectorOwner =
      ((LynxView*)((LynxUI*)_eventHandler.uiOwner.rootUI).view).baseInspectorOwner;
  if (!inspectorOwner) {
    return;
  }
  [inspectorOwner showMessageOnConsole:msg withLevel:level];
}

/**
 * Used to highlight the nodes that are actually touched. This function takes effect only when
 * LynxDevtool and HighlightTouch are turned on.
 */
- (void)inspectHitTarget {
  if (![LynxEnv.sharedInstance highlightTouchEnabled]) {
    return;
  }
  id<LynxBaseInspectorOwner> inspectorOwner =
      ((LynxView*)_eventHandler.rootView).baseInspectorOwner;
  if (!inspectorOwner) {
    return;
  }
  NSError* error = nil;
  NSMutableDictionary* jsonDict = [NSMutableDictionary dictionary];
  jsonDict[@"id"] = @1;
  jsonDict[@"method"] = @"DOM.setAttributesAsText";
  if (_preTargetInlineCSSText) {
    NSDictionary* paramsDict = @{
      @"nodeId" : @([_preTarget signature]),
      @"text" : [NSString stringWithFormat:@"style=\"%@\"", _preTargetInlineCSSText],
      @"name" : @"style"
    };
    jsonDict[@"params"] = paramsDict;
    NSData* jsonData = [NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:&error];
    if (jsonData) {
      NSString* jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
      [inspectorOwner invokeCDPFromSDK:jsonString
                          withCallback:^(NSString* result){
                          }];
    }
  }

  jsonDict[@"method"] = @"CSS.getInlineStylesForNode";
  NSMutableDictionary* paramsDict = [NSMutableDictionary dictionary];
  paramsDict[@"nodeId"] = @([_target signature]);
  jsonDict[@"params"] = paramsDict;
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:&error];
  if (jsonData) {
    NSString* jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    __weak typeof(self) weakSelf = self;
    [inspectorOwner
        invokeCDPFromSDK:jsonString
            withCallback:^(NSString* result) {
              __strong typeof(weakSelf) strongSelf = weakSelf;
              NSError* error = nil;
              NSString* inlineStyle = result;
              NSRegularExpression* regex = [NSRegularExpression
                  regularExpressionWithPattern:@"\"cssText\"\\s*:\\s*\"([^\"]*)\""
                                       options:NSRegularExpressionCaseInsensitive
                                         error:&error];
              if (inlineStyle && !error) {
                NSTextCheckingResult* match =
                    [regex firstMatchInString:inlineStyle
                                      options:0
                                        range:NSMakeRange(0, inlineStyle.length)];
                if (match) {
                  NSRange matchRange = [match rangeAtIndex:1];
                  strongSelf->_preTargetInlineCSSText = [inlineStyle substringWithRange:matchRange];
                }
              }
              NSString* cssText = [strongSelf->_preTargetInlineCSSText ?: @""
                  stringByAppendingString:
                      @"background-color:#9CC4E6;border-width:2px;border-color:red;"];
              jsonDict[@"method"] = @"DOM.setAttributesAsText";
              paramsDict[@"text"] = [NSString stringWithFormat:@"style=\"%@\"", cssText];
              paramsDict[@"name"] = @"style";
              jsonDict[@"params"] = paramsDict;
              NSData* jsonData = [NSJSONSerialization dataWithJSONObject:jsonDict
                                                                 options:0
                                                                   error:&error];
              if (jsonData) {
                NSString* jsonString = [[NSString alloc] initWithData:jsonData
                                                             encoding:NSUTF8StringEncoding];
                [inspectorOwner invokeCDPFromSDK:jsonString
                                    withCallback:^(NSString* result){
                                    }];
              }
            }];
  }
}

- (void)addMap:(NSMutableDictionary<NSString*, NSMutableArray<NSMutableArray*>*>*)dict
         touch:(UITouch*)touch {
  EventTargetDetail* detail = touches_map_[touch];
  NSString* sign = [[NSNumber numberWithInteger:[detail.ui signature]] stringValue];
  if (![dict objectForKey:sign]) {
    [dict setObject:[NSMutableArray new] forKey:sign];
  }
  NSMutableArray<NSMutableArray*>* events = [dict objectForKey:sign];

  NSMutableArray* event = [NSMutableArray new];
  CGPoint clientPoint = [touch locationInView:nil];
  CGPoint pagePoint = [touch locationInView:_eventHandler.rootView];
  CGPoint viewPoint = pagePoint;
  if ([detail.ui isKindOfClass:[LynxUI class]]) {
    LynxUI* ui = (LynxUI*)detail.ui;
    viewPoint = [touch locationInView:ui.view];
  }

  [event addObject:detail.identifier];
  // client
  [event addObject:[NSNumber numberWithDouble:clientPoint.x]];
  [event addObject:[NSNumber numberWithDouble:clientPoint.y]];
  // page
  [event addObject:[NSNumber numberWithDouble:pagePoint.x]];
  [event addObject:[NSNumber numberWithDouble:pagePoint.y]];
  // x,y
  [event addObject:[NSNumber numberWithDouble:viewPoint.x]];
  [event addObject:[NSNumber numberWithDouble:viewPoint.y]];
  [events addObject:event];
}

// OnTouchesBegan, generate event target response chain. And traversed the event target response
// chain to make the target's touch state pseudo-class take effect.
- (void)onTouchesBegan {
  id<LynxEventTarget> target = _target;
  // TODO(songshourui.null): for fiber Arch, need to set enableTouchPseudo YES by default
  if (target == nil) {
    return;
  }
  while (target != nil) {
    [_touchDeque addObject:[LynxWeakProxy proxyWithTarget:target]];
    [target onPseudoStatusFrom:LynxTouchPseudoStateNone changedTo:LynxTouchPseudoStateActive];
    if (_enableTouchPseudo) {
      [_eventHandler.eventEmitter onPseudoStatusChanged:(int32_t)target.signature
                                          fromPreStatus:(int32_t)LynxTouchPseudoStateNone
                                        toCurrentStatus:(int32_t)LynxTouchPseudoStateActive];
    }
    if (![target enableTouchPseudoPropagation]) {
      break;
    }
    target = [target parentTarget];
  }
  _touchDeque = [[[_touchDeque reverseObjectEnumerator] allObjects] mutableCopy];
}

- (UITouch*)findFirstValidTouch:(NSSet<UITouch*>*)touches {
  __block UITouch* touch = nil;
  [touches enumerateObjectsUsingBlock:^(UITouch* _Nonnull obj, BOOL* _Nonnull stop) {
    if ([_touches containsObject:obj]) {
      touch = obj;
      *stop = YES;
    }
  }];
  return touch;
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
    [self showMessageOnConsole:
              [NSString stringWithFormat:@"LynxTouchHandler: receive touch for lynx %ld, touch %d",
                                         [_eventHandler.rootView hash], 0]
                     withLevel:LynxLogBoxLevelInfo];
  }
  _LogI(@"Lynxview LynxTouchHandler touchesBegan %p: ", _eventHandler.rootView);

  if (self.state == UIGestureRecognizerStatePossible) {
    self.state = UIGestureRecognizerStateBegan;
  } else if (self.state == UIGestureRecognizerStateBegan) {
    self.state = UIGestureRecognizerStateChanged;
  }
  if (_eventHandler.touchTarget == nil) {
    return;
  }

  if ([_touches count] == 0) {
    [self initTouchEnv];
    [self initClickEnv];
    [_eventHandler resetEventEnv];
    _target = _eventHandler.touchTarget;
    _event = event;
  }

  if (!_enableMultiTouch && [_touches count] != 0) {
    // TODO(hexionghui): Align with the :active logic on the Android side: When one finger long
    // presses, another finger tap will not invalidate :active, only after the long press is
    // released.
    [self deactivatePseudoState:LynxTouchPseudoStateActive];
    // The method only dispatch multiple touch events to sepcific view.
    [_target dispatchTouch:LynxEventTouchStart touches:touches withEvent:event];
    return;
  }

  [self.gestureArenaManager setActiveUIToArena:_eventHandler.touchTarget];

  UITouch* firstTouch = touches.allObjects.firstObject;

  NSMutableDictionary<NSString*, NSMutableArray<NSMutableArray*>*>* dict =
      [NSMutableDictionary new];
  for (UITouch* touch in touches) {
    _timestamp = [[NSDate date] timeIntervalSince1970];
    id<LynxEventTarget> target =
        [_eventHandler hitTestInner:[touch locationInView:_eventHandler.rootView] withEvent:event];
    EventTargetDetail* detail = [[EventTargetDetail alloc] initWithUI:target];

    detail.downPoint = [touch locationInView:_eventHandler.rootView];
    detail.preTouchPoint = detail.downPoint;
    NSNumber* identifier = nil;
    if (!reuse_id_pool_.empty()) {
      identifier = [NSNumber numberWithInt:*reuse_id_pool_.cbegin()];
      reuse_id_pool_.erase(reuse_id_pool_.cbegin());
    } else {
      identifier = [NSNumber numberWithUnsignedLong:touches_map_.size()];
    }
    detail.identifier = identifier;
    touches_map_[touch] = detail;
    [active_target_map_ setObject:target forKey:[@([target signature]) stringValue]];
    [self addMap:dict touch:touch];

    LynxTouchEvent* touchEvent = nil;

    if (!_enableMultiTouch) {
      // TODO(hexionghui): Fix the problem: Multiple touchstart events are triggered when
      // multiple fingers touch at the same time.
      touchEvent = [self dispatchEvent:LynxEventTouchStart toTarget:target touch:touch];
    } else {
      touchEvent = [self initialTouchEvent:LynxEventTouchStart toTarget:target touch:touch];
    }

    [_touches addObject:touch];

    if (touch == firstTouch) {
      // Dispatch TouchStart
      [self.gestureArenaManager dispatchBubble:LynxEventTouchStart touchEvent:touchEvent];
      [self.gestureArenaManager dispatchTouchToArena:LynxEventTouchStart
                                             touches:touches
                                               event:event
                                          touchEvent:touchEvent];
    }
  }

  if (!_enableMultiTouch) {
    // TODO(hexionghui): Fix the problem: the :active event is triggered when
    // multiple fingers touch at the same time.
    [self onTouchesBegan];
  } else {
    [self dispatchTouchAndEvent:LynxEventTouchStart params:dict];
    if ([_touches count] > 1) {
      _hasMultiTouch = YES;
    } else {
      // For the :active logic, it only support single finger.
      [self onTouchesBegan];
    }
  }
  [_target dispatchTouch:LynxEventTouchStart touches:touches withEvent:event];
}

// OnTouchesMove, the touched event target may change. Disable the touch pseudo class for
// targets not on the response chain.
- (void)onTouchesMoveWithTarget:(id<LynxEventTarget>)target {
  if (target == nil || [_touchDeque count] == 0) {
    return;
  }
  id<LynxEventTarget> newTarget = target;
  NSMutableArray<id<LynxEventTarget>>* queue = [[NSMutableArray alloc] init];

  while (newTarget != nil) {
    [queue addObject:newTarget];
    if (![newTarget enableTouchPseudoPropagation]) {
      break;
    }
    newTarget = newTarget.parentTarget;
  }
  queue = [[[queue reverseObjectEnumerator] allObjects] mutableCopy];

  NSInteger index = -1;
  for (NSInteger i = 0; i < (NSInteger)[_touchDeque count] && i < (NSInteger)[queue count]; ++i) {
    id<LynxEventTarget> preTarget = _touchDeque[i].target;
    id<LynxEventTarget> nowTarget = queue[i];
    if (preTarget.signature != nowTarget.signature) {
      break;
    }
    index = i;
  }

  for (NSInteger i = (NSInteger)[_touchDeque count] - 1; i >= index + 1; --i) {
    id<LynxEventTarget> ui = _touchDeque[i].target;
    [ui onPseudoStatusFrom:LynxTouchPseudoStateActive changedTo:LynxTouchPseudoStateNone];
    if (_enableTouchPseudo) {
      [_eventHandler.eventEmitter onPseudoStatusChanged:(int32_t)ui.signature
                                          fromPreStatus:(int32_t)LynxTouchPseudoStateActive
                                        toCurrentStatus:(int32_t)LynxTouchPseudoStateNone];
    }
    [_touchDeque removeLastObject];
  }
}

- (BOOL)checkOuterGestureChanged:(NSSet<UITouch*>*)touches {
  if ([self enableSimultaneousTouch]) {
    return NO;
  }
  NSArray* gestures = [_outerGestures allValues];
  for (LynxWeakProxy* gesture in gestures) {
    if (((UIGestureRecognizer*)gesture.target).state == UIGestureRecognizerStateChanged) {
      self.state = UIGestureRecognizerStateCancelled;
      [self dispatchEvent:LynxEventTouchCancel
                 toTarget:_target
                    touch:[self findFirstValidTouch:touches]];
      return YES;
    }
  }

  return NO;
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if ([self checkOuterGestureChanged:touches]) {
    [self onTouchEndOrCancel];
    [self resetTouchEnv];
    return;
  }

  _LogI(@"Lynxview LynxTouchHandler touchesMoved %p: ", _eventHandler.rootView);

  _touchMoving = YES;
  self.state = UIGestureRecognizerStateChanged;
  if (_target == nil || touches_map_.empty()) {
    return;
  }

  NSMutableDictionary<NSString*, NSMutableArray<NSMutableArray*>*>* dict =
      [NSMutableDictionary new];

  UITouch* firstTouch = [self findFirstValidTouch:touches];

  for (UITouch* touch in touches) {
    if (![_touches containsObject:touch]) {
      continue;
    }
    _timestamp = [[NSDate date] timeIntervalSince1970];

    CGPoint point = [touch locationInView:_eventHandler.rootView];
    if (touches_map_.find(touch) == touches_map_.end()) {
      LLogError(@"Lynxview LynxTouchHandler touche miss: %f %f", point.x, point.y);
    }
    EventTargetDetail* detail = touches_map_[touch];
    if (point.x != detail.preTouchPoint.x || point.y != detail.preTouchPoint.y) {
      [self addMap:dict touch:touch];
    }
    detail.preTouchPoint = point;

    if (point.x != _preTouchPoint.x || point.y != _preTouchPoint.y) {
      LynxTouchEvent* touchEvent = nil;
      if (!_enableMultiTouch) {
        touchEvent = [self dispatchEvent:LynxEventTouchMove toTarget:_target touch:touch];
      } else {
        touchEvent = [self initialTouchEvent:LynxEventTouchMove toTarget:_target touch:touch];
      }

      if (touch == firstTouch) {
        // Dispatch TouchMove
        [self.gestureArenaManager dispatchBubble:LynxEventTouchMove touchEvent:touchEvent];
        [self.gestureArenaManager dispatchTouchToArena:LynxEventTouchMove
                                               touches:touches
                                                 event:event
                                            touchEvent:touchEvent];
      }

      _touchOutSide = _gestureRecognized || _touchOutSide;
      if ([_touchDeque count] >= 0 || !_touchOutSide) {
        id<LynxEventTarget> target = [_eventHandler hitTestInner:point withEvent:event];
        _touchOutSide = [self onTouchMove:touch withEvent:event withTarget:target];
        NSInteger slideTargetSign = [_eventHandler canRespondTapOrClickEvent:_target];
        NSInteger propsTargetSign = [_eventHandler canRespondTapOrClickWhenUISlideByProps:_target];
        if (_gestureRecognized || slideTargetSign != -1 || propsTargetSign != -1 || _touchMoved) {
          // Only when the first finger is moved, :active will be disabled.
          if ([detail.identifier isEqual:@0]) {
            if ((point.x - touches_map_[touch].downPoint.x) *
                        (point.x - touches_map_[touch].downPoint.x) +
                    (point.y - touches_map_[touch].downPoint.y) *
                        (point.y - touches_map_[touch].downPoint.y) >
                _tapSlop * _tapSlop) {
              // TODO(hexionghui): Align with the :active logic on the Android side: When one finger
              // long presses, another finger touch will not invalidate :active, only after the long
              // press is released.
              [self deactivatePseudoState:LynxTouchPseudoStateActive];
            }
          }
        }
      }
    }
    _preTouchPoint = point;
  }

  if (_enableMultiTouch) {
    [self dispatchTouchAndEvent:LynxEventTouchMove params:dict];
  }
  [_target dispatchTouch:LynxEventTouchMove touches:touches withEvent:event];
}

// OnTouchEndOrCancel, the touched event target may change. Disable the touch pseudo class for all
// targets.
- (void)onTouchEndOrCancel {
  [self deactivatePseudoState:LynxTouchPseudoStateAll];
}

// change all targets from active to none
- (void)deactivatePseudoState:(int32_t)state {
  for (LynxWeakProxy* proxy : _touchDeque) {
    id<LynxEventTarget> ui = proxy.target;
    if (!ui) {
      continue;
    }
    if (_enableTouchPseudo) {
      [_eventHandler.eventEmitter onPseudoStatusChanged:(int32_t)ui.signature
                                          fromPreStatus:ui.pseudoStatus
                                        toCurrentStatus:ui.pseudoStatus & ~state];
    }
    [ui onPseudoStatusFrom:ui.pseudoStatus changedTo:ui.pseudoStatus & ~state];
  }
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
    [self showMessageOnConsole:
              [NSString stringWithFormat:@"LynxTouchHandler: receive touch for lynx %ld, touch %d",
                                         [_eventHandler.rootView hash], 1]
                     withLevel:LynxLogBoxLevelInfo];
  }
  _LogI(@"Lynxview LynxTouchHandler touchesEnded %p: ", _eventHandler.rootView);

  if ([self isAllTouchesAreCancelledOrEnded:_touches]) {
    self.state = UIGestureRecognizerStateEnded;
  } else if ([self hasAnyTouchesChanged:_touches]) {
    self.state = UIGestureRecognizerStateChanged;
  }

  NSMutableDictionary<NSString*, NSMutableArray<NSMutableArray*>*>* dict =
      [NSMutableDictionary new];

  UITouch* firstTouch = [self findFirstValidTouch:touches];

  for (UITouch* touch in touches) {
    if (![_touches containsObject:touch]) {
      continue;
    }
    _timestamp = [[NSDate date] timeIntervalSince1970];

    if (touches_map_.find(touch) == touches_map_.end()) {
      CGPoint point = [touch locationInView:_eventHandler.rootView];
      LLogError(@"Lynxview LynxTouchHandler touche miss: %f %f", point.x, point.y);
    }
    [self addMap:dict touch:touch];
    LynxTouchEvent* touchEvent = nil;
    if (!_enableMultiTouch && _target) {
      touchEvent = [self dispatchEvent:LynxEventTouchEnd toTarget:_target touch:touch];
      // TODO(hexionghui): Fix the problem: Multiple click events are triggered when
      // multiple fingers touch at the same time.
      // For the click event, it only support single finger.
      [self sendClickEvent:touch];
    } else {
      touchEvent = [self initialTouchEvent:LynxEventTouchEnd toTarget:_target touch:touch];
    }

    if (firstTouch == touch) {
      // Dispatch TouchEnd
      [self.gestureArenaManager dispatchBubble:LynxEventTouchEnd touchEvent:touchEvent];
      [self.gestureArenaManager dispatchTouchToArena:LynxEventTouchEnd
                                             touches:touches
                                               event:event
                                          touchEvent:touchEvent];
    }

    [_touches removeObject:touch];
  }

  if (_enableMultiTouch) {
    [self dispatchTouchAndEvent:LynxEventTouchEnd params:dict];
    for (UITouch* touch in touches) {
      reuse_id_pool_.insert([touches_map_[touch].identifier intValue]);
      touches_map_.erase(touch);
    }

    // For the click event, it only support single finger.
    if (!_hasMultiTouch) {
      [self sendClickEvent:[touches anyObject]];
    }
  }
  if (_target != nil) {
    [_target dispatchTouch:LynxEventTouchEnd touches:touches withEvent:event];
  } else if (_preTarget != nil) {
    // The method only dispatch multiple touch events to specific view, it only works when
    // all the touches are in a specific view.
    [_preTarget dispatchTouch:LynxEventTouchEnd touches:touches withEvent:event];
  }

  if ([_touches count] == 0) {
    if (_enableEndGestureAtLastFingerUp) {
      self.state = UIGestureRecognizerStateEnded;
    }
    [self onTouchEndOrCancel];
    [self resetTouchEnv];
  }
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
  if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
    [self showMessageOnConsole:
              [NSString stringWithFormat:@"LynxTouchHandler: receive touch for lynx %ld, touch %d",
                                         [_eventHandler.rootView hash], 3]
                     withLevel:LynxLogBoxLevelInfo];
  }
  _LogI(@"Lynxview LynxTouchHandler touchesCancelled %p: ", _eventHandler.rootView);

  if ([self isAllTouchesAreCancelledOrEnded:_touches]) {
    self.state = UIGestureRecognizerStateCancelled;
  } else if ([self hasAnyTouchesChanged:_touches]) {
    self.state = UIGestureRecognizerStateChanged;
  }

  NSMutableDictionary* dict = [NSMutableDictionary new];

  UITouch* firstTouch = [self findFirstValidTouch:touches];

  for (UITouch* touch in touches) {
    if (![_touches containsObject:touch]) {
      continue;
    }
    _timestamp = [[NSDate date] timeIntervalSince1970];

    if (touches_map_.find(touch) == touches_map_.end()) {
      CGPoint point = [touch locationInView:_eventHandler.rootView];
      LLogError(@"Lynxview LynxTouchHandler touche miss: %f %f", point.x, point.y);
    }
    [self addMap:dict touch:touch];

    LynxTouchEvent* touchEvent = nil;

    if (!_enableMultiTouch && _target) {
      touchEvent = [self dispatchEvent:LynxEventTouchCancel toTarget:_target touch:touch];
    } else {
      touchEvent = [self initialTouchEvent:LynxEventTouchCancel toTarget:_target touch:touch];
    }

    if (touch == firstTouch) {
      // Dispatch TouchCancel
      [self.gestureArenaManager dispatchBubble:LynxEventTouchCancel touchEvent:touchEvent];
      [self.gestureArenaManager dispatchTouchToArena:LynxEventTouchCancel
                                             touches:touches
                                               event:event
                                          touchEvent:touchEvent];
    }

    [_touches removeObject:touch];
  }

  if (_enableMultiTouch) {
    [self dispatchTouchAndEvent:LynxEventTouchCancel params:dict];
    for (UITouch* touch in touches) {
      reuse_id_pool_.insert([touches_map_[touch].identifier intValue]);
      touches_map_.erase(touch);
    }
  }
  [_target dispatchTouch:LynxEventTouchCancel touches:touches withEvent:event];

  if ([_touches count] == 0) {
    if (_enableEndGestureAtLastFingerUp) {
      self.state = UIGestureRecognizerStateCancelled;
    }
    [self onTouchEndOrCancel];
    [self resetTouchEnv];
  }
}

- (void)sendClickEvent:(UITouch*)touch {
  if (deque_.empty()) {
    return;
  }
  for (size_t i = 0; i < deque_.size(); ++i) {
    id<LynxEventTarget> target = deque_[i];
    if (![target isKindOfClass:[LynxUI class]]) {
      continue;
    }
    LynxUI* ui = (LynxUI*)target;
    if (ui != nil && ui.view != nil && [ui.view isKindOfClass:[UIScrollView class]]) {
      UIScrollView* view = (UIScrollView*)ui.view;
      if (view.decelerating) {
        return;
      }
    }
  }

  id<LynxEventTarget> ui = deque_.back();
  if (ui == nil) {
    return;
  }

  // TODO(songshourui.null): opt me
  // now there is a bad case that when
  //  scroll-view.contentOffset.y + scrollView.frame.size.height >
  //  scrollView.contentSize.height + scrollView.contentSize.height.contentInset.bottom or
  //  scroll-view.contentOffset.x + scrollView.frame.size.width >
  //  scrollView.contentSize.width + scrollView.contentSize.height.contentInset.right,
  // click event will be triggered. For this bad case, there is currently no good solution.
  NSInteger slideTargetSign = [_eventHandler canRespondTapOrClickEvent:ui];
  NSInteger propsTargetSign = [_eventHandler canRespondTapOrClickEvent:ui];
  if (_touchBegin == YES && _touchEndOrCancel == NO && _touchOutSide == NO && ui != nil &&
      _gestureRecognized == NO && slideTargetSign == -1 && propsTargetSign == -1) {
    // TODO check can send click event or not
    [self dispatchEvent:LynxEventClick toTarget:ui touch:touch];
  }
}

- (BOOL)onTouchMove:(UITouch*)touch
          withEvent:(UIEvent*)event
         withTarget:(id<LynxEventTarget>)target {
  if ((touches_map_[touch].preTouchPoint.x - touches_map_[touch].downPoint.x) *
              (touches_map_[touch].preTouchPoint.x - touches_map_[touch].downPoint.x) +
          (touches_map_[touch].preTouchPoint.y - touches_map_[touch].downPoint.y) *
              (touches_map_[touch].preTouchPoint.y - touches_map_[touch].downPoint.y) >
      _tapSlop * _tapSlop) {
    _touchMoved = YES;
  }
  if (_eventHandler == nil || deque_.empty() || target == nil) {
    return YES;
  }
  id<LynxEventTarget> newui = target;
  std::deque<id<LynxEventTarget>> deque;
  while (newui != nil && newui.parentTarget != newui) {
    deque.push_front(newui);
    newui = newui.parentTarget;
  }
  if (deque.size() < deque_.size()) {
    return YES;
  }
  for (size_t i = 0; i < deque_.size(); ++i) {
    id<LynxEventTarget> ui = deque_[i];
    if (ui == nil || ui.signature != deque[i].signature) {
      return YES;
    }
  }
  return NO;
}

- (BOOL)isDescendantOfLynxView:(UIGestureRecognizer*)gesture {
  return [gesture.view isDescendantOfView:_eventHandler.rootView];
}

- (BOOL)blockNativeEvent:(UIGestureRecognizer*)gestureRecognizer {
  id<LynxEventTarget> target = _eventHandler.touchTarget;
  BOOL res = NO;
  while (target != nil) {
    if ([target isKindOfClass:[LynxUI class]]) {
      LynxUI* ui = (LynxUI*)target;
      if ([ui blockNativeEvent:gestureRecognizer]) {
        res = YES;
        break;
      }
    }
    target = target.parentTarget;
  }
  return res;
}

- (BOOL)enableSimultaneousTouch {
  id<LynxEventTarget> target = _eventHandler.touchTarget;
  BOOL res = NO;
  while (target != nil) {
    if ([target isKindOfClass:[LynxUI class]]) {
      LynxUI* ui = (LynxUI*)target;
      if (ui.enableSimultaneousTouch) {
        res = YES;
        break;
      }
    }
    target = target.parentTarget;
  }
  return res;
}

// It is necessary to override this function to return "No" to ensure that the current gesture will
// not prevent other gestures.
- (BOOL)canPreventGestureRecognizer:(__unused UIGestureRecognizer*)preventedGestureRecognizer {
  return NO;
}

// Override this function to return "NO" if it is a LynxView gesture or an internal LynxView
// gesture, indicating that it will not be prevented by these gestures. Otherwise, return "YES" to
// indicate that it can be prevented by external gestures.
- (BOOL)canBePreventedByGestureRecognizer:(UIGestureRecognizer*)preventingGestureRecognizer {
  return ![preventingGestureRecognizer.view isDescendantOfView:_eventHandler.rootView];
}

#pragma mark - UIGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  // _enableTouchRefactor's default value is false. If this flag is true, the external gesture
  // which's state is possible or began will not cancel the Lynx iOS touch gesture see issue:#7920.
  if (_enableTouchRefactor && ![self isDescendantOfLynxView:otherGestureRecognizer] &&
      (otherGestureRecognizer.state == UIGestureRecognizerStatePossible ||
       otherGestureRecognizer.state == UIGestureRecognizerStateBegan)) {
    [_outerGestures setValue:[LynxWeakProxy proxyWithTarget:otherGestureRecognizer]
                      forKey:[@(otherGestureRecognizer.hash) stringValue]];

    return YES;
  }

  // Simultaneously recognize the velocityTracker
  if (otherGestureRecognizer == _velocityTracker.tracker) {
    return YES;
  }

  if ([self enableSimultaneousTouch]) {
    return YES;
  }

  auto res = ![self isDescendantOfLynxView:otherGestureRecognizer];
  if (res == YES && _touchBegin == YES && _touchEndOrCancel == NO) {
    _timestamp = [[NSDate date] timeIntervalSince1970];
    if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
      [self
          showMessageOnConsole:
              [NSString stringWithFormat:@"LynxTouchHandler: receive touch for lynx %ld, touch %d",
                                         [_eventHandler.rootView hash], 3]
                     withLevel:LynxLogBoxLevelInfo];
    }
    if (!_enableMultiTouch) {
      CGPoint windowLocation = [otherGestureRecognizer locationInView:otherGestureRecognizer.view];
      CGPoint clientPoint = [otherGestureRecognizer.view convertPoint:windowLocation
                                                               toView:_eventHandler.rootView];
      CGPoint viewPoint = [otherGestureRecognizer locationInView:gestureRecognizer.view];

      [self dispatchEvent:LynxEventTouchCancel
                 toTarget:_eventHandler.touchTarget
                    phase:UITouchPhaseCancelled
              clientPoint:clientPoint
                pagePoint:clientPoint
                viewPoint:viewPoint];
    } else {
      NSMutableDictionary* dict = [NSMutableDictionary new];
      for (UITouch* touch in _touches) {
        [self addMap:dict touch:touch];
      }
      [self dispatchTouchAndEvent:LynxEventTouchCancel params:dict];
    }
    [_target dispatchTouch:LynxEventTouchCancel touches:_touches withEvent:_event];

    [self onTouchEndOrCancel];
    [self resetTouchEnv];
  }
  return !res;
}

// Return YES to allow gestureRecognizer to begin execution only after otherGestureRecognizer fails.
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  if ([self isDescendantOfLynxView:otherGestureRecognizer]) {
    return NO;
  } else {
    if ([self blockNativeEvent:gestureRecognizer]) {
      return NO;
    } else {
      return YES;
    }
  }
}

// Return YES to allow otherGestureRecognizer to begin execution only after gestureRecognizer fails.
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  if ([self isDescendantOfLynxView:otherGestureRecognizer]) {
    return NO;
  } else {
    if ([self blockNativeEvent:gestureRecognizer]) {
      return YES;
    } else {
      return NO;
    }
  }
}

- (BOOL)isAllTouchesAreCancelledOrEnded:(NSSet<UITouch*>*)touches {
  for (UITouch* touch in touches) {
    if (touch.phase == UITouchPhaseBegan || touch.phase == UITouchPhaseMoved ||
        touch.phase == UITouchPhaseStationary) {
      return NO;
    }
  }
  return YES;
}

- (BOOL)hasAnyTouchesChanged:(NSSet<UITouch*>*)touches {
  for (UITouch* touch in touches) {
    if (touch.phase == UITouchPhaseBegan || touch.phase == UITouchPhaseMoved) {
      return YES;
    }
  }
  return NO;
}

@end
