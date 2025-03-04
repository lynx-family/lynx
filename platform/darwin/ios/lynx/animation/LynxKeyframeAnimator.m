// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxKeyframeAnimator.h"
#import <mach/mach_time.h>
#import "LynxAnimationDelegate.h"
#import "LynxAnimationUtils.h"
#import "LynxColorUtils.h"
#import "LynxConverter+LynxCSSType.h"
#import "LynxConverter+Transform.h"
#import "LynxConverter+UI.h"
#import "LynxGlobalObserver.h"
#import "LynxLog.h"
#import "LynxPropsProcessor.h"
#import "LynxUI+Internal.h"
#import "LynxUI.h"

static const CFTimeInterval kTimeNotInit = 0;
static const double kAnimationIterationCountInfinite = 1E9;

#pragma mark - PauseTimeHelper
@interface PauseTimeHelper : NSObject
@property(nonatomic) CFTimeInterval pauseTime;
@property(nonatomic, weak) LynxUI* ui;
- (void)recordPauseTime;
- (CFTimeInterval)getPauseDuration;
@end

@implementation PauseTimeHelper
- (instancetype)initWithUI:(LynxUI*)lynxUI {
  self = [super init];
  if (self) {
    _pauseTime = kTimeNotInit;
    _ui = lynxUI;
  }
  return self;
}

- (void)recordPauseTime {
  if (_pauseTime == kTimeNotInit) {
    _pauseTime = [_ui.view.layer convertTime:getSyncedTimestamp() fromLayer:nil];
  }
}
- (CFTimeInterval)getPauseDuration {
  if (_pauseTime == kTimeNotInit) {
    return 0;
  }
  CFTimeInterval pauseDuration =
      [_ui.view.layer convertTime:getSyncedTimestamp() fromLayer:nil] - _pauseTime;
  _pauseTime = kTimeNotInit;
  return pauseDuration;
}

@end

#pragma mark - LynxKeyframeParsedData
@implementation LynxKeyframeParsedData
- (instancetype)init {
  _keyframeValues = [[NSMutableDictionary alloc] init];
  _keyframeTimes = [[NSMutableDictionary alloc] init];
  _beginStyles = [[NSMutableDictionary alloc] init];
  _endStyles = [[NSMutableDictionary alloc] init];
  _isPercentTransform = NO;
  return self;
}
@end

#pragma mark - LynxKeyframeAnimator
@interface LynxKeyframeAnimator ()
@property(nonatomic, strong, nullable) LynxAnimationInfo* info;
@property(nonatomic, assign) LynxKFAnimatorState state;
@end

@implementation LynxKeyframeAnimator {
  LynxKeyframeParsedData* _keyframeParsedData;
  CFTimeInterval _keyframeStartTime;
  NSMutableDictionary<NSString*, CAKeyframeAnimation*>* _internalAnimators;
  PauseTimeHelper* _pauseTimeHelper;
  LynxAnimationDelegate* _delegate;
  CADisplayLink* _displayLink;
}

static NSString* const kTransformStr = @"transform";
static NSString* const kOpacityStr = @"opacity";
// for _ui.backgroundManager.backgroundLayer.backgroundColor
static NSString* const kExtraBackgroundColorStr = @"extraBackgroundColor";
// for _ui.view.layer.backgroundColor
static NSString* const kBackgroundColorStr = @"backgroundColor";
static NSString* const kTransformRotationXStr = @"transform.rotation.x";
static NSString* const kTransformRotationYStr = @"transform.rotation.y";
static NSString* const kTransformRotationZStr = @"transform.rotation.z";
static const CATransform3D kEmptyCATransform3D = {0};

+ (NSString*)kTransformStr {
  return kTransformStr;
}
+ (NSString*)kOpacityStr {
  return kOpacityStr;
}
+ (NSString*)kBackgroundColorStr {
  return kBackgroundColorStr;
}

- (instancetype)initWithUI:(LynxUI*)ui {
  self = [super init];
  if (self) {
    _ui = ui;
    _keyframeParsedData = nil;
    _propertyOriginValue = nil;
    _keyframeStartTime = kTimeNotInit;
    _info = nil;
    _state = LynxKFAnimatorStateIdle;
    _internalAnimators = nil;
    _pauseTimeHelper = [[PauseTimeHelper alloc] initWithUI:ui];
    _autoResumeAnimation = YES;
    _displayLink = nil;
    [self initAnimationDelegate];
  }
  return self;
}

#pragma mark - public function
- (void)apply:(LynxAnimationInfo*)info {
  switch (_state) {
    case LynxKFAnimatorStateIdle:
    case LynxKFAnimatorStateCanceled:
    case LynxKFAnimatorStateCanceledLegacy: {
      if ([info isEqualToKeyframeInfo:_info] && _state == LynxKFAnimatorStateIdle &&
          ![self shouldReInitTransform]) {
        return;
      }
      // iOS-specific state : LynxKFAnimatorStateCanceledLegacy. In order to keep the legacy logic
      // consistent in legacy mode, we should return here.
      if ([info isEqualToKeyframeInfo:_info] && _state == LynxKFAnimatorStateCanceledLegacy) {
        return;
      }
      if (info.iterationCount <= 0 || info.duration <= 0) {
        return;
      }
      [self applyAnimationInfo:info];
      break;
    }

    case LynxKFAnimatorStatePaused:
    case LynxKFAnimatorStateRunning: {
      if ([info isEqualToKeyframeInfo:_info] && ![self shouldReInitTransform]) {
        return;
      }
      if ([info isOnlyPlayStateChanged:_info]) {
        if (_state == LynxKFAnimatorStatePaused) {
          [self resume:info];
        } else {
          [self pause:info];
        }
      } else {
        [self cancel];
        [self apply:info];
      }
      break;
    }
    default:
      break;
  }
}

- (void)dealloc {
  if (_displayLink) {
    [_displayLink invalidate];
    _displayLink = nil;
  }
}

- (void)sendCancelEvent {
  if (_autoResumeAnimation &&
      (_state == LynxKFAnimatorStateCanceled || _state == LynxKFAnimatorStateRunning ||
       _state == LynxKFAnimatorStatePaused)) {
    [_ui.context.observer notifyAnimationEnd];
    [LynxAnimationDelegate sendAnimationEvent:_ui
                                    eventName:(NSString*)kAnimationEventCancel
                                  eventParams:@{
                                    @"animation_type" : @"keyframe-animation",
                                    @"animation_name" : _info.name,
                                    @"finished" : @YES
                                  }];
  }
}

- (void)destroy {
  [_displayLink invalidate];
  _displayLink = nil;
  // Special case in iOS.
  // Most of time, the state should not be canceled when destroying, but iOS will
  // cancel the animation in the background, and special processing should be done at this time.
  [self sendCancelEvent];
  if (_state == LynxKFAnimatorStateCanceledLegacy) {
    [self removeAllAnimationFromLayer:_info];
  }
  [self cancel];
  [self removeAllAnimationFromLayer:_info];
  _internalAnimators = nil;
  [self restoreLayerStyles:_propertyOriginValue];
  _state = LynxKFAnimatorStateDestroy;
}

- (BOOL)shouldReInitTransform {
  return [self isPercentTransform] && [_ui didSizeChanged];
}

// Background layer and border layer may not have been created when keyframe start. So need to
// notify animator BGlayer have been added.
- (void)notifyBGLayerAdded {
  if (_state == LynxKFAnimatorStateRunning || _state == LynxKFAnimatorStatePaused) {
    LynxAnimationInfo* temp_info = _info;
    [self cancel];
    [self apply:temp_info];
  }
}

// UI's property may be changed when keyframe is running, we should notify animator that property
// has been updated in order to make sure UI's props correct after animator destoryed.
- (void)notifyPropertyUpdated:(NSString*)name value:(id)value {
  if ([_propertyOriginValue objectForKey:name]) {
    _propertyOriginValue[name] = value;
  }
  if ([name isEqualToString:kBackgroundColorStr]) {
    if ([_propertyOriginValue objectForKey:kExtraBackgroundColorStr]) {
      _propertyOriginValue[kExtraBackgroundColorStr] = value;
    }
  }
}

- (BOOL)isRunning {
  return _state == LynxKFAnimatorStateRunning;
}

- (void)tryToResumeAnimationOnNextFrame {
  if (!_displayLink) {
    _displayLink = [CADisplayLink displayLinkWithTarget:self
                                               selector:@selector(tryToResumeAnimation:)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
  } else {
    _displayLink.paused = false;
  }
}

- (void)detachFromUI {
  [self cancel];
  [self removeAllAnimationFromLayer:_info];
  [self restoreLayerStyles:_propertyOriginValue];
  _ui = nil;
}

- (void)attachToUI:(LynxUI*)ui {
  _ui = ui;
  [self applyAnimationInfo:_info];
}

#pragma mark - private function
- (void)tryToResumeAnimation:(CADisplayLink*)link {
  _displayLink.paused = true;
  // If LynxView's window is nil, it means that LynxView has been removed from window. We are not
  // necessary to do anything. We also need do nothing if app is in background. We will try to
  // resume animation in LynxView's willMoveToWindow lifetime or enterForeground lifetime.
  if ([[UIApplication sharedApplication] applicationState] == UIApplicationStateBackground ||
      _ui.context.rootView.window == nil) {
    return;
  }
  [self apply:_info];
}

- (void)initAnimationDelegate {
  __weak LynxKeyframeAnimator* weakSelf = self;
  _delegate = [LynxAnimationDelegate
      initWithDidStart:nil
               didStop:^(CAAnimation* anim, BOOL finished) {
                 if ([anim isKindOfClass:[CAKeyframeAnimation class]]) {
                   __strong LynxKeyframeAnimator* strongSelf = weakSelf;
                   if (!strongSelf) {
                     return;
                   }

                   // Send animation end event only if animator's state
                   // is running.
                   if (strongSelf.state == LynxKFAnimatorStateRunning) {
                     // send animation end event
                     if ((strongSelf.autoResumeAnimation && finished) ||
                         !strongSelf.autoResumeAnimation) {
                       // Notify animation did stop.
                       [strongSelf.ui.context.observer notifyAnimationEnd];
                       [LynxAnimationDelegate
                           sendAnimationEvent:strongSelf.ui
                                    eventName:(NSString*)kAnimationEventEnd
                                  eventParams:@{
                                    @"animation_type" : @"keyframe-animation",
                                    @"animation_name" : strongSelf.info.name,
                                    @"finished" : [NSNumber numberWithBool:finished]
                                  }];
                     }
                   }
                   // Legacy code. When `finished` is false and autoResumeAnimation is disabled,
                   // just change state and return to avoid break change.
                   if (!finished && !strongSelf.autoResumeAnimation) {
                     strongSelf.state = LynxKFAnimatorStateCanceledLegacy;
                     return;
                   }

                   // Change animator's state.
                   if (finished) {
                     [strongSelf finish];
                   } else {
                     [strongSelf cancel];
                     // If `finished` is NO, it means that animation was canceled by iOS system,
                     // which is not what we expect. So we try to resume animation on next
                     // frame.
                     [strongSelf tryToResumeAnimationOnNextFrame];
                   }
                 }
               }];
}

- (BOOL)isAnimationExpired:(LynxAnimationInfo*)info {
  if (_keyframeStartTime == kTimeNotInit) {
    return NO;
  }

  double allDuration = info.iterationCount >= kAnimationIterationCountInfinite
                           ? DBL_MAX
                           : info.duration * info.iterationCount + info.delay;

  return ([_ui.view.layer convertTime:getSyncedTimestamp() fromLayer:nil] - _keyframeStartTime >=
          allDuration);
}

// Check whether is percentage transform keyframe.
- (BOOL)isPercentTransform {
  if (_keyframeParsedData != nil && _keyframeParsedData.isPercentTransform) {
    return YES;
  }
  if (_ui.transformOriginRaw != nil && _ui.transformOriginRaw.hasPercent) {
    return YES;
  }
  return NO;
}

- (NSDictionary*)getKeyframeEndStyles {
  BOOL isAlternate = [LynxAnimationInfo isDirectionAlternate:_info];
  BOOL isReverse = [LynxAnimationInfo isDirectionReverse:_info];
  BOOL isIterCountOdd = ((uint64_t)_info.iterationCount % 2 != 0);
  BOOL reverse_style =
      (!isAlternate && isReverse) ||
      (isAlternate && ((isReverse && isIterCountOdd) || (!isReverse && !isIterCountOdd)));
  // If iteration count is infinite, use end styles always.
  reverse_style &= (_info.iterationCount < kAnimationIterationCountInfinite);

  return reverse_style ? _keyframeParsedData.beginStyles : _keyframeParsedData.endStyles;
}

- (NSMutableDictionary*)recordLayerStyles {
  NSMutableDictionary* styles = [[NSMutableDictionary alloc] init];
  NSArray<NSString*>* keys = [_keyframeParsedData.keyframeValues allKeys];
  for (NSString* key in keys) {
    if ([key isEqualToString:kTransformStr]) {
      // We should record _ui.transformRaw instand of _ui.view.layer.transform, because
      // _ui.view.layer.transform will be incorrect with percentage transform before layout ready.
      CATransform3D layerTrans = _ui.view.layer.transform;
      styles[kTransformStr] = _ui.transformRaw
                                  ? _ui.transformRaw
                                  : (CATransform3DEqualToTransform(layerTrans, kEmptyCATransform3D)
                                         ? [NSValue valueWithCATransform3D:CATransform3DIdentity]
                                         : [NSValue valueWithCATransform3D:layerTrans]);
    } else if ([key isEqualToString:kOpacityStr]) {
      if (_ui.backgroundManager.opacityView) {
        styles[kOpacityStr] =
            [NSNumber numberWithFloat:_ui.backgroundManager.opacityView.layer.opacity];
      } else {
        styles[kOpacityStr] = [NSNumber numberWithFloat:_ui.view.layer.opacity];
      }
    } else if ([key isEqualToString:kBackgroundColorStr]) {
      // backgroundColor will be apply to view or backgroundLayer, so we record both of them.
      CGColorRef clearColor = [[UIColor clearColor] CGColor];
      CGColorRef viewLayerColor = _ui.view.layer.backgroundColor;
      CGColorRef bgLayerColor = _ui.backgroundManager.backgroundLayer
                                    ? _ui.backgroundManager.backgroundLayer.backgroundColor
                                    : nil;
      CGColorRef bgManagerColor = _ui.backgroundManager.backgroundColor
                                      ? _ui.backgroundManager.backgroundColor.CGColor
                                      : nil;
      styles[kBackgroundColorStr] =
          viewLayerColor ? (__bridge id)viewLayerColor
                         : (bgManagerColor ? (__bridge id)bgManagerColor : (__bridge id)clearColor);
      if (_ui.backgroundManager.backgroundLayer != nil) {
        styles[kExtraBackgroundColorStr] =
            bgLayerColor ? (__bridge id)bgLayerColor
                         : (bgManagerColor ? (__bridge id)bgManagerColor : (__bridge id)clearColor);
      }
    } else if ([key isEqualToString:kTransformRotationXStr] ||
               [key isEqualToString:kTransformRotationYStr] ||
               [key isEqualToString:kTransformRotationZStr]) {
      id rotation = [_ui.view.layer valueForKeyPath:key];
      if (rotation) {
        styles[key] = rotation;
      }
    }
  }
  return styles;
}

- (NSDictionary*)recordPresentationLayerStyles {
  NSMutableDictionary* styles = [[NSMutableDictionary alloc] init];
  NSArray<NSString*>* keys = [_keyframeParsedData.keyframeValues allKeys];
  for (NSString* key in keys) {
    if ([key isEqualToString:kTransformStr]) {
      // PresentationLayer's states may be invalid in the early stage. So check whether the value is
      // valid firstly, if not, record layer's states.
      CATransform3D presentLayerTrans = _ui.view.layer.presentationLayer.transform;
      CATransform3D layerTrans = _ui.view.layer.transform;
      CATransform3D recordedTrans =
          CATransform3DEqualToTransform(presentLayerTrans, kEmptyCATransform3D) ? layerTrans
                                                                                : presentLayerTrans;
      recordedTrans = CATransform3DEqualToTransform(recordedTrans, kEmptyCATransform3D)
                          ? CATransform3DIdentity
                          : recordedTrans;
      styles[kTransformStr] = [NSValue valueWithCATransform3D:recordedTrans];
    } else if ([key isEqualToString:kOpacityStr]) {
      // When presentationLayer is nil, it is very early stage. We should get opacity from keyframe
      // begin value rather than from layer.
      if (_ui.view.layer.presentationLayer == nil) {
        styles[kOpacityStr] = [_keyframeParsedData.keyframeValues[@"opacity"] firstObject];
      } else {
        if (_ui.backgroundManager.opacityView) {
          styles[kOpacityStr] = [NSNumber
              numberWithFloat:_ui.backgroundManager.opacityView.layer.presentationLayer.opacity];
        } else {
          styles[kOpacityStr] = [NSNumber numberWithFloat:_ui.view.layer.presentationLayer.opacity];
        }
      }
    } else if ([key isEqualToString:kBackgroundColorStr]) {
      CGColorRef clearColor = [[UIColor clearColor] CGColor];
      CGColorRef viewPresentLayerColor =
          _ui.view.layer.presentationLayer ? _ui.view.layer.presentationLayer.backgroundColor : nil;
      CGColorRef viewLayerColor = _ui.view.layer ? _ui.view.layer.backgroundColor : nil;
      CALayer* bgPresentLayer = _ui.backgroundManager.backgroundLayer
                                    ? _ui.backgroundManager.backgroundLayer.presentationLayer
                                    : nil;
      CGColorRef bgPresentLayerColor = bgPresentLayer ? bgPresentLayer.backgroundColor : nil;
      CGColorRef bgLayerColor = _ui.backgroundManager.backgroundLayer
                                    ? _ui.backgroundManager.backgroundLayer.backgroundColor
                                    : nil;
      styles[kBackgroundColorStr] =
          viewPresentLayerColor
              ? (__bridge id)viewPresentLayerColor
              : (viewLayerColor ? (__bridge id)viewLayerColor : (__bridge id)clearColor);
      if (_ui.backgroundManager.backgroundLayer != nil) {
        styles[kExtraBackgroundColorStr] =
            bgPresentLayerColor
                ? (__bridge id)bgPresentLayerColor
                : (bgLayerColor ? (__bridge id)bgLayerColor : (__bridge id)clearColor);
      }
    }
  }
  return styles;
}

- (void)restoreLayerStyles:(NSDictionary*)styles {
  if (!styles) {
    return;
  }
  NSArray<NSString*>* keys = [_keyframeParsedData.keyframeValues allKeys];
  for (NSString* key in keys) {
    if (![styles objectForKey:key] && [[styles objectForKey:key] isEqual:[NSNull null]]) {
      continue;
    }
    if ([key isEqualToString:kTransformStr]) {
      CATransform3D transform3D = CATransform3DIdentity;
      if ([styles[kTransformStr] isKindOfClass:[NSArray<LynxTransformRaw*> class]]) {
        transform3D = [LynxConverter toCATransform3D:styles[kTransformStr] ui:_ui];

      } else if ([styles[kTransformStr] isKindOfClass:[NSValue class]]) {
        transform3D = [styles[kTransformStr] CATransform3DValue];
      }
      _ui.view.layer.transform = transform3D;
      _ui.backgroundManager.transform = transform3D;
      if (_ui.backgroundManager.backgroundLayer != nil) {
        _ui.backgroundManager.backgroundLayer.transform = transform3D;
      }
      if (_ui.backgroundManager.borderLayer != nil) {
        _ui.backgroundManager.borderLayer.transform = transform3D;
      }
    } else if ([key isEqualToString:kOpacityStr] &&
               [styles[kOpacityStr] isKindOfClass:[NSNumber class]]) {
      _ui.view.layer.opacity = [styles[kOpacityStr] floatValue];
      _ui.backgroundManager.opacity = [styles[kOpacityStr] floatValue];
    } else if ([key isEqualToString:kBackgroundColorStr]) {
      _ui.view.layer.backgroundColor = (__bridge CGColorRef)styles[kBackgroundColorStr];
      if (_ui.backgroundManager.backgroundLayer != nil &&
          [styles objectForKey:kExtraBackgroundColorStr]) {
        _ui.backgroundManager.backgroundLayer.backgroundColor =
            (__bridge CGColorRef)styles[kExtraBackgroundColorStr];
      }
    }
  }
  [_ui.view setNeedsDisplay];
}

// state can only be changed by these five functions(pause, resume, cancel, run, finish).
- (void)pause:(LynxAnimationInfo*)info {
  NSAssert(info.playState == LynxAnimationPlayStatePaused, @"info.playState must be paused");
  NSAssert(_state == LynxAnimationPlayStateRunning, @"_state must be running");

  _state = LynxKFAnimatorStatePaused;
  NSDictionary* layerStyles = [self recordPresentationLayerStyles];

  [self removeAllAnimationFromLayer:info];

  [self restoreLayerStyles:layerStyles];

  if (![self isAnimationExpired:info]) {
    [_pauseTimeHelper recordPauseTime];
  }
  _info = info;
}

- (void)resume:(LynxAnimationInfo*)info {
  NSAssert(info.playState == LynxAnimationPlayStateRunning, @"info.playState must be running");
  NSAssert(_state == LynxKFAnimatorStatePaused, @"_state must be paused");

  _state = LynxKFAnimatorStateRunning;
  NSArray<NSString*>* keys = [_internalAnimators allKeys];
  // transform's animator.beginTime must be earlier than transform.rotation.x's
  // animator.beginTime(=.=) So we sort keys here firstly to make transform ahead of
  // transform.rotation.x
  NSArray* sortedKeys = [keys sortedArrayUsingComparator:^NSComparisonResult(
                                  NSString* _Nonnull str1, NSString* _Nonnull str2) {
    return [str1 compare:str2 options:NSCaseInsensitiveSearch];
  }];

  CFTimeInterval pauseDuration = [_pauseTimeHelper getPauseDuration];
  _keyframeStartTime += pauseDuration;

  LynxAnimationDelegate* delegate = [_delegate copy];
  for (NSString* key in sortedKeys) {
    CAKeyframeAnimation* animator = _internalAnimators[key];
    if (animator == nil) {
      continue;
    }
    animator.beginTime = _keyframeStartTime + info.delay;
    if (delegate) {
      // We only need set delegate to one animator.
      animator.delegate = delegate;
      delegate = nil;
    }
    [self addAnimationToLayer:key name:info.name animator:animator];
  }

  _info = info;
}

- (void)cancel {
  // Cancel may be called externally anytime, so check state here.
  if (_state != LynxKFAnimatorStateRunning && _state != LynxKFAnimatorStatePaused) {
    return;
  }
  _state = LynxKFAnimatorStateCanceled;
}

- (void)run {
  NSAssert(_state == LynxKFAnimatorStateIdle || _state == LynxKFAnimatorStateCanceled ||
               _state == LynxKFAnimatorStateCanceledLegacy,
           @"_state must be idle or canceled");
  _state = LynxKFAnimatorStateRunning;
}

- (void)finish {
  NSAssert(_state == LynxKFAnimatorStateRunning || _state == LynxKFAnimatorStateIdle,
           @"_state must be running");
  [self removeAllAnimationFromLayer:_info];
  _internalAnimators = nil;
  // Restore layer's styles
  NSDictionary* endStyles;
  endStyles = [LynxAnimationInfo isFillModeRemoved:_info] ? _propertyOriginValue
                                                          : [self getKeyframeEndStyles];
  [self restoreLayerStyles:endStyles];
  _state = LynxKFAnimatorStateIdle;
}

#define SAVE_ROTATION_DATA(type, path_name)                                                        \
  [self prepareKFValuesAndTimesContainer:path_name];                                               \
  setRotationValues(currentRotation##type, currentMoment, previousRotation##type##Value,           \
                    previousRotation##type##KeyTimes,                                              \
                    _keyframeParsedData.keyframeValues[path_name],                                 \
                    _keyframeParsedData.keyframeTimes[path_name]);                                 \
  previousRotation##type##Value = currentRotation##type;                                           \
  previousRotation##type##KeyTimes = currentMoment;                                                \
  if ([currentMoment floatValue] == 0.0f) {                                                        \
    _keyframeParsedData.beginStyles[path_name] = [NSNumber numberWithFloat:currentRotation##type]; \
  } else if ([currentMoment floatValue] == 1.0f) {                                                 \
    _keyframeParsedData.endStyles[path_name] = [NSNumber numberWithFloat:currentRotation##type];   \
    ;                                                                                              \
  }

// adding rotation keyframes algorithm:
// For all n rotations, we have moment[1], moment[2]...moment[n] and rotation[1],
// rotation[2]...rotation[n].(in order) We need to insert rotations if the differences between
// rotation[i] and rotation[i+1] is bigger than half a circle. delta moment = moment[i+1] -
// moment[i], delta rotaion = rotation[i+1] - rotation[i] delta n= ceil(delta rotaion / 3), delta n
// means (delta n - 1) rotation(s) needs to be added 3rad is a little smaller than PI, and make
// every part inserted smaller than PI. add moment = delta moment / delta n, add rotation = delta
// rotaion / delta n moment = moment[i] + add moment * k, rotation = rotaion[i] + add rotaion * k, 1
// <= k < delta n
void setRotationValues(CGFloat currentRotation, NSNumber* currentMoment, CGFloat previousRotation,
                       NSNumber* previousMoment, NSMutableArray<NSNumber*>* values,
                       NSMutableArray<NSNumber*>* moments) {
  if (previousMoment) {
    CGFloat deltaRotation = currentRotation - previousRotation;
    CGFloat deltaMoment = [currentMoment floatValue] - [previousMoment floatValue];
    NSInteger deltaN = ceil(ABS(deltaRotation / 3));
    deltaRotation = deltaRotation / deltaN;
    deltaMoment = deltaMoment / deltaN;
    for (int i = 1; i <= deltaN; ++i) {
      CGFloat iRotation = previousRotation + i * deltaRotation;
      CGFloat iMoment = [previousMoment floatValue] + i * deltaMoment;
      [values addObject:[NSNumber numberWithFloat:iRotation]];
      [moments addObject:[NSNumber numberWithFloat:iMoment]];
    }
  } else {
    [values addObject:[NSNumber numberWithFloat:currentRotation]];
    [moments addObject:currentMoment];
  }
}

- (void)prepareKFValuesAndTimesContainer:(NSString*)key {
  if ([_keyframeParsedData.keyframeValues objectForKey:key] == nil) {
    _keyframeParsedData.keyframeValues[key] = [[NSMutableArray alloc] init];
  }
  if ([_keyframeParsedData.keyframeTimes objectForKey:key] == nil) {
    _keyframeParsedData.keyframeTimes[key] = [[NSMutableArray alloc] init];
  }
}

// TODO: ugly code, will remove in release/2.2
- (BOOL)shouldPerformRotateZInMatrix:(LynxAnimationInfo*)info {
  LynxKeyframes* keyframes = [_ui.context.keyframesDict objectForKey:info.name];
  NSArray<NSString*>* moments = [keyframes.styles allKeys];
  moments = [moments sortedArrayUsingComparator:^(NSString* number1, NSString* number2) {
    return [number1 floatValue] > [number2 floatValue] ? NSOrderedDescending : NSOrderedAscending;
  }];
  NSEnumerator* momentsEnumerator = [LynxAnimationInfo isDirectionReverse:info]
                                        ? [moments reverseObjectEnumerator]
                                        : [moments objectEnumerator];
  NSNumber* currentMoment = nil;
  NSString* currentMomentStr = nil;
  __block CGFloat lastRotateZRad = 0;
  __block CGFloat newRotateZRad = 0;
  while (currentMomentStr = [momentsEnumerator nextObject]) {
    NSDictionary* currentStyle = [keyframes.styles objectForKey:currentMomentStr];
    currentMoment = [NSNumber numberWithFloat:[currentMomentStr floatValue]];

    // calc reverse currentMoment if direction is reversed
    if ([LynxAnimationInfo isDirectionReverse:info]) {
      currentMoment = [NSNumber numberWithFloat:1.0f - [currentMoment floatValue]];
    }

    [currentStyle enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL* stop) {
      if ([key isEqualToString:kTransformStr]) {
        NSArray<LynxTransformRaw*>* transformRawArr = [LynxTransformRaw toTransformRaw:obj];
        newRotateZRad = [LynxTransformRaw getRotateZRad:transformRawArr];
        *stop = YES;
      }
    }];
    if (newRotateZRad - lastRotateZRad >= M_PI) {
      return NO;
    }
    lastRotateZRad = newRotateZRad;
  }
  return YES;
}

- (void)parseKeyframes:(LynxAnimationInfo*)info {
  _keyframeParsedData = [[LynxKeyframeParsedData alloc] init];
  LynxKeyframes* keyframes = [_ui.context.keyframesDict objectForKey:info.name];
  NSArray<NSString*>* moments = [keyframes.styles allKeys];
  moments = [moments sortedArrayUsingComparator:^(NSString* number1, NSString* number2) {
    return [number1 floatValue] > [number2 floatValue] ? NSOrderedDescending : NSOrderedAscending;
  }];
  NSEnumerator* momentsEnumerator = [LynxAnimationInfo isDirectionReverse:info]
                                        ? [moments reverseObjectEnumerator]
                                        : [moments objectEnumerator];

  // rotation is different, previous status needs recording.
  __block char rotationType = LynxTransformRotationNone;
  __block CGFloat currentRotationX = 0;
  __block CGFloat currentRotationY = 0;
  __block CGFloat currentRotationZ = 0;
  __block CGFloat previousRotationXValue = 0;
  __block CGFloat previousRotationYValue = 0;
  __block CGFloat previousRotationZValue = 0;
  __block NSNumber* previousRotationXKeyTimes = nil;
  __block NSNumber* previousRotationYKeyTimes = nil;
  __block NSNumber* previousRotationZKeyTimes = nil;

  BOOL performRotateZInMatrix = [self shouldPerformRotateZInMatrix:info];

  NSNumber* currentMoment = nil;
  NSString* currentMomentStr = nil;
  while (currentMomentStr = [momentsEnumerator nextObject]) {
    NSDictionary* currentStyle = [keyframes.styles objectForKey:currentMomentStr];
    currentMoment = [NSNumber numberWithFloat:[currentMomentStr floatValue]];

    // calc reverse currentMoment if direction is reversed
    if ([LynxAnimationInfo isDirectionReverse:info]) {
      currentMoment = [NSNumber numberWithFloat:1.0f - [currentMoment floatValue]];
    }

    [currentStyle enumerateKeysAndObjectsUsingBlock:^(id key, id obj, BOOL* stop) {
      if ([key isEqualToString:kTransformStr]) {
        NSArray<LynxTransformRaw*>* transformRawArr = [LynxTransformRaw toTransformRaw:obj];
        if (transformRawArr != nil && [LynxTransformRaw hasPercent:transformRawArr]) {
          _keyframeParsedData.isPercentTransform = YES;
        }
        CATransform3D transformWithoutRotate = CATransform3DIdentity;
        CATransform3D transformWithoutRotateXY = CATransform3DIdentity;
        [LynxConverter toCATransform3D:transformRawArr
                                    ui:_ui
                              newFrame:_ui.updatedFrame
                transformWithoutRotate:&transformWithoutRotate
              transformWithoutRotateXY:&transformWithoutRotateXY
                          rotationType:&rotationType
                             rotationX:&currentRotationX
                             rotationY:&currentRotationY
                             rotationZ:&currentRotationZ];

        CATransform3D newTransformMatrix =
            performRotateZInMatrix ? transformWithoutRotateXY : transformWithoutRotate;

        [self prepareKFValuesAndTimesContainer:kTransformStr];
        [_keyframeParsedData.keyframeValues[kTransformStr]
            addObject:[NSValue valueWithCATransform3D:newTransformMatrix]];
        [_keyframeParsedData.keyframeTimes[kTransformStr] addObject:currentMoment];
        if ([currentMoment floatValue] == 0.0f) {
          _keyframeParsedData.beginStyles[kTransformStr] = transformRawArr;
        } else if ([currentMoment floatValue] == 1.0f) {
          _keyframeParsedData.endStyles[kTransformStr] = transformRawArr;
        }

        if (rotationType & LynxTransformRotationX) {
          SAVE_ROTATION_DATA(X, kTransformRotationXStr);
        } else {
          [self prepareKFValuesAndTimesContainer:kTransformRotationXStr];
          [_keyframeParsedData.keyframeValues[kTransformRotationXStr]
              addObject:[NSNumber numberWithFloat:0]];
          [_keyframeParsedData.keyframeTimes[kTransformRotationXStr] addObject:currentMoment];
          previousRotationXValue = 0;
          previousRotationXKeyTimes = currentMoment;
        }

        if (rotationType & LynxTransformRotationY) {
          SAVE_ROTATION_DATA(Y, kTransformRotationYStr);
        } else {
          [self prepareKFValuesAndTimesContainer:kTransformRotationYStr];
          [_keyframeParsedData.keyframeValues[kTransformRotationYStr]
              addObject:[NSNumber numberWithFloat:0]];
          [_keyframeParsedData.keyframeTimes[kTransformRotationYStr] addObject:currentMoment];
          previousRotationYValue = 0;
          previousRotationYKeyTimes = currentMoment;
        }

        if (!performRotateZInMatrix) {
          if (rotationType & LynxTransformRotationZ) {
            SAVE_ROTATION_DATA(Z, kTransformRotationZStr);
          } else {
            [self prepareKFValuesAndTimesContainer:kTransformRotationZStr];
            [_keyframeParsedData.keyframeValues[kTransformRotationZStr]
                addObject:[NSNumber numberWithFloat:0]];
            [_keyframeParsedData.keyframeTimes[kTransformRotationZStr] addObject:currentMoment];
            previousRotationZValue = 0;
            previousRotationZKeyTimes = currentMoment;
          }
        }
      } else if ([key isEqualToString:kOpacityStr]) {
        [self prepareKFValuesAndTimesContainer:kOpacityStr];
        NSNumber* opacity = [NSNumber numberWithFloat:[LynxConverter toCGFloat:obj]];
        [_keyframeParsedData.keyframeValues[kOpacityStr] addObject:opacity];
        [_keyframeParsedData.keyframeTimes[kOpacityStr] addObject:currentMoment];
        if ([currentMoment floatValue] == 0.0f) {
          _keyframeParsedData.beginStyles[kOpacityStr] = opacity;
        } else if ([currentMoment floatValue] == 1.0f) {
          _keyframeParsedData.endStyles[kOpacityStr] = opacity;
        }

      } else if ([key isEqualToString:@"background-color"]) {
        [self prepareKFValuesAndTimesContainer:kBackgroundColorStr];
        id color = (id)[LynxConverter toUIColor:obj].CGColor;
        [_keyframeParsedData.keyframeValues[kBackgroundColorStr] addObject:color];
        [_keyframeParsedData.keyframeTimes[kBackgroundColorStr] addObject:currentMoment];
        if ([currentMoment floatValue] == 0.0f) {
          _keyframeParsedData.beginStyles[kBackgroundColorStr] = color;
        } else if ([currentMoment floatValue] == 1.0f) {
          _keyframeParsedData.endStyles[kBackgroundColorStr] = color;
        }
      }
    }];
  }

  // Just need to recode origin value once.
  // Must record layer state here, because we may need the layer origin state as start and end
  // value.
  if (_propertyOriginValue == nil) {
    _propertyOriginValue = [self recordLayerStyles];
  }

  NSArray<NSString*>* keys = [_keyframeParsedData.keyframeTimes allKeys];
  for (NSString* key in keys) {
    if (![_propertyOriginValue objectForKey:key] ||
        [[_propertyOriginValue objectForKey:key] isEqual:[NSNull null]]) {
      continue;
    }
    id origin_value = _propertyOriginValue[key];
    if ([key isEqualToString:kTransformStr] &&
        [origin_value isKindOfClass:[NSArray<LynxTransformRaw*> class]]) {
      origin_value = [NSValue valueWithCATransform3D:[LynxConverter toCATransform3D:origin_value
                                                                                 ui:_ui]];
    }
    if (![_keyframeParsedData.beginStyles objectForKey:key]) {
      [_keyframeParsedData.keyframeValues[key] insertObject:origin_value atIndex:0];
      [_keyframeParsedData.keyframeTimes[key] insertObject:[NSNumber numberWithFloat:0.0f]
                                                   atIndex:0];
      _keyframeParsedData.beginStyles[key] = origin_value;
    }
    if (![_keyframeParsedData.endStyles objectForKey:key]) {
      [_keyframeParsedData.keyframeValues[key] addObject:origin_value];
      [_keyframeParsedData.keyframeTimes[key] addObject:[NSNumber numberWithFloat:1.0f]];
      _keyframeParsedData.endStyles[key] = origin_value;
    }
  }
}

- (void)applyAnimationInfo:(LynxAnimationInfo*)info {
  NSAssert(_state == LynxKFAnimatorStateIdle || _state == LynxKFAnimatorStateCanceled ||
               _state == LynxKFAnimatorStateCanceledLegacy,
           @"_state must be idle or canceled when call applyAnimationInfo()");

  if (info == nil) {
    return;
  }

  BOOL firstTimeApplied = (_keyframeStartTime == kTimeNotInit);

  // If the state is `canceled`, clear all old animators firstly.
  if (_state == LynxKFAnimatorStateCanceled) {
    [self removeAllAnimationFromLayer:_info];
    _internalAnimators = nil;
  }

  if (_keyframeParsedData == nil || [self shouldReInitTransform]) {
    [self parseKeyframes:info];
  }

  // We will use '_keyframeStartTime' to get play time later, so should update '_keyframeStartTime'
  // ahead here.
  if (_keyframeStartTime != kTimeNotInit && info.playState == LynxAnimationPlayStateRunning) {
    _keyframeStartTime += [_pauseTimeHelper getPauseDuration];
  }

  _internalAnimators = [[NSMutableDictionary alloc] init];
  NSArray<NSString*>* keys = [_keyframeParsedData.keyframeValues allKeys];
  // transform's animator.beginTime must be earlier than transform.rotation.x's
  // animator.beginTime(=.=) So we sort keys here firstly to make transform ahead of
  // transform.rotation.x
  NSArray* sortedKeys = [keys sortedArrayUsingComparator:^NSComparisonResult(
                                  NSString* _Nonnull str1, NSString* _Nonnull str2) {
    return [str1 compare:str2 options:NSCaseInsensitiveSearch];
  }];

  LynxAnimationDelegate* delegate = [_delegate copy];
  for (NSString* key in sortedKeys) {
    CAKeyframeAnimation* animator = [CAKeyframeAnimation animationWithKeyPath:key];
    animator.duration = info.duration;
    animator.timingFunction = info.timingFunction;
    animator.repeatCount =
        ([LynxAnimationInfo isDirectionAlternate:info] ? (info.iterationCount / 2)
                                                       : info.iterationCount);
    animator.fillMode = info.fillMode;
    animator.removedOnCompletion = NO;

    // mKeyframeStartTime only need to be updated once.
    if (_keyframeStartTime == kTimeNotInit) {
      _keyframeStartTime = [self.ui.view.layer convertTime:getSyncedTimestamp() fromLayer:nil];
    }

    animator.beginTime = _keyframeStartTime + info.delay;

    animator.autoreverses = [LynxAnimationInfo isDirectionAlternate:info];
    if (delegate) {
      // We only need set delegate to one animator.
      animator.delegate = delegate;
      delegate = nil;
    }
    animator.values = _keyframeParsedData.keyframeValues[key];
    animator.keyTimes = _keyframeParsedData.keyframeTimes[key];

    [self addAnimationToLayer:key name:info.name animator:animator];
    _internalAnimators[key] = animator;
  }

  if (![self isAnimationExpired:info]) {
    // Send animation start event
    // only send start event when state is IDLE.
    if (_state == LynxKFAnimatorStateIdle) {
      // Notify animation will start.
      [_ui.context.observer notifyAnimationStart];
      [LynxAnimationDelegate sendAnimationEvent:_ui
                                      eventName:(NSString*)kAnimationEventStart
                                    eventParams:@{
                                      @"animation_type" : @"keyframe-animation",
                                      @"animation_name" : info.name
                                    }];
    }

    // Change animator state
    [self run];
    if (info.playState == LynxAnimationPlayStatePaused) {
      // If the initial play state is paused, we should post the pause task to next runloop in order
      // to getting correct styles from presentation layer.
      if (firstTimeApplied) {
        __weak LynxKeyframeAnimator* weakSelf = self;
        dispatch_async(dispatch_get_main_queue(), ^{
          __strong LynxKeyframeAnimator* strongSelf = weakSelf;
          [strongSelf pause:strongSelf.info];
        });
      } else {
        [self pause:info];
      }
    }
  } else {
    // Special case in iOS. IOS may cancel animation sometimes, so we should try to resume animation
    // even if animation is expired when _autoResumeAnimation is enabled.
    if (_autoResumeAnimation && _state == LynxKFAnimatorStateCanceled) {
      [self run];
    }
  }

  _info = info;
}

- (void)addAnimationToLayer:(NSString*)key
                       name:(NSString*)name
                   animator:(CAKeyframeAnimation*)animator {
  if ([key isEqualToString:kOpacityStr]) {
    [self addOpacityAnimationToLayer:key name:name animator:animator];
    return;
  }
  // If two keyframes which have same property be applyed to a layer, we should give them a unique
  // key in layer. Otherwise they will be overwritten.
  NSString* animationUniqueKey = [key stringByAppendingString:name];

  [_ui.view.layer addAnimation:animator forKey:animationUniqueKey];

  // backgroundColor animation not need to be added to borderLayer and backgroundLayer. Otherwise
  // color animation will overflow out of border radius when the border radius is
  // complex(background's context is image).
  CAKeyframeAnimation* bgAnimator = [animator mutableCopy];
  if (![key isEqualToString:kBackgroundColorStr]) {
    // Animator in backgroundLayer and borderLayer do not need delegate.
    bgAnimator.delegate = nil;
    if (_ui.backgroundManager.backgroundLayer != nil) {
      [_ui.backgroundManager.backgroundLayer addAnimation:bgAnimator forKey:animationUniqueKey];
    }
    if (_ui.backgroundManager.borderLayer != nil) {
      [_ui.backgroundManager.borderLayer addAnimation:bgAnimator forKey:animationUniqueKey];
    }
  }
}

- (void)addOpacityAnimationToLayer:(NSString*)key
                              name:(NSString*)name
                          animator:(CAKeyframeAnimation*)animator {
  NSString* animationUniqueKey = [key stringByAppendingString:name];
  if (_ui.backgroundManager.opacityView) {
    // overlap rendering
    [_ui.backgroundManager.opacityView.layer addAnimation:animator forKey:animationUniqueKey];
  } else {
    // add animation for each layer
    [_ui.view.layer addAnimation:animator forKey:animationUniqueKey];
    CAKeyframeAnimation* bgAnimator = [animator mutableCopy];
    bgAnimator.delegate = nil;
    [_ui.backgroundManager addAnimation:bgAnimator forKey:animationUniqueKey];
  }
}

- (void)removeAllAnimationFromLayer:(LynxAnimationInfo*)info {
  if (info == nil) {
    return;
  }
  NSArray<NSString*>* keys = [_internalAnimators allKeys];
  for (NSString* key in keys) {
    // Remove action from internal not need to invoke delegate's didStop callback.
    ((LynxAnimationDelegate*)_internalAnimators[key].delegate).didStop = nil;
    NSString* animationUniqueKey = [key stringByAppendingString:info.name];
    [self.ui.view.layer removeAnimationForKey:animationUniqueKey];
    [_ui.backgroundManager removeAnimationForKey:animationUniqueKey];
    if ([key isEqualToString:kOpacityStr] && _ui.backgroundManager.opacityView) {
      [_ui.backgroundManager.opacityView.layer removeAnimationForKey:animationUniqueKey];
    }
  }
}

#undef SAVE_ROTATION_DATA

@end
