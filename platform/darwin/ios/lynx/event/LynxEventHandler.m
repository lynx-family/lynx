// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBaseLogBoxProxy.h"
#import "LynxEnv.h"
#import "LynxEventHandler+Internal.h"
#import "LynxLog.h"
#import "LynxTouchHandler+Internal.h"
#import "LynxTouchHandler.h"
#import "LynxUI+Internal.h"
#import "LynxUnitUtils.h"
#import "LynxView+Internal.h"
#import "LynxView.h"
#import "LynxViewInternal.h"
#import "LynxWeakProxy.h"
#import "UIView+Lynx.h"

#pragma mark - LynxEventHandler
@interface LynxEventHandler ()

@property(nonatomic, readwrite) BOOL gestureRecognized;

@end

#pragma mark - CustomGestureRecognizerDelegate
@interface CustomGestureRecognizerDelegate : NSObject <UIGestureRecognizerDelegate>

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler;

@property(weak) LynxEventHandler* eventHandler;

@end

@implementation CustomGestureRecognizerDelegate

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler {
  self = [super init];
  if (self) {
    self.eventHandler = eventHandler;
  }
  return self;
}

@end

#pragma mark - TapGestureRecognizerDelegate
@interface TapGestureRecognizerDelegate : CustomGestureRecognizerDelegate

@end

@implementation TapGestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  return self.eventHandler.longPressRecognizer == otherGestureRecognizer;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  BOOL res = NO;
  if ([otherGestureRecognizer.view isDescendantOfView:self.eventHandler.rootView]) {
    if (otherGestureRecognizer.view.lynxEnableTapGestureSimultaneously) {
      res = YES;
    } else if ([otherGestureRecognizer.view isKindOfClass:[UIScrollView class]] &&
               ((UIScrollView*)otherGestureRecognizer.view).isDecelerating) {
      [self.eventHandler onGestureRecognizedByEventTarget:self.eventHandler.touchTarget];
    }
  } else if (self.eventHandler.enableSimultaneousTap) {
    res = YES;
  }
  if (!res && otherGestureRecognizer.view != self.eventHandler.rootView &&
      otherGestureRecognizer.state != UIGestureRecognizerStateFailed &&
      otherGestureRecognizer.state != UIGestureRecognizerStateCancelled) {
    if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
      [self.eventHandler.touchRecognizer
          showMessageOnConsole:
              [NSString stringWithFormat:@"LynxEventHandler: tap failed due to [gesture] %@ %@ %ld",
                                         NSStringFromClass([otherGestureRecognizer.view class]),
                                         NSStringFromClass([otherGestureRecognizer class]),
                                         otherGestureRecognizer.state]
                     withLevel:LynxLogBoxLevelWarning];
    }
  }
  return res;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
       shouldReceiveTouch:(UITouch*)touch {
  UIView* scrollableView = nil;

  if ([touch.view isKindOfClass:[UICollectionViewCell class]] ||
      [touch.view isKindOfClass:[UITableViewCell class]]) {
    scrollableView = touch.view.superview;
  } else if ([touch.view.superview isKindOfClass:[UICollectionViewCell class]] ||
             [touch.view.superview isKindOfClass:[UITableViewCell class]]) {
    scrollableView = touch.view.superview.superview;
  } else if ([touch.view isKindOfClass:[UICollectionView class]] ||
             [touch.view isKindOfClass:[UITableView class]]) {
    scrollableView = touch.view;
  }

  BOOL res = YES;
  if ([scrollableView isKindOfClass:[UICollectionView class]]) {
    res = !((UICollectionView*)scrollableView).isDecelerating;
  } else if ([scrollableView isKindOfClass:[UITableView class]]) {
    res = !((UITableView*)scrollableView).isDecelerating;
  }
  if (!res) {
    [self.eventHandler onGestureRecognizedByEventTarget:self.eventHandler.touchTarget];
  }
  return res;
}

@end

#pragma mark - LongPressGestureRecognizerDelegate
@interface LongPressGestureRecognizerDelegate : CustomGestureRecognizerDelegate

@property(nonatomic, readwrite) BOOL disableLongpressAfterScroll;

@end

@implementation LongPressGestureRecognizerDelegate

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler {
  self = [super initWithEventHandler:eventHandler];
  if (self) {
    self.disableLongpressAfterScroll = NO;
  }
  return self;
}

// Return NO, indicating that there is no otherGestureRecognizer that would cause gestureRecognizer
// to begin execution only after otherGestureRecognizer fails.
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  // If this flag is true, long press gesture will not be recognized after triggering scrolling.
  if (self.disableLongpressAfterScroll) {
    if ([otherGestureRecognizer isKindOfClass:UIPanGestureRecognizer.class] &&
        [otherGestureRecognizer.view isKindOfClass:UIScrollView.class]) {
      if (((UIScrollView*)otherGestureRecognizer.view).isDecelerating) {
        return YES;
      }
    }
  }
  return NO;
}

// Return YES to allow gestureRecognizer and otherGestureRecognizer to be triggered simultaneously.
- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  if (otherGestureRecognizer.state == UIGestureRecognizerStateBegan &&
      ![otherGestureRecognizer.view isDescendantOfView:self.eventHandler.rootView]) {
    self.eventHandler.gestureRecognized = YES;
  }
  return YES;
}

@end

#pragma mark - PanGestureRecognizerDelegate
@interface PanGestureRecognizerDelegate : CustomGestureRecognizerDelegate

@property(nonatomic, readonly) NSArray<LynxWeakProxy*>* gestures;

@end

@implementation PanGestureRecognizerDelegate {
  NSMutableDictionary<NSString*, LynxWeakProxy*>* _innerGestures;
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer*)gestureRecognizer {
  return YES;
}

- (instancetype)initWithEventHandler:(LynxEventHandler*)eventHandler {
  self = [super initWithEventHandler:eventHandler];
  if (self) {
    _innerGestures = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (NSArray<LynxWeakProxy*>*)gestures {
  return [_innerGestures allValues];
}

// Return YES to allow gestureRecognizer to begin execution only after otherGestureRecognizer fails.
- (BOOL)gestureRecognizer:(UIPanGestureRecognizer*)gestureRecognizer
    shouldRequireFailureOfGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  return NO;
}

// Return YES to allow otherGestureRecognizer to begin execution only after gestureRecognizer fails.
- (BOOL)gestureRecognizer:(UIPanGestureRecognizer*)gestureRecognizer
    shouldBeRequiredToFailByGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer {
  if (![gestureRecognizer.view isEqual:otherGestureRecognizer.view]) {
    [_innerGestures setValue:[LynxWeakProxy proxyWithTarget:otherGestureRecognizer]
                      forKey:[@(otherGestureRecognizer.hash) stringValue]];
    return YES;
  }
  return NO;
}

// Return YES to allow gestureRecognizer and otherGestureRecognizer to be recognized simultaneously.
- (BOOL)gestureRecognizer:(UIPanGestureRecognizer*)gestureRecognizer
    shouldRecognizeSimultaneouslyWithGestureRecognizer:
        (UIGestureRecognizer*)otherGestureRecognizer {
  if ([gestureRecognizer.view isEqual:otherGestureRecognizer.view]) {
    return YES;
  }
  if (gestureRecognizer.state == UIGestureRecognizerStatePossible ||
      gestureRecognizer.state == UIGestureRecognizerStateBegan) {
    return YES;
  }
  return NO;
}

@end

#pragma mark - LynxEventHandler
@implementation LynxEventHandler {
  __weak LynxUIOwner* _uiOwner;
  __weak LynxUI* _rootUI;
  __weak id<LynxEventTarget> _touchTarget;
  CGPoint _longPressPoint;
  CustomGestureRecognizerDelegate* _tapDelegate;
  LongPressGestureRecognizerDelegate* _longPressDelegate;
  UIPanGestureRecognizer* _panGestureRecognizer;
  PanGestureRecognizerDelegate* _panGestureDelegate;
  float range_;
  NSMutableSet* _set;
  NSMutableSet* _setOfPropsChanged;
}

- (void)dealloc {
  _touchRecognizer.target = nil;
  _touchRecognizer.preTarget = nil;
  _touchRecognizer.gestureArenaManager = nil;
}

- (instancetype)initWithRootView:(UIView*)rootView {
  return [self initWithRootView:rootView withRootUI:nil];
}

- (instancetype)initWithRootView:(UIView*)rootView withRootUI:(LynxUI*)rootUI {
  self = [super init];
  if (self) {
    _rootView = rootView;
    _rootUI = rootUI;
    _touchRecognizer = [[LynxTouchHandler alloc] initWithEventHandler:self];
    _tapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                             action:@selector(dispatchTapEvent:)];
    _longPressRecognizer =
        [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                      action:@selector(dispatchLongPressEvent:)];

    _tapDelegate = [[TapGestureRecognizerDelegate alloc] initWithEventHandler:self];
    _longPressDelegate = [[LongPressGestureRecognizerDelegate alloc] initWithEventHandler:self];

    _tapRecognizer.delegate = _tapDelegate;
    _tapRecognizer.cancelsTouchesInView = YES;
    _longPressRecognizer.delegate = _longPressDelegate;
    _longPressRecognizer.cancelsTouchesInView = YES;

    [_rootView addGestureRecognizer:_tapRecognizer];
    [_rootView addGestureRecognizer:_longPressRecognizer];
    [_rootView addGestureRecognizer:_touchRecognizer];

    [_touchRecognizer setupVelocityTracker:_rootView];

    // Defaul value is nil. If LynxUI has consume-slide-event prop, init _panGestureRecognizer and
    // _panGestureDelegate.
    _panGestureRecognizer = nil;
    _panGestureDelegate = nil;

    _longPressPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    range_ = 50;
    self.gestureRecognized = NO;
    _set = [NSMutableSet set];
    _setOfPropsChanged = [NSMutableSet set];
  }
  return self;
}

- (void)attachLynxView:(LynxView*)rootView {
  _rootView = rootView;
  [_rootView addGestureRecognizer:_tapRecognizer];
  [_rootView addGestureRecognizer:_longPressRecognizer];
  [_rootView addGestureRecognizer:_touchRecognizer];
  if (_panGestureRecognizer != nil) {
    [_rootView addGestureRecognizer:_panGestureRecognizer];
  }
}

- (LynxCustomEvent*)generateGestureEvent:(UIGestureRecognizer*)sender
                                withName:(NSString*)name
                                      ui:(id<LynxEventTarget>)ui {
  CGPoint pagePoint = [sender locationInView:_rootView];
  NSDictionary* detail = @{
    @"x" : [NSNumber numberWithFloat:pagePoint.x],
    @"y" : [NSNumber numberWithFloat:pagePoint.y]
  };
  LynxCustomEvent* gestureEventInfo = [[LynxDetailEvent alloc] initWithName:name
                                                                 targetSign:ui.signature
                                                                     detail:detail];
  return gestureEventInfo;
}

- (BOOL)touchUI:(id<LynxEventTarget>)ui isDescendantOfUI:(id<LynxEventTarget>)pre {
  if (_touchTarget.signature == pre.signature) {
    return YES;
  }
  BOOL res = NO;
  if (ui == nil || pre == nil) {
    return res;
  }
  id<LynxEventTarget> parent = ui;
  while (parent != nil && parent != parent.parentTarget) {
    if (parent == pre) {
      res = YES;
      break;
    }
    parent = parent.parentTarget;
  }
  return res;
}

- (void)dispatchTapEvent:(UITapGestureRecognizer*)sender {
  LLogInfo(@"Lynxview LynxEventHandler dispatchTapEvent %p: ", self.rootView);

  // For the tap event, it only support single finger.
  if ([_touchRecognizer isEnableAndGetMultiTouch]) {
    return;
  }
  CGPoint clientPoint = [sender locationInView:nil];
  CGPoint pagePoint = [sender locationInView:_rootView];
  NSInteger slideTargetSign = [self canRespondTapOrClickEvent:_touchTarget];
  NSInteger propsTargetSign = [self canRespondTapOrClickWhenUISlideByProps:_touchTarget];
  BOOL touchInSideLynx = [self touchUI:[self hitTestInner:pagePoint withEvent:nil]
                      isDescendantOfUI:_touchRecognizer.preTarget];
  if (slideTargetSign == -1 && propsTargetSign == -1 && touchInSideLynx) {
    CGPoint viewPoint = pagePoint;
    if ([self.touchRecognizer.preTarget isKindOfClass:[LynxUI class]]) {
      LynxUI* ui = (LynxUI*)self.touchRecognizer.preTarget;
      viewPoint = [sender locationInView:ui.view];
    }
    LynxTouchEvent* event =
        [[LynxTouchEvent alloc] initWithName:LynxEventTap
                                   targetTag:_touchRecognizer.preTarget.signature
                                 clientPoint:clientPoint
                                   pagePoint:pagePoint
                                   viewPoint:viewPoint];
    event.eventTarget = _touchRecognizer.preTarget;
    event.timestamp = [[NSDate date] timeIntervalSince1970];
    if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
      [_touchRecognizer
          showMessageOnConsole:[NSString
                                   stringWithFormat:@"LynxEventHandler: fire tap for target %ld",
                                                    _touchRecognizer.preTarget.signature]
                     withLevel:LynxLogBoxLevelInfo];
    }
    [_eventEmitter dispatchTouchEvent:event];
  } else {
    if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
      [_touchRecognizer
          showMessageOnConsole:[NSString stringWithFormat:@"LynxEventHandler: tap failed due to "
                                                          @"[outside] %d [slide] %ld [props] %ld",
                                                          !touchInSideLynx, slideTargetSign,
                                                          propsTargetSign]
                     withLevel:LynxLogBoxLevelWarning];
    }
  }
}

- (void)dispatchLongPressEvent:(UILongPressGestureRecognizer*)sender {
  LLogInfo(@"Lynxview LynxEventHandler dispatchLongPressEvent %p: ", self.rootView);

  // For the longpress event, it only support single finger.
  if (_touchTarget == nil || [_touchRecognizer isEnableAndGetMultiTouch]) {
    return;
  }
  CGPoint clientPoint = [sender locationInView:nil];
  CGPoint pagePoint = [sender locationInView:_rootView];
  CGPoint viewPoint = pagePoint;
  if ([self.touchRecognizer.preTarget isKindOfClass:[LynxUI class]]) {
    LynxUI* ui = (LynxUI*)self.touchRecognizer.preTarget;
    viewPoint = [sender locationInView:ui.view];
  }

  if (sender.state == UIGestureRecognizerStateBegan) {
    LynxTouchEvent* event = [[LynxTouchEvent alloc] initWithName:LynxEventLongPress
                                                       targetTag:_touchTarget.signature
                                                     clientPoint:clientPoint
                                                       pagePoint:pagePoint
                                                       viewPoint:viewPoint];
    event.eventTarget = _touchTarget;
    event.timestamp = [[NSDate date] timeIntervalSince1970];
    if (![_eventEmitter dispatchTouchEvent:event]) {
      _longPressPoint = pagePoint;
      self.gestureRecognized = NO;
      [self resetEventEnv];
    }
    if ([_rootView isKindOfClass:[LynxView class]]) {
      [((LynxView*)_rootView) onLongPress];
    }
    _touchRecognizer.tapSlop = range_;
  } else if (sender.state == UIGestureRecognizerStateChanged) {
    if (_longPressPoint.x != -FLT_MAX && _longPressPoint.y != -FLT_MAX) {
      if (fabs(pagePoint.x - _longPressPoint.x) > range_ ||
          fabs(pagePoint.y - _longPressPoint.y) > range_) {
        _longPressPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
      }
    }
  } else if (sender.state == UIGestureRecognizerStateEnded) {
    NSInteger slideTargetSign = [self canRespondTapOrClickEvent:_touchTarget];
    NSInteger propsTargetSign = [self canRespondTapOrClickWhenUISlideByProps:_touchTarget];
    BOOL touchInThreshold = _longPressPoint.x != -FLT_MAX && _longPressPoint.y != -FLT_MAX;
    BOOL touchInSideLynx = [self touchUI:[self hitTestInner:pagePoint withEvent:nil]
                        isDescendantOfUI:_touchRecognizer.preTarget];
    if (touchInThreshold && slideTargetSign == -1 && propsTargetSign == -1 && touchInSideLynx) {
      LynxTouchEvent* event =
          [[LynxTouchEvent alloc] initWithName:LynxEventTap
                                     targetTag:_touchRecognizer.preTarget.signature
                                   clientPoint:clientPoint
                                     pagePoint:pagePoint
                                     viewPoint:viewPoint];
      event.eventTarget = _touchRecognizer.preTarget;
      event.timestamp = [[NSDate date] timeIntervalSince1970];
      if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
        [_touchRecognizer
            showMessageOnConsole:[NSString
                                     stringWithFormat:@"LynxEventHandler: fire tap for target %ld",
                                                      _touchRecognizer.preTarget.signature]
                       withLevel:LynxLogBoxLevelInfo];
      }
      [_eventEmitter dispatchTouchEvent:event];
      _longPressPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    } else {
      if ([LynxEnv.sharedInstance highlightTouchEnabled]) {
        [_touchRecognizer
            showMessageOnConsole:[NSString stringWithFormat:@"LynxEventHandler: tap failed due to "
                                                            @"[move] %d [slide] %ld [props] %ld",
                                                            !touchInThreshold, slideTargetSign,
                                                            propsTargetSign]
                       withLevel:LynxLogBoxLevelWarning];
      }
    }
  } else if (sender.state == UIGestureRecognizerStateCancelled ||
             sender.state == UIGestureRecognizerStateFailed) {
    _longPressPoint = CGPointMake(-FLT_MAX, -FLT_MAX);
    self.gestureRecognized = NO;
    [self resetEventEnv];
  }
}

// Only when a LynxUI has "consume-slide-event" property, needCheckConsumeSlideEvent could be
// executed. Otherwise, needCheckConsumeSlideEvent will not be executed, which causes the
// UIPanGestureRecognizer will not be added to the LynxView, consistent with the previous behavior,
// to avoid breaking changes.
- (void)needCheckConsumeSlideEvent {
  if (_panGestureRecognizer && _panGestureDelegate) {
    return;
  }

  // Init gesture
  _panGestureRecognizer =
      [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(dispatchPanEvent:)];
  // Init delegate
  _panGestureDelegate = [[PanGestureRecognizerDelegate alloc] initWithEventHandler:self];
  _panGestureRecognizer.delegate = _panGestureDelegate;
  _panGestureRecognizer.cancelsTouchesInView = self.tapRecognizer.cancelsTouchesInView;
  [_rootView addGestureRecognizer:_panGestureRecognizer];
}

- (BOOL)consumeSlideEvents:(CGFloat)angle {
  id<LynxEventTarget> target = _touchTarget;
  while (target != nil && target != target.parentTarget) {
    if ([target consumeSlideEvent:angle]) {
      return YES;
    }
    target = target.parentTarget;
  }
  return NO;
}

- (void)dispatchPanEvent:(UIPanGestureRecognizer*)sender {
  // Calculate the distance and angle of finger movement.
  CGFloat distanceX = [sender translationInView:sender.view].x;
  CGFloat distanceY = [sender translationInView:sender.view].y;

  // To avoid the dead zone in angle calculation during the initial stage of finger movement, when
  // distanceX and distanceY are small, return true first to make LynxView consume the event. When
  // the finger moves beyond a certain threshold, calculate the angle to determine whether to
  // consume the slide event.
  CGFloat threshold = 10;
  if (fabs(distanceX) <= threshold && fabs(distanceY) <= threshold) {
    return;
  }

  // Use atan2(y, x) * 180 / PI to calculate the angle.
  CGFloat semicircleAngle = 180;
  CGFloat angle = atan2(distanceY, distanceX) * semicircleAngle / M_PI;
  if ([self consumeSlideEvents:angle]) {
    [((PanGestureRecognizerDelegate*)sender.delegate).gestures
        enumerateObjectsUsingBlock:^(LynxWeakProxy* _Nonnull obj, NSUInteger idx,
                                     BOOL* _Nonnull stop) {
          ((UIGestureRecognizer*)obj).state = UIGestureRecognizerStateFailed;
        }];
    return;
  }
  sender.state = UIGestureRecognizerStateFailed;
}

- (void)updateUiOwner:(LynxUIOwner*)owner eventEmitter:(LynxEventEmitter*)eventEmitter {
  _uiOwner = owner;
  _eventEmitter = eventEmitter;
}

- (NSInteger)setGestureArenaManagerAndGetIndex:(nullable LynxGestureArenaManager*)manager {
  _gestureArenaManager = manager;
  NSInteger i = [_touchRecognizer setGestureArenaManagerAndGetIndex:manager];
  return i;
}

- (void)removeGestureArenaManager:(NSInteger)index {
  _gestureArenaManager = nil;
  [_touchRecognizer removeGestureArenaManager:index];
}

// should be called when touch target has been found, will not change _touchTarget
- (id<LynxEventTarget>)hitTestInner:(CGPoint)point withEvent:(UIEvent*)event {
  if (_rootUI == nil) {
    _rootUI = (LynxUI*)_uiOwner.rootUI;
  }
  return [_rootUI hitTest:point withEvent:event];
}

// should be called when looking for the target for the first time, touchTarget will be recorded
- (id<LynxEventTarget>)hitTest:(CGPoint)point withEvent:(UIEvent*)event {
  if (_rootUI == nil) {
    _rootUI = (LynxUI*)_uiOwner.rootUI;
  }
  _touchTarget = [self hitTestInner:point withEvent:event];
  return _touchTarget;
}

// TODO(hexionghui): Delete this, use onGestureRecognizedByEventTarget instead.
- (void)onGestureRecognized {
  self.gestureRecognized = YES;
  if (_touchRecognizer != nil) {
    [_touchRecognizer onGestureRecognized];
  }
}

- (void)onGestureRecognizedByEventTarget:(id<LynxEventTarget>)ui {
  if (_set != nil) {
    [_set addObject:@(ui.signature)];
  }
}

- (void)onPropsChangedByEventTarget:(id<LynxEventTarget>)ui {
  if (_setOfPropsChanged != nil) {
    [_setOfPropsChanged addObject:@(ui.signature)];
  }
}

- (void)resetEventEnv {
  [_set removeAllObjects];
  [_setOfPropsChanged removeAllObjects];
}

- (NSInteger)checkCanRespondTapOrClick:(id<LynxEventTarget>)ui withSet:(NSSet*)set {
  if (ui == nil) {
    return 0;
  }
  if (set == nil || [set count] == 0) {
    return -1;
  }
  NSInteger res = -1;
  id<LynxEventTarget> parent = ui;
  while (parent != nil && parent.parentTarget != parent) {
    if ([set containsObject:@(parent.signature)]) {
      res = parent.signature;
      break;
    }
    parent = parent.parentTarget;
  }
  return res;
}

// Specify whether the tap/click event can be triggered when the ui slides.
// At this time, the ui slide is triggered by a gesture.
- (NSInteger)canRespondTapOrClickEvent:(id<LynxEventTarget>)ui {
  return [self checkCanRespondTapOrClick:ui withSet:_set];
}

// Specify whether the tap/click event can be triggered when the ui slides.
// At this time, the ui slide is triggered by the front-end modifying the
// animation or layout attributes.
- (NSInteger)canRespondTapOrClickWhenUISlideByProps:(id<LynxEventTarget>)ui {
  return [self checkCanRespondTapOrClick:ui withSet:_setOfPropsChanged];
}

- (id<LynxEventTarget>)touchTarget {
  return _touchTarget;
}

- (void)setEnableViewReceiveTouch:(BOOL)enable {
  if (enable) {
    _tapRecognizer.cancelsTouchesInView = NO;
    _longPressRecognizer.cancelsTouchesInView = NO;
    _panGestureRecognizer.cancelsTouchesInView = NO;
    if (_panGestureRecognizer != nil) {
      _panGestureRecognizer.cancelsTouchesInView = NO;
    }
  }
}

- (void)setDisableLongpressAfterScroll:(bool)value {
  _longPressDelegate.disableLongpressAfterScroll = value;
}

// issue: #7022, In some scenarios, users want the LynxTap gesture and the outer tap gesture to be
// triggered at the same time. So add the enableSimultaneousTap configuration. When
// enableSimultaneousTap is set, LynxTap gestures and outer tap gestures can be triggered at the
// same time.
- (void)setEnableSimultaneousTap:(BOOL)enable {
  _enableSimultaneousTap = enable;
}

- (void)setTapSlop:(NSString*)tapSlop {
  range_ = [LynxUnitUtils toPtFromUnitValue:tapSlop
                               rootFontSize:0
                                curFontSize:0
                                  rootWidth:0
                                 rootHeight:0
                              withDefaultPt:0];
}

- (void)setLongPressDuration:(int32_t)value {
  // If long press duration < 0, do not set this negative value.
  if (value < 0) {
    return;
  }
  NSTimeInterval duration = value / 1000.0;
  ((UILongPressGestureRecognizer*)_longPressRecognizer).minimumPressDuration = duration;
}

- (LynxUIOwner*)uiOwner {
  return _uiOwner;
}

@end
