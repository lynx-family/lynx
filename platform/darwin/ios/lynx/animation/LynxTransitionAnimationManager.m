// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxAnimationDelegate.h>
#import <Lynx/LynxAnimationTransformRotation.h>
#import <Lynx/LynxAnimationUtils.h>
#import <Lynx/LynxConverter+LynxCSSType.h>
#import <Lynx/LynxConverter+Transform.h>
#import <Lynx/LynxConverter.h>
#import <Lynx/LynxGlobalObserver.h>
#import <Lynx/LynxTransformRaw.h>
#import <Lynx/LynxTransitionAnimationManager+Private.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIUnitUtils.h>

static const NSString* const kTransitionEventStart = @"transitionstart";
static const NSString* const kTransitionEventEnd = @"transitionend";

enum {
  kOpacityTransitionSlot,
  kTransformTransitionSlot,
  kRoutedTransitionCount,
};

static NSUInteger RoutedTransitionSlot(LynxAnimationProp property) {
  return property == TRANSITION_TRANSFORM ? kTransformTransitionSlot : kOpacityTransitionSlot;
}

typedef NS_ENUM(uint8_t, LynxRoutedTransitionState) {
  LynxRoutedTransitionRunning,
  LynxRoutedTransitionHolding,
  LynxRoutedTransitionStopped,
};
static NSString* const kTransitionInstallRevision = @"lynxTransitionInstallRevision";

typedef struct {
  CGFloat startRotation[3];
  CGFloat targetRotation[3];
  CFTimeInterval beginTime;
  CGSize size;
  uint32_t installRevision;
  BOOL sizeDependent;
} LynxTransitionTransformState;

@interface LynxRoutedTransition : NSObject <CAAnimationDelegate> {
 @public
  __weak LynxTransitionAnimationManager* _manager;
  uint64_t _animationID;
  uint32_t _generation;
  LynxAnimationProp _property;
  LynxRoutedTransitionState _state;
  id _startValue;
  id _targetValue;
  LynxAnimationInfo* _animationInfo;
  LynxTransitionTransformState _transform;

 @private
  LynxGlobalObserver* _observer;
  BOOL _didStart;
}
- (void)endObservation __attribute__((objc_direct));
@end

static NSValue* RoutedPresentationValue(LynxUI* ui, LynxRoutedTransition* transition) {
  if (!transition || transition->_state != LynxRoutedTransitionRunning) {
    return nil;
  }
  CALayer* layer = transition->_property == OPACITY ? ui.backgroundManager.opacityView.layer : nil;
  CALayer* presentation = (layer ?: ui.view.layer).presentationLayer;
  if (!presentation) {
    return nil;
  }
  return transition->_property == OPACITY ? @(presentation.opacity)
                                          : [NSValue valueWithCATransform3D:presentation.transform];
}

static void AttachPlatformTransition(LynxUI* ui, CAAnimation* animation, LynxAnimationProp property,
                                     NSString* name) {
  if (property == OPACITY && ui.backgroundManager.opacityView) {
    [LynxAnimationUtils attachOpacityTo:ui animation:animation forKey:name];
  } else {
    [LynxAnimationUtils attachTo:ui animation:animation forKey:name];
  }
}

static double CubicCoordinate(double t, double a, double b) {
  const double s = 1 - t;
  return t * (3 * s * (s * a + t * b) + t * t);
}

static double TransformProgress(LynxUI* ui, LynxRoutedTransition* transition) {
  const CFTimeInterval elapsed = [ui.view.layer convertTime:CACurrentMediaTime() fromLayer:nil] -
                                 transition->_transform.beginTime;
  const CFTimeInterval duration = transition->_animationInfo.duration;
  if (elapsed <= 0 || elapsed >= duration) {
    return elapsed <= 0 ? 0 : 1;
  }
  const double progress = elapsed / duration;
  CAMediaTimingFunction* timing = transition->_animationInfo.timingFunction;
  if (!timing) {
    return progress;
  }
  float a[2], b[2];
  [timing getControlPointAtIndex:1 values:a];
  [timing getControlPointAtIndex:2 values:b];
  double low = 0, high = 1;
  for (NSUInteger i = 0; i < 20; ++i) {
    const double t = (low + high) / 2;
    if (CubicCoordinate(t, a[0], b[0]) < progress) {
      low = t;
    } else {
      high = t;
    }
  }
  return CubicCoordinate((low + high) / 2, a[1], b[1]);
}

static CGFloat RotationAtProgress(LynxRoutedTransition* transition, NSUInteger axis,
                                  double progress) {
  const CGFloat from = transition->_transform.startRotation[axis];
  return from + (transition->_transform.targetRotation[axis] - from) * progress;
}

typedef struct {
  CATransform3D withoutRotate;
  CATransform3D withoutRotateXY;
  CGFloat rotation[3];
} LynxTransitionTransform;

static NSString* const kRotationPaths[] = {@"transform.rotation.x", @"transform.rotation.y",
                                           @"transform.rotation.z"};

static void ConvertPlatformTransformEndpoint(NSArray<LynxTransformRaw*>* raw, LynxUI* ui,
                                             LynxTransitionTransform* endpoint) {
  char rotationType = LynxTransformRotationNone;
  [LynxConverter toCATransform3D:raw
                              ui:ui
                        newFrame:ui.updatedFrame
          transformWithoutRotate:&endpoint->withoutRotate
        transformWithoutRotateXY:&endpoint->withoutRotateXY
                    rotationType:&rotationType
                       rotationX:&endpoint->rotation[0]
                       rotationY:&endpoint->rotation[1]
                       rotationZ:&endpoint->rotation[2]];
}

static void DecomposePresentationTransform(CATransform3D transform,
                                           LynxTransitionTransform* endpoint) {
  CALayer* snapshot = [CALayer layer];
  snapshot.transform = transform;
  for (NSUInteger axis = 0; axis < 3; ++axis) {
    endpoint->rotation[axis] = [[snapshot valueForKeyPath:kRotationPaths[axis]] doubleValue];
  }
  [snapshot setValue:@0 forKeyPath:kRotationPaths[0]];
  [snapshot setValue:@0 forKeyPath:kRotationPaths[1]];
  endpoint->withoutRotateXY = snapshot.transform;
  [snapshot setValue:@0 forKeyPath:kRotationPaths[2]];
  endpoint->withoutRotate = snapshot.transform;
}

static NSArray<CAAnimation*>* CreateTransformAnimations(const LynxTransitionTransform* from,
                                                        const LynxTransitionTransform* to,
                                                        LynxAnimationInfo* config) {
  const BOOL rotateZInMatrix = fabs(to->rotation[2] - from->rotation[2]) < M_PI;
  const CATransform3D* fromMatrix = rotateZInMatrix ? &from->withoutRotateXY : &from->withoutRotate;
  const CATransform3D* toMatrix = rotateZInMatrix ? &to->withoutRotateXY : &to->withoutRotate;
  NSMutableArray* animations = [NSMutableArray array];
  [animations addObject:[LynxAnimationUtils
                            createBasicAnimation:@"transform"
                                            from:[NSValue valueWithCATransform3D:*fromMatrix]
                                              to:[NSValue valueWithCATransform3D:*toMatrix]
                                            info:config]];
  const NSUInteger axes = rotateZInMatrix ? 2 : 3;
  for (NSUInteger axis = 0; axis < axes; ++axis) {
    if (from->rotation[axis] != to->rotation[axis]) {
      [animations addObject:[LynxAnimationUtils
                                createBasicAnimation:kRotationPaths[axis]
                                                from:[NSNumber numberWithFloat:from->rotation[axis]]
                                                  to:[NSNumber numberWithFloat:to->rotation[axis]]
                                                info:config]];
    }
  }
  return animations;
}

static NSArray<CAAnimation*>* CreateRoutedTransformAnimations(LynxUI* ui,
                                                              LynxRoutedTransition* transition,
                                                              LynxRoutedTransition* previous,
                                                              double progress) {
  LynxTransitionTransform start = {0}, end = {0};
  if ([transition->_startValue isKindOfClass:[NSValue class]]) {
    DecomposePresentationTransform([transition->_startValue CATransform3DValue], &start);
    // The matrix gives the displayed orientation. Timing identifies its turn.
    for (NSUInteger axis = 0; axis < 3; ++axis) {
      const CGFloat angle = RotationAtProgress(previous, axis, progress);
      start.rotation[axis] += 2 * M_PI * round((angle - start.rotation[axis]) / (2 * M_PI));
    }
  } else {
    ConvertPlatformTransformEndpoint(transition->_startValue, ui, &start);
  }
  ConvertPlatformTransformEndpoint(transition->_targetValue, ui, &end);
  memcpy(transition->_transform.startRotation, start.rotation, sizeof(start.rotation));
  memcpy(transition->_transform.targetRotation, end.rotation, sizeof(end.rotation));
  transition->_transform.size = ui.updatedFrame.size;
  return CreateTransformAnimations(&start, &end, transition->_animationInfo);
}

@interface LynxTransitionAnimationManager () {
  LynxRoutedTransition* _routedTransitions[kRoutedTransitionCount];
  NSMutableDictionary<NSNumber*, CAAnimation*>* _transitionAnimations;
  NSMutableDictionary<NSNumber*, LynxAnimationInfo*>* _transitionInfos;
  NSMutableDictionary<NSNumber*, LynxAnimationDelegate*>* _transitionDelegates;
}
@property(nonatomic, weak, readonly) LynxUI* ui;
@property(nonatomic) CGRect latestFrame;

- (BOOL)removePlatformTransition:(LynxRoutedTransition*)transition __attribute__((objc_direct));
- (void)transitionsDidAssemble;
- (BOOL)isTransitionSize:(CGRect)oldFrame newFrame:(CGRect)newFrame;
- (BOOL)isTransitionPosition:(CGRect)oldFrame newFrame:(CGRect)newFrame;
- (BOOL)hasTransition:(LynxAnimationProp)prop;
- (void)enqueueTransitionAnimation:(CAAnimation*)animation withConfig:(LynxAnimationInfo*)config;
- (LynxAnimationDelegate*)createTransitionDelegate:(LynxAnimationInfo*)info
                                         animation:(CAAnimation*)transition;
- (void)setupTransitionAnimationForContents:(CABasicAnimation*)content
                                 withConfig:(LynxAnimationInfo*)config
                                   forLayer:(CALayer*)layer
                                   withProp:(LynxAnimationProp)prop;
- (void)autoAddAnimationForComplexBackgroundWithUI:(LynxUI*)ui
                                             frame:(CGRect)newFrame
                                            config:(LynxAnimationInfo*)config
                                          withProp:(LynxAnimationProp)prop;
@end

@implementation LynxRoutedTransition

- (void)animationDidStart:(CAAnimation*)animation {
  if (_state == LynxRoutedTransitionRunning && !_didStart &&
      [[animation valueForKey:kTransitionInstallRevision] unsignedIntValue] ==
          _transform.installRevision) {
    _didStart = YES;
    _observer = _manager.ui.context.observer;
    [_observer notifyAnimationStart];
  }
}

- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)finished {
  if (_state == LynxRoutedTransitionRunning &&
      [[animation valueForKey:kTransitionInstallRevision] unsignedIntValue] ==
          _transform.installRevision) {
    if (!finished) {
      _targetValue = nil;
    }
    if (![_manager removePlatformTransition:self]) {
      [self endObservation];
    }
  }
}

- (void)endObservation {
  LynxGlobalObserver* observer = _observer;
  _observer = nil;
  [observer notifyAnimationEnd];
}

@end

@implementation LynxTransitionAnimationManager

- (instancetype)initWithLynxUI:(LynxUI*)ui {
  if (self = [super init]) {
    _ui = ui;
  }
  return self;
}

- (BOOL)isShouldTransitionType:(LynxAnimationProp)type {
  if (type == OPACITY || type == TRANSITION_WIDTH || type == TRANSITION_HEIGHT ||
      type == TRANSITION_BACKGROUND_COLOR || type == TRANSITION_VISIBILITY ||
      type == TRANSITION_LEFT || type == TRANSITION_RIGHT || type == TRANSITION_TOP ||
      type == TRANSITION_BOTTOM || type == TRANSITION_TRANSFORM || type == TRANSITION_ALL ||
      type == TRANSITION_LEGACY_ALL_1 || type == TRANSITION_LEGACY_ALL_2 ||
      type == TRANSITION_LEGACY_ALL_3) {
    return true;
  }
  return false;
}

- (void)assembleTransitions:(NSArray*)params {
  if (nil == params || params.count == 0) {
    [self transitionsDidAssemble];
    return;
  }
  _transitionAnimations = [[NSMutableDictionary alloc] init];
  if (!_transitionDelegates) {
    _transitionDelegates = [[NSMutableDictionary alloc] init];
  }
  NSDictionary* oldTransitionInfos = _transitionInfos;
  _transitionInfos = [[NSMutableDictionary alloc] init];
  int index = 0;
  for (id a in params) {
    LynxAnimationInfo* info = [LynxConverter toTransitionAnimationInfo:a];
    if (info) {
      if (![self isShouldTransitionType:info.prop]) {
        continue;
      }
      if (info.prop == TRANSITION_ALL || info.prop == TRANSITION_LEGACY_ALL_1 ||
          info.prop == TRANSITION_LEGACY_ALL_2 || info.prop == TRANSITION_LEGACY_ALL_3) {
        index = 0;
        info.prop = OPACITY;
        info.name = [LynxConverter toLynxPropName:info.prop];
        info.orderIndex = index++;
        _transitionInfos[[NSNumber numberWithUnsignedInteger:info.prop]] = info;
        for (int i = 4; i <= 12; ++i) {
          LynxAnimationInfo* infoCopy = [LynxAnimationInfo copyAnimationInfo:info
                                                                    withProp:(1 << i)];
          infoCopy.orderIndex = index++;
          _transitionInfos[[NSNumber numberWithUnsignedInteger:infoCopy.prop]] = infoCopy;
        }
        break;
      }
      info.orderIndex = index++;
      _transitionInfos[[NSNumber numberWithUnsignedInteger:info.prop]] = info;
    }
  }

  // When both TRANSITION_LEFT and TRANSITION_RIGHT exist, keep the one that was added to
  // _transitionInfos later.
  [LynxAnimationInfo removeDuplicateAnimation:_transitionInfos
                                      withKey:TRANSITION_LEFT
                                    sameToKey:TRANSITION_RIGHT];

  // When both TRANSITION_TOP and TRANSITION_BOTTOM exist, keep the one that was added to
  // _transitionInfos later.
  [LynxAnimationInfo removeDuplicateAnimation:_transitionInfos
                                      withKey:TRANSITION_TOP
                                    sameToKey:TRANSITION_BOTTOM];

  // cancel animations that were removed from css.
  if (oldTransitionInfos != nil) {
    NSArray* allKeys = [oldTransitionInfos allKeys];
    for (NSNumber* prop in allKeys) {
      if ([_transitionInfos objectForKey:prop] == nil) {
        [self removeTransitionAnimation:[prop intValue]];
      }
    }
  }
}

- (BOOL)maybeUpdateBackgroundWithTransitionAnimation:(UIColor*)color {
  if (![self isTransitionBackgroundColor]) {
    return NO;
  }
  const BOOL isSimple = ((_ui.backgroundManager.animationOptions & LynxAnimOptHasBGComplex) == 0);
  CALayer* targetLayer = _ui.view.layer;
  if (isSimple && _ui.backgroundManager.backgroundLayer) {
    targetLayer = _ui.backgroundManager.backgroundLayer;
  }

  if (targetLayer &&
      [UIColor colorWithCGColor:targetLayer.presentationLayer.backgroundColor] == color) {
    [self removeTransitionAnimation:TRANSITION_BACKGROUND_COLOR];
    return NO;
  }
  __weak LynxUI* weakUI = _ui;
  [self performTransitionAnimationsWithBackground:color
                                         callback:^(BOOL finished) {
                                           __strong LynxUI* strongUI = weakUI;
                                           if (strongUI) {
                                             strongUI.backgroundManager.backgroundColor = color;
                                             [strongUI.view setNeedsDisplay];
                                           }
                                         }];
  return YES;
}

- (BOOL)maybeUpdateOpacityWithTransitionAnimation:(CGFloat)opacity {
  if (![self hasTransition:OPACITY]) {
    return NO;
  }

  CALayer* targetLayer;
  if (_ui.backgroundManager.opacityView) {
    targetLayer = _ui.backgroundManager.opacityView.layer.presentationLayer;
  } else {
    targetLayer = _ui.view.layer.presentationLayer;
  }

  if (targetLayer && targetLayer.opacity == opacity) {
    [self removeTransitionAnimation:OPACITY];
    return NO;
  }
  __weak LynxUI* weakUI = _ui;
  [self performTransitionAnimationsWithOpacity:opacity
                                      callback:^(BOOL finished) {
                                        __strong LynxUI* strongUI = weakUI;
                                        if (strongUI) {
                                          [strongUI view].layer.opacity = opacity;
                                          strongUI.backgroundManager.opacity = opacity;
                                          [strongUI.view setNeedsDisplay];
                                        }
                                      }];
  return YES;
}

// Style updates own the layer's model value.
- (void)applyPlatformTransition:(LynxAnimationInfo*)info
                           from:(id)from
                             to:(id)to
                    animationID:(uint64_t)animationID
                     generation:(uint32_t)generation {
  const LynxAnimationProp property = info.prop;
  const NSUInteger slot = RoutedTransitionSlot(property);
  LynxRoutedTransition* previous = _routedTransitions[slot];
  if (previous && (animationID < previous->_animationID || (animationID == previous->_animationID &&
                                                            generation <= previous->_generation))) {
    return;
  }
  LynxUI* ui = _ui;
  NSValue* displayed = RoutedPresentationValue(ui, previous);
  if (displayed) {
    from = displayed;
  } else if (previous && previous->_state != LynxRoutedTransitionStopped) {
    from = previous->_startValue;
  }

  LynxRoutedTransition* transition = [LynxRoutedTransition new];
  transition->_manager = self;
  transition->_animationID = animationID;
  transition->_generation = generation;
  transition->_property = property;
  transition->_startValue = from;
  transition->_targetValue = to;
  transition->_animationInfo = info;

  CAAnimation* animation;
  if (property == OPACITY) {
    animation = [LynxAnimationUtils createBasicAnimation:@"opacity" from:from to:to info:info];
  } else {
    transition->_transform.sizeDependent =
        [LynxTransformRaw hasPercent:to] ||
        ([from isKindOfClass:[NSArray class]] && [LynxTransformRaw hasPercent:from]);
    CAAnimationGroup* group = [CAAnimationGroup animation];
    group.animations = CreateRoutedTransformAnimations(
        ui, transition, previous, displayed ? TransformProgress(ui, previous) : 0);
    animation = group;
  }
  _routedTransitions[slot] = transition;
  if (previous) {
    previous->_state = LynxRoutedTransitionStopped;
    previous->_startValue = nil;
  }
  NSString* name = [LynxConverter toLynxPropName:property];
  [LynxAnimationUtils removeAnimation:ui withName:name];
  [previous endObservation];
  if (_routedTransitions[slot] != transition || transition->_state != LynxRoutedTransitionRunning) {
    return;
  }
  animation.delegate = transition;
  [LynxAnimationUtils applyAnimationProperties:animation withInfo:info forLayer:ui.view.layer];
  transition->_transform.beginTime = animation.beginTime;
  AttachPlatformTransition(ui, animation, property, name);
}

- (NSValue*)stopPlatformTransition:(uint64_t)animationID
                        generation:(uint32_t)generation
                  holdPresentation:(BOOL)holdPresentation {
  LynxRoutedTransition* transition = nil;
  for (NSUInteger slot = 0; slot < kRoutedTransitionCount; ++slot) {
    LynxRoutedTransition* candidate = _routedTransitions[slot];
    if (candidate && candidate->_animationID == animationID) {
      transition = candidate;
      break;
    }
  }
  if (!transition || (holdPresentation ? generation != transition->_generation
                                       : generation < transition->_generation)) {
    return nil;
  }
  if (!holdPresentation) {
    transition->_generation = generation;
    transition->_targetValue = nil;
    [self removePlatformTransition:transition];
    return nil;
  }
  if (transition->_state == LynxRoutedTransitionHolding) {
    return transition->_startValue;
  }

  LynxUI* ui = _ui;
  const BOOL stopped = transition->_state == LynxRoutedTransitionStopped;
  NSValue* displayed = RoutedPresentationValue(ui, transition);
  id value = displayed ?: (stopped ? transition->_targetValue : transition->_startValue);
  if (!value) {
    return nil;
  }
  const LynxAnimationProp property = transition->_property;
  if (property == TRANSITION_TRANSFORM) {
    if ([value isKindOfClass:[NSArray class]]) {
      value = [NSValue valueWithCATransform3D:[LynxConverter toCATransform3D:value ui:ui]];
    }
    const double progress = stopped ? 1 : (displayed ? TransformProgress(ui, transition) : 0);
    for (NSUInteger axis = 0; axis < 3; ++axis) {
      transition->_transform.startRotation[axis] = RotationAtProgress(transition, axis, progress);
    }
  }

  // Keep the displayed value until Core commits its first sample.
  NSString* name = [LynxConverter toLynxPropName:property];
  CABasicAnimation* hold =
      [LynxAnimationUtils createBasicAnimation:property == OPACITY ? @"opacity" : @"transform"
                                          from:value
                                            to:value
                                          info:transition->_animationInfo];
  hold.duration = 1;
  hold.speed = 0;
  hold.fillMode = kCAFillModeBoth;
  hold.removedOnCompletion = NO;
  transition->_state = LynxRoutedTransitionHolding;
  transition->_startValue = value;
  [LynxAnimationUtils removeAnimation:ui withName:name];
  if (_routedTransitions[RoutedTransitionSlot(property)] == transition &&
      transition->_state == LynxRoutedTransitionHolding) {
    AttachPlatformTransition(ui, hold, property, name);
  }
  [transition endObservation];
  return value;
}

- (BOOL)removePlatformTransition:(LynxRoutedTransition*)transition {
  if (!transition || transition->_state == LynxRoutedTransitionStopped ||
      _routedTransitions[RoutedTransitionSlot(transition->_property)] != transition) {
    return NO;
  }
  transition->_state = LynxRoutedTransitionStopped;
  transition->_startValue = nil;
  [LynxAnimationUtils removeAnimation:_ui
                             withName:[LynxConverter toLynxPropName:transition->_property]];
  [transition endObservation];
  return YES;
}

- (void)removeLegacyTransformTransition {
  LynxRoutedTransition* transition = _routedTransitions[kTransformTransitionSlot];
  if (!transition || transition->_state == LynxRoutedTransitionStopped) {
    [self removeTransitionAnimation:TRANSITION_TRANSFORM];
  }
}

- (BOOL)maybeUpdateFrameWithTransitionAnimation:(CGRect)newFrame
                                    withPadding:(UIEdgeInsets)padding
                                         border:(UIEdgeInsets)border
                                         margin:(UIEdgeInsets)margin {
  // We end all layout animators here. All necessary layout animators will be added again later.
  [self removeAllLayoutTransitionAnimation];
  //                           width
  // (xxXLeft,xxYTop)|_______________________|
  //                 |                       |
  //                 |   (centerX,centerY)   |  height
  //                 |                       |
  //                 |-----------------------|
  //                 |                       |(xxXRight,xxYBottom)
  //

  CGRect oldBounds = _ui.view.layer.presentationLayer.bounds;
  CGPoint oldPosition = _ui.view.layer.presentationLayer.position;
  // Before the first draw of UIView, the frame of the CALayer is not synchronized to the
  // presentation layer, at this time the frame of presentation layer is nil. At this time we
  // should try to get the right frame from CALayer.
  if (_ui.view.layer.presentationLayer == nil) {
    oldBounds = _ui.view.layer.bounds;
    oldPosition = _ui.view.layer.position;
  }
  // The newFrame does not involve transform, but the frame from CALayer involve transform. So we
  // use bounds and position to make a frame without transform.
  CGFloat newPositionX = oldPosition.x - _ui.view.layer.anchorPoint.x * oldBounds.size.width;
  CGFloat newPositionY = oldPosition.y - _ui.view.layer.anchorPoint.y * oldBounds.size.height;
  CGRect oldFrame =
      CGRectMake(newPositionX, newPositionY, oldBounds.size.width, oldBounds.size.height);

  // Old frame from presentationLayer must be rounded to physical pixel first before it can be
  // compared with new frame.
  [LynxUIUnitUtils roundRectToPhysicalPixelGrid:&oldFrame];

  bool xPosChanged = oldFrame.origin.x != newFrame.origin.x;
  bool yPosChanged = oldFrame.origin.y != newFrame.origin.y;
  bool widthChanged = oldFrame.size.width != newFrame.size.width;
  bool heightChanged = oldFrame.size.height != newFrame.size.height;
  bool xRightNotChanged =
      (oldFrame.origin.x + oldFrame.size.width) == (newFrame.origin.x + newFrame.size.width);
  bool yBottomNotChange =
      (oldFrame.origin.y + oldFrame.size.height) == (newFrame.origin.y + newFrame.size.height);

  bool hasLeftAni = [self hasTransition:TRANSITION_LEFT] || [self hasTransition:TRANSITION_RIGHT];
  bool hasTopAni = [self hasTransition:TRANSITION_TOP] || [self hasTransition:TRANSITION_BOTTOM];
  bool hasWidthAni = [self hasTransition:TRANSITION_WIDTH];
  bool hasHeightAni = [self hasTransition:TRANSITION_HEIGHT];

  CGRect beginFrame;
  beginFrame.origin.x = ((hasLeftAni && xPosChanged) || (hasWidthAni && xRightNotChanged))
                            ? oldFrame.origin.x
                            : newFrame.origin.x;
  beginFrame.origin.y = ((hasTopAni && yPosChanged) || (hasHeightAni && yBottomNotChange))
                            ? oldFrame.origin.y
                            : newFrame.origin.y;
  beginFrame.size.width = widthChanged && hasWidthAni ? oldFrame.size.width : newFrame.size.width;
  beginFrame.size.height =
      heightChanged && hasHeightAni ? oldFrame.size.height : newFrame.size.height;

  bool centerXChanged =
      (beginFrame.origin.x + beginFrame.size.width * _ui.view.layer.anchorPoint.x) !=
      (newFrame.origin.x + newFrame.size.width * _ui.view.layer.anchorPoint.x);
  bool centerYChanged =
      (beginFrame.origin.y + beginFrame.size.height * _ui.view.layer.anchorPoint.y) !=
      (newFrame.origin.y + newFrame.size.height * _ui.view.layer.anchorPoint.y);

  // Due to iOS limitations, when position x animation and width animation exist at the same time,
  // we have to keep their animation timing properties consistent. Copy a temp animation infos
  // container for some temporary infos adjustments
  NSMutableDictionary* transitionInfosTemp = [_transitionInfos mutableCopy];
  if (hasLeftAni && hasWidthAni && xPosChanged && widthChanged) {
    [LynxAnimationInfo makePositionAndSizeTimingInfoConsistent:transitionInfosTemp
                                               withPositionKey:TRANSITION_LAYOUT_POSITION_X
                                                   withSizeKey:TRANSITION_WIDTH];
  }
  // Ditto
  if (hasTopAni && hasHeightAni && yPosChanged && heightChanged) {
    [LynxAnimationInfo makePositionAndSizeTimingInfoConsistent:transitionInfosTemp
                                               withPositionKey:TRANSITION_LAYOUT_POSITION_Y
                                                   withSizeKey:TRANSITION_HEIGHT];
  }

  // update start frame before animation
  [_ui updateFrameWithoutLayoutAnimation:beginFrame
                             withPadding:padding
                                  border:border
                                  margin:margin];

  [_ui onLayoutAnimationStart:beginFrame];

  __weak LynxTransitionAnimationManager* weakSelf = self;

  void (^finishedBlock)(BOOL finished) = ^void(BOOL finished) {
    __strong LynxTransitionAnimationManager* strongSelf = weakSelf;
    if (strongSelf) {
      [strongSelf.ui updateFrameWithoutLayoutAnimation:strongSelf.latestFrame
                                           withPadding:padding
                                                border:border
                                                margin:margin];
      [strongSelf.ui onLayoutAnimationEnd:beginFrame];
    }
    strongSelf = nil;
  };

  for (NSNumber* prop in transitionInfosTemp) {
    LynxAnimationInfo* info = transitionInfosTemp[prop];
    if ((info.prop & TRANSITION_LAYOUT) == 0) {
      continue;
    }
    switch (info.prop) {
      case TRANSITION_LEFT:
      case TRANSITION_RIGHT: {
        if (xPosChanged) {
          [self createLayoutAnimation:info
                           beginFrame:beginFrame
                             endFrame:newFrame
                             callback:finishedBlock
               needAdditionalAnimator:NO];
        }
        break;
      }
      case TRANSITION_TOP:
      case TRANSITION_BOTTOM: {
        if (yPosChanged) {
          [self createLayoutAnimation:info
                           beginFrame:beginFrame
                             endFrame:newFrame
                             callback:finishedBlock
               needAdditionalAnimator:NO];
        }
        break;
      }
      case TRANSITION_WIDTH: {
        if (widthChanged) {
          // We need additional left animations in the following cases to ensure the correctness of
          // the animation.
          bool needAdditionalAnimator = centerXChanged && !(hasLeftAni && xPosChanged);
          [self createLayoutAnimation:info
                           beginFrame:beginFrame
                             endFrame:newFrame
                             callback:finishedBlock
               needAdditionalAnimator:needAdditionalAnimator];
        }
        break;
      }
      case TRANSITION_HEIGHT: {
        if (heightChanged) {
          // We need additional top animations in the following cases to ensure the correctness of
          // the animation.
          bool needAdditionalAnimator = centerYChanged && !(hasTopAni && yPosChanged);
          [self createLayoutAnimation:info
                           beginFrame:beginFrame
                             endFrame:newFrame
                             callback:finishedBlock
               needAdditionalAnimator:needAdditionalAnimator];
        }
        break;
      }
      default:
        break;
    }
  }
  self.latestFrame = newFrame;
  return YES;
}

- (BOOL)isTransitionSize:(CGRect)oldFrame newFrame:(CGRect)newFrame {
  return (([self hasTransition:TRANSITION_WIDTH] && oldFrame.size.width != newFrame.size.width) ||
          ([self hasTransition:TRANSITION_HEIGHT] && oldFrame.size.height != newFrame.size.height));
}

- (BOOL)isTransitionPosition:(CGRect)oldFrame newFrame:(CGRect)newFrame {
  return (([self hasTransition:TRANSITION_LEFT] || [self hasTransition:TRANSITION_RIGHT]) &&
          oldFrame.origin.x != newFrame.origin.x) ||
         (([self hasTransition:TRANSITION_TOP] || [self hasTransition:TRANSITION_BOTTOM]) &&
          oldFrame.origin.y != newFrame.origin.y);
}

- (BOOL)isTransitionBackgroundColor {
  return [self hasTransition:TRANSITION_BACKGROUND_COLOR];
}

- (BOOL)isTransitionOpacity:(CGFloat)oldOpacity newOpacity:(CGFloat)newOpacity {
  return [self hasTransition:OPACITY] && oldOpacity != newOpacity;
}

- (BOOL)isTransitionVisibility:(BOOL)oldState newState:(BOOL)newState {
  return [self hasTransition:TRANSITION_VISIBILITY] && oldState != newState;
}

- (BOOL)isTransitionTransform:(CATransform3D)oldTransform newTransform:(CATransform3D)newTransform {
  return [self hasTransition:TRANSITION_TRANSFORM] &&
         !CATransform3DEqualToTransform(oldTransform, newTransform);
}

- (BOOL)isTransitionTransformRotation:(LynxAnimationTransformRotation*)oldTransformRotation
                 newTransformRotation:(LynxAnimationTransformRotation*)newTransformRotation {
  return [self hasTransition:TRANSITION_TRANSFORM] &&
         ![oldTransformRotation isEqualToTransformRotation:newTransformRotation];
}

- (BOOL)hasTransition:(LynxAnimationProp)prop {
  return [_transitionInfos objectForKey:[NSNumber numberWithUnsignedInteger:prop]] != nil;
}

#pragma mark - Transition

- (void)createLayoutAnimation:(LynxAnimationInfo*)info
                   beginFrame:(CGRect)beginFrame
                     endFrame:(CGRect)endFrame
                     callback:(CompletionBlock)block
       needAdditionalAnimator:(BOOL)needAdditionalAnimator {
  info.completeBlock = block;
  CAAnimation* animator = nil;
  switch (info.prop) {
    case TRANSITION_LEFT:
    case TRANSITION_RIGHT: {
      animator = [LynxAnimationUtils
          createBasicAnimation:@"position.x"
                          from:@(beginFrame.origin.x +
                                 beginFrame.size.width * _ui.view.layer.anchorPoint.x)
                            to:@(endFrame.origin.x +
                                 endFrame.size.width * _ui.view.layer.anchorPoint.x)
                          info:info];
      break;
    }
    case TRANSITION_TOP:
    case TRANSITION_BOTTOM: {
      animator = [LynxAnimationUtils
          createBasicAnimation:@"position.y"
                          from:@(beginFrame.origin.y +
                                 beginFrame.size.height * _ui.view.layer.anchorPoint.y)
                            to:@(endFrame.origin.y +
                                 endFrame.size.height * _ui.view.layer.anchorPoint.y)
                          info:info];
      break;
    }
    case TRANSITION_WIDTH: {
      CAAnimationGroup* group = [[CAAnimationGroup alloc] init];
      NSMutableArray<CAAnimation*>* animations = [[NSMutableArray alloc] init];
      CABasicAnimation* widthAnimator =
          [LynxAnimationUtils createBasicAnimation:@"bounds.size.width"
                                              from:@(beginFrame.size.width)
                                                to:@(endFrame.size.width)
                                              info:info];
      [animations addObject:widthAnimator];
      if (needAdditionalAnimator) {
        CABasicAnimation* leftAnimator = [LynxAnimationUtils
            createBasicAnimation:@"position.x"
                            from:@(beginFrame.origin.x +
                                   beginFrame.size.width * _ui.view.layer.anchorPoint.x)
                              to:@(endFrame.origin.x +
                                   endFrame.size.width * _ui.view.layer.anchorPoint.x)
                            info:info];
        [animations addObject:leftAnimator];
      }
      [group setAnimations:animations];
      animator = group;
      break;
    }
    case TRANSITION_HEIGHT: {
      CAAnimationGroup* group = [[CAAnimationGroup alloc] init];
      NSMutableArray<CAAnimation*>* animations = [[NSMutableArray alloc] init];
      CABasicAnimation* heightAnimator =
          [LynxAnimationUtils createBasicAnimation:@"bounds.size.height"
                                              from:@(beginFrame.size.height)
                                                to:@(endFrame.size.height)
                                              info:info];
      [animations addObject:heightAnimator];
      if (needAdditionalAnimator) {
        CABasicAnimation* topAnimator = [LynxAnimationUtils
            createBasicAnimation:@"position.y"
                            from:@(beginFrame.origin.y +
                                   beginFrame.size.height * _ui.view.layer.anchorPoint.y)
                              to:@(endFrame.origin.y +
                                   endFrame.size.height * _ui.view.layer.anchorPoint.y)
                            info:info];
        [animations addObject:topAnimator];
      }
      [group setAnimations:animations];
      animator = group;
      break;
    }
    default:
      break;
  }
  if (animator != nil) {
    [self enqueueTransitionAnimation:animator withConfig:info];
  }
}

#pragma mark - transition background

- (void)performTransitionAnimationsWithBackground:(UIColor*)color callback:(CompletionBlock)block {
  LynxAnimationInfo* config = [_transitionInfos
      objectForKey:[NSNumber numberWithUnsignedInteger:TRANSITION_BACKGROUND_COLOR]];
  if (!config) {
    return;
  }
  config.completeBlock = block;
  const BOOL isSimple = ((_ui.backgroundManager.animationOptions & LynxAnimOptHasBGComplex) == 0);
  id fromValue;
  id toValue;
  if (isSimple) {
    fromValue = _ui.backgroundManager.backgroundLayer
                    ? (id)_ui.backgroundManager.backgroundLayer.presentationLayer.backgroundColor
                    : (id)_ui.view.layer.presentationLayer.backgroundColor;
    _ui.backgroundManager.backgroundColor = color;
    toValue = (id)color.CGColor;
  } else {
    fromValue = (id)[_ui.backgroundManager getBackgroundImageForContentsAnimation].CGImage;
    _ui.backgroundManager.backgroundColor = color;
    toValue = (id)[_ui.backgroundManager getBackgroundImageForContentsAnimation].CGImage;
  }
  CABasicAnimation* c =
      [LynxAnimationUtils createBasicAnimation:(isSimple ? @"backgroundColor" : @"contents")
                                          from:fromValue
                                            to:toValue
                                          info:config];

  [self enqueueTransitionAnimation:c withConfig:config];
}

#pragma mark - transition visibility

- (void)performTransitionAnimationsWithVisibility:(BOOL)hidden callback:(CompletionBlock)block {
  LynxAnimationInfo* config =
      [_transitionInfos objectForKey:[NSNumber numberWithUnsignedInteger:TRANSITION_VISIBILITY]];
  if (!config) {
    return;
  }

  if (_ui.view.hidden == hidden) {
    return;
  }
  CGFloat rawOpacity = _ui.view.alpha;
  CGFloat startOpacity = rawOpacity;
  CGFloat endOpacity = rawOpacity;
  bool rawHidden = _ui.view.hidden;
  if (rawHidden == YES) {
    _ui.view.alpha = 0;
    _ui.view.hidden = NO;
    startOpacity = 0;
  } else {
    endOpacity = 0;
  }
  __weak typeof(self) weakSelf = self;
  config.completeBlock = ^(BOOL finished) {
    weakSelf.ui.view.hidden = hidden;
    weakSelf.ui.view.alpha = rawOpacity;
    block(true);
  };

  CABasicAnimation* animation =
      [LynxAnimationUtils createBasicAnimation:@"opacity"
                                          from:[NSNumber numberWithFloat:startOpacity]
                                            to:[NSNumber numberWithFloat:endOpacity]
                                          info:config];

  [self enqueueTransitionAnimation:animation withConfig:config];
}

#pragma mark - transition opacity

- (void)performTransitionAnimationsWithOpacity:(CGFloat)newOpacity callback:(CompletionBlock)block {
  LynxAnimationInfo* config =
      [_transitionInfos objectForKey:[NSNumber numberWithUnsignedInteger:OPACITY]];
  if (!config) {
    return;
  }
  config.completeBlock = block;
  CGFloat rawOpacity;
  if (_ui.backgroundManager.opacityView) {
    rawOpacity = _ui.backgroundManager.opacityView.layer.presentationLayer.opacity;
  } else {
    rawOpacity = _ui.view.layer.presentationLayer.opacity;
  }
  CABasicAnimation* animation =
      [LynxAnimationUtils createBasicAnimation:@"opacity"
                                          from:[NSNumber numberWithFloat:rawOpacity]
                                            to:[NSNumber numberWithFloat:newOpacity]
                                          info:config];

  [self enqueueTransitionAnimation:animation withConfig:config];
}

#pragma mark - transition transform

- (void)performTransitionAnimationsWithTransform:(CATransform3D)newTransform
                          transformWithoutRotate:(CATransform3D)newTransformWithoutRotate
                        transformWithoutRotateXY:(CATransform3D)newTransformWithoutRotateXY
                                        rotation:(LynxAnimationTransformRotation*)newRotation
                                        callback:(CompletionBlock)block {
  LynxAnimationInfo* config =
      [_transitionInfos objectForKey:[NSNumber numberWithUnsignedInteger:TRANSITION_TRANSFORM]];
  if (!config) {
    return;
  }
  config.completeBlock = block;
  LynxUI* ui = _ui;
  LynxAnimationTransformRotation* oldRotation = ui.lastTransformRotation;
  LynxTransitionTransform from = {
      ui.lastTransformWithoutRotate,
      ui.lastTransformWithoutRotateXY,
      {oldRotation.rotationX, oldRotation.rotationY, oldRotation.rotationZ}};
  LynxTransitionTransform to = {
      newTransformWithoutRotate,
      newTransformWithoutRotateXY,
      {newRotation.rotationX, newRotation.rotationY, newRotation.rotationZ}};
  CAAnimationGroup* group = [CAAnimationGroup animation];
  group.animations = CreateTransformAnimations(&from, &to, config);
  [self enqueueTransitionAnimation:group withConfig:config];
}

#pragma mark - prepare && start transition

- (void)enqueueTransitionAnimation:(CAAnimation*)animation withConfig:(LynxAnimationInfo*)config {
  [LynxAnimationUtils applyAnimationProperties:animation withInfo:config forLayer:_ui.view.layer];
  LynxAnimationDelegate* delegate = [self createTransitionDelegate:config animation:animation];
  animation.delegate = delegate;
  NSNumber* prop = @(config.prop);
  _transitionDelegates[prop] = delegate;
  _transitionAnimations[prop] = animation;
}

- (void)refreshPlatformTransform {
  LynxRoutedTransition* transition = _routedTransitions[kTransformTransitionSlot];
  LynxUI* ui = _ui;
  if (!transition || transition->_state != LynxRoutedTransitionRunning ||
      !transition->_transform.sizeDependent ||
      CGSizeEqualToSize(transition->_transform.size, ui.updatedFrame.size)) {
    return;
  }
  NSString* name = [LynxConverter toLynxPropName:TRANSITION_TRANSFORM];
  CAAnimationGroup* animation = (CAAnimationGroup*)[ui.view.layer animationForKey:name];
  if (animation.delegate != transition) {
    return;
  }
  animation = [animation copy];
  animation.animations = CreateRoutedTransformAnimations(ui, transition, transition, 0);
  [animation setValue:@(++transition->_transform.installRevision)
               forKey:kTransitionInstallRevision];
  AttachPlatformTransition(ui, animation, TRANSITION_TRANSFORM, name);
}

- (void)applyTransitionAnimation {
  [self refreshPlatformTransform];
  NSEnumerator* e = [_transitionAnimations keyEnumerator];
  NSNumber* prop;
  while ((prop = e.nextObject) != nil) {
    // TODO: (liyanbo) refactor this.
    LynxAnimationInfo* info = _transitionInfos[prop];
    // Remove old animations of info.prop on all layers firstly.
    [LynxAnimationUtils removeAnimation:_ui withName:[LynxConverter toLynxPropName:info.prop]];
    if (info.prop == TRANSITION_BACKGROUND_COLOR) {
      const BOOL isSimple =
          ((_ui.backgroundManager.animationOptions & LynxAnimOptHasBGComplex) == 0);
      CAAnimation* animation = _transitionAnimations[prop];
      if (isSimple) {
        CALayer* targetLayer = _ui.backgroundManager.backgroundLayer;
        if (targetLayer) {
          [targetLayer addAnimation:animation
                             forKey:[NSString stringWithFormat:@"%@%@", DUP_ANI_PREFIX, info.name]];
        } else {
          [_ui.view.layer addAnimation:animation forKey:info.name];
        }
      } else {
        [_ui.view.layer addAnimation:animation forKey:info.name];
      }
    } else if (info.prop == OPACITY && _ui.backgroundManager.overlapRendering) {
      [LynxAnimationUtils attachOpacityTo:_ui
                                animation:_transitionAnimations[prop]
                                   forKey:info.name];

    } else {
      [LynxAnimationUtils attachTo:_ui animation:_transitionAnimations[prop] forKey:info.name];
      if (info.prop == TRANSITION_TRANSFORM || (info.prop & TRANSITION_LAYOUT)) {
        [self autoAddAnimationForComplexBackgroundWithUI:_ui
                                                   frame:_latestFrame
                                                  config:info
                                                withProp:info.prop];
      }
    }
  }
  [_transitionAnimations removeAllObjects];
}
#pragma mark - utils

- (void)transitionsDidAssemble {
  _transitionInfos = nil;
  [self removeAllTransitionAnimation];
  _transitionAnimations = nil;
  _transitionDelegates = nil;
}

- (void)removeAllTransitionAnimation {
  [self removeTransitionAnimation:OPACITY];
  for (LynxAnimationProp i = TRANSITION_WIDTH; i <= TRANSITION_TRANSFORM; i <<= 1) {
    [self removeTransitionAnimation:i];
  }
  _routedTransitions[kOpacityTransitionSlot] = nil;
  _routedTransitions[kTransformTransitionSlot] = nil;
}

- (void)removeTransitionAnimation:(LynxAnimationProp)prop {
  if (prop == OPACITY || prop == TRANSITION_TRANSFORM) {
    LynxRoutedTransition* transition = _routedTransitions[RoutedTransitionSlot(prop)];
    if (transition && transition->_state != LynxRoutedTransitionStopped) {
      transition->_targetValue = nil;
      [self removePlatformTransition:transition];
      return;
    }
  }
  NSNumber* key = [NSNumber numberWithUnsignedInteger:prop];
  LynxAnimationDelegate* delegate = [_transitionDelegates objectForKey:key];
  if (delegate) {
    [delegate forceStop];
    [_transitionDelegates removeObjectForKey:key];
  }
  [LynxAnimationUtils removeAnimation:_ui withName:[LynxConverter toLynxPropName:prop]];
}

- (void)removeAllLayoutTransitionAnimation {
  [self removeTransitionAnimation:TRANSITION_WIDTH];
  [self removeTransitionAnimation:TRANSITION_HEIGHT];
  [self removeTransitionAnimation:TRANSITION_LEFT];
  [self removeTransitionAnimation:TRANSITION_TOP];
  [self removeTransitionAnimation:TRANSITION_RIGHT];
  [self removeTransitionAnimation:TRANSITION_BOTTOM];
}

- (LynxAnimationDelegate*)createTransitionDelegate:(LynxAnimationInfo*)info
                                         animation:(CAAnimation*)transition {
  const NSTimeInterval duration = MAX(transition.duration, 0);
  const NSTimeInterval startElapsedTime = MIN(MAX(-info.delay, 0), duration);
  // A forced stop has no CAAnimation argument, so retain its scheduled begin time.
  const CFTimeInterval beginTime = transition.beginTime;
  __weak LynxTransitionAnimationManager* weakSelf = self;
  return [LynxAnimationDelegate
      initWithDidStart:^(CAAnimation* animation) {
        __strong LynxTransitionAnimationManager* strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }

        // Notify animation did start.
        [strongSelf.ui.context.observer notifyAnimationStart];
        // dispatch transition start event
        [LynxAnimationDelegate
            sendAnimationEvent:strongSelf.ui
                     eventName:(NSString*)kTransitionEventStart
                   eventParams:@{
                     @"animation_type" : [LynxConverter toLynxPropName:info.prop],
                     @"elapsedTime" : @(startElapsedTime)
                   }];
      }
      didStop:^(CAAnimation* animation, BOOL finished) {
        __strong LynxTransitionAnimationManager* strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        NSTimeInterval elapsedTime = duration;
        if (!finished) {
          CFTimeInterval localTime = [strongSelf.ui.view.layer convertTime:CACurrentMediaTime()
                                                                 fromLayer:nil];
          elapsedTime = MIN(MAX(localTime - beginTime, 0), duration);
        }
        if (info.completeBlock) {
          info.completeBlock(finished);
          info.completeBlock = nil;
        }

        // Notify animation did stop.
        [strongSelf.ui.context.observer notifyAnimationEnd];
        // dispatch transition end event
        [LynxAnimationDelegate
            sendAnimationEvent:strongSelf.ui
                     eventName:(NSString*)kTransitionEventEnd
                   eventParams:@{
                     @"animation_type" : [LynxConverter toLynxPropName:info.prop],
                     @"elapsedTime" : @(elapsedTime),
                     @"finished" : [NSNumber numberWithBool:finished]
                   }];

        // keep callback just exec once.
        [strongSelf->_transitionDelegates
            removeObjectForKey:[NSNumber numberWithUnsignedInteger:info.prop]];
      }];
}

- (void)setupTransitionAnimationForContents:(CABasicAnimation*)content
                                 withConfig:(LynxAnimationInfo*)config
                                   forLayer:(CALayer*)layer
                                   withProp:(LynxAnimationProp)prop {
  [LynxAnimationUtils applyAnimationProperties:content withInfo:config forLayer:layer];
  NSString* animationName =
      [NSString stringWithFormat:@"%@%@", DUP_CONTENT_ANI_PREFIX, config.name];
  [layer addAnimation:content forKey:animationName];
}

- (void)autoAddAnimationForComplexBackgroundWithUI:(LynxUI*)ui
                                             frame:(CGRect)newFrame
                                            config:(LynxAnimationInfo*)config
                                          withProp:(LynxAnimationProp)prop {
  LynxBackgroundManager* bg = ui.backgroundManager;
  const int opts = bg.animationOptions;
  if ((opts & (LynxAnimOptHasBorderComplex | LynxAnimOptHasBGComplex)) == 0) {
    return;
  }

  // only bound change, we do not need to get different image, an image large enough is ok
  CGSize size = ui.frame.size;
  if (size.width < newFrame.size.width) {
    size.width = newFrame.size.width;
  }
  if (size.height < newFrame.size.height) {
    size.height = newFrame.size.height;
  }
  if (size.width < 1) {
    size.width = 1;
  }
  if (size.height < 1) {
    size.height = 1;
  }

  if (opts & LynxAnimOptHasBorderComplex) {
    CABasicAnimation* content;
    if (LynxBgTypeComplex == bg.borderLayer.type) {
      UIImage* image = [bg getBorderImageForContentsAnimationWithSize:size];
      content = [LynxAnimationUtils createBasicAnimation:@"contents"
                                                    from:(id)image.CGImage
                                                      to:(id)image.CGImage
                                                    info:config];
      [LynxAnimationUtils addContentAnimationDelegateTo:content
                                         forTargetLayer:bg.borderLayer
                                            withContent:image
                                               withProp:prop];

    } else if (LynxBgTypeShape == bg.borderLayer.type && TRANSITION_TRANSFORM != prop) {
      CGPathRef from = CGPathRetain(bg.borderLayer.path);
      CGPathRef to = [bg getBorderPathForAnimationWithSize:newFrame.size];
      content = [LynxAnimationUtils createBasicAnimation:@"path"
                                                    from:(__bridge id)from
                                                      to:(__bridge id)to
                                                    info:config];
      [LynxAnimationUtils addPathAnimationDelegateTo:content
                                      forTargetLayer:bg.borderLayer
                                            withPath:to
                                            withProp:prop];
      // The CGPathRef 'to' will be released when in the animation end callback later.
      CGPathRelease(from);
    }
    [self setupTransitionAnimationForContents:content
                                   withConfig:config
                                     forLayer:bg.borderLayer
                                     withProp:prop];
  }
  if (opts & LynxAnimOptHasBGComplex) {
    __weak LynxTransitionAnimationManager* weakSelf = self;
    lynx_async_display_completion_block_t completionBlock = ^(UIImage* _Nonnull image) {
      __strong LynxTransitionAnimationManager* strongSelf = weakSelf;
      if (strongSelf) {
        CABasicAnimation* content = [LynxAnimationUtils createBasicAnimation:@"contents"
                                                                        from:(id)image.CGImage
                                                                          to:(id)image.CGImage
                                                                        info:config];
        [LynxAnimationUtils addContentAnimationDelegateTo:content
                                           forTargetLayer:bg.backgroundLayer
                                              withContent:image
                                                 withProp:prop];
        [strongSelf setupTransitionAnimationForContents:content
                                             withConfig:config
                                               forLayer:bg.backgroundLayer
                                               withProp:prop];
      }
    };
    [bg getBackgroundImageForContentsAnimationAsync:completionBlock withSize:size];
  }
}

@end
