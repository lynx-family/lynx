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
#import <Lynx/LynxTransitionAnimationManager+Private.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIUnitUtils.h>
#import "LynxAnimationUtils+Private.h"

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

@interface LynxRoutedTransition : NSObject <CAAnimationDelegate> {
 @public
  __weak LynxTransitionAnimationManager* _manager;
  uint64_t _animationID;
  uint32_t _generation;
  LynxAnimationProp _property;
  id _startValue;  // The installed start value, cleared when the transition stops.

 @private
  LynxGlobalObserver* _observer;
  BOOL _didStart;
}
- (void)endObservation __attribute__((objc_direct));
@end

static id RoutedStartValue(LynxUI* ui, LynxRoutedTransition* transition) {
  if (!transition || !transition->_startValue) {
    return nil;
  }
  CALayer* layer = transition->_property == OPACITY ? ui.backgroundManager.opacityView.layer : nil;
  CALayer* presentation = (layer ?: ui.view.layer).presentationLayer;
  if (!presentation) {
    return transition->_startValue;
  }
  return transition->_property == OPACITY ? @(presentation.opacity)
                                          : [NSValue valueWithCATransform3D:presentation.transform];
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

static CAAnimationGroup* CreateTransformTransition(const LynxTransitionTransform* from,
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
  CAAnimationGroup* group = [CAAnimationGroup animation];
  group.animations = animations;
  return group;
}

@interface LynxTransitionAnimationManager () {
  LynxRoutedTransition* _routedTransitions[kRoutedTransitionCount];
  NSMutableDictionary<NSNumber*, CAAnimation*>* _transitionAnimations;
}
@property(nonatomic, weak, readonly) LynxUI* ui;
@property(nonatomic, readonly, nullable)
    NSMutableDictionary<NSNumber*, LynxAnimationInfo*>* transitionInfos;
@property(nonatomic, readonly, nullable)
    NSMutableDictionary<NSNumber*, LynxAnimationDelegate*>* transitionDelegates;
@property(nonatomic) CGRect latestFrame;

- (BOOL)removePlatformTransition:(LynxRoutedTransition*)transition __attribute__((objc_direct));
- (void)transitionsDidAssemble;
- (BOOL)isTransitionSize:(CGRect)oldFrame newFrame:(CGRect)newFrame;
- (BOOL)isTransitionPosition:(CGRect)oldFrame newFrame:(CGRect)newFrame;
- (BOOL)hasTransition:(LynxAnimationProp)prop;
- (void)prepareTransitionAnimation:(CAAnimation*)animation
                        withConfig:(LynxAnimationInfo*)config
                          forLayer:(CALayer*)layer;
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
  if (_startValue && !_didStart) {
    _didStart = YES;
    _observer = _manager.ui.context.observer;
    [_observer notifyAnimationStart];
  }
}

- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)finished {
  if (_startValue && ![_manager removePlatformTransition:self]) {
    [self endObservation];
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
  from = RoutedStartValue(ui, previous) ?: from;
  CAAnimation* animation;
  if (property == OPACITY) {
    animation = [LynxAnimationUtils createBasicAnimation:@"opacity" from:from to:to info:info];
  } else {
    LynxTransitionTransform start = {0}, end = {0};
    if ([from isKindOfClass:[NSValue class]]) {
      DecomposePresentationTransform([from CATransform3DValue], &start);
    } else {
      ConvertPlatformTransformEndpoint(from, ui, &start);
    }
    ConvertPlatformTransformEndpoint(to, ui, &end);
    animation = CreateTransformTransition(&start, &end, info);
  }
  LynxRoutedTransition* transition = [LynxRoutedTransition new];
  transition->_manager = self;
  transition->_animationID = animationID;
  transition->_generation = generation;
  transition->_property = property;
  transition->_startValue = from;
  _routedTransitions[slot] = transition;

  if (previous) {
    previous->_startValue = nil;
  }
  NSString* name = [LynxConverter toLynxPropName:property];
  [LynxAnimationUtils removeAnimation:ui withName:name];
  [previous endObservation];
  if (_routedTransitions[slot] != transition || !transition->_startValue) {
    return;
  }

  animation.delegate = transition;
  [self prepareTransitionAnimation:animation withConfig:info forLayer:ui.view.layer];
  if (property == OPACITY && ui.backgroundManager.opacityView) {
    [LynxAnimationUtils attachOpacityTo:ui animation:animation forKey:name];
  } else {
    [LynxAnimationUtils attachTo:ui animation:animation forKey:name];
  }
}

- (void)cancelPlatformTransition:(uint64_t)animationID generation:(uint32_t)generation {
  for (NSUInteger slot = 0; slot < kRoutedTransitionCount; ++slot) {
    LynxRoutedTransition* transition = _routedTransitions[slot];
    if (transition && transition->_animationID == animationID &&
        generation >= transition->_generation) {
      transition->_generation = generation;
      [self removePlatformTransition:transition];
      return;
    }
  }
}

- (BOOL)removePlatformTransition:(LynxRoutedTransition*)transition {
  if (!transition || !transition->_startValue ||
      _routedTransitions[RoutedTransitionSlot(transition->_property)] != transition) {
    return NO;
  }
  transition->_startValue = nil;
  [LynxAnimationUtils removeAnimation:_ui
                             withName:[LynxConverter toLynxPropName:transition->_property]];
  [transition endObservation];
  return YES;
}

- (void)removeLegacyTransformTransition {
  LynxRoutedTransition* transition = _routedTransitions[kTransformTransitionSlot];
  if (!transition || !transition->_startValue) {
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
  [self enqueueTransitionAnimation:CreateTransformTransition(&from, &to, config) withConfig:config];
}

#pragma mark - prepare && start transition

- (void)prepareTransitionAnimation:(CAAnimation*)animation
                        withConfig:(LynxAnimationInfo*)config
                          forLayer:(CALayer*)layer {
  [LynxAnimationUtils applyAnimationProperties:animation withInfo:config forLayer:layer];
  LynxApplyHighFrameRateHint(animation, _ui.context.prefersHighFrameRateForPlatformAnimations);
}

- (void)enqueueTransitionAnimation:(CAAnimation*)animation withConfig:(LynxAnimationInfo*)config {
  [self prepareTransitionAnimation:animation withConfig:config forLayer:_ui.view.layer];
  LynxAnimationDelegate* delegate = [self createTransitionDelegate:config animation:animation];
  animation.delegate = delegate;
  NSNumber* prop = @(config.prop);
  _transitionDelegates[prop] = delegate;
  _transitionAnimations[prop] = animation;
}

- (void)applyTransitionAnimation {
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
  if ((prop == OPACITY || prop == TRANSITION_TRANSFORM) &&
      [self removePlatformTransition:_routedTransitions[RoutedTransitionSlot(prop)]]) {
    return;
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
        [strongSelf.transitionDelegates
            removeObjectForKey:[NSNumber numberWithUnsignedInteger:info.prop]];
      }];
}

- (void)setupTransitionAnimationForContents:(CABasicAnimation*)content
                                 withConfig:(LynxAnimationInfo*)config
                                   forLayer:(CALayer*)layer
                                   withProp:(LynxAnimationProp)prop {
  [self prepareTransitionAnimation:content withConfig:config forLayer:layer];
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
