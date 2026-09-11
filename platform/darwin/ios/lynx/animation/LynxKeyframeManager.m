// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <Lynx/LynxKeyframeAnimator.h>
#import <Lynx/LynxKeyframeManager.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxUI.h>

#import <Lynx/LynxPlatformAnimation.h>

@implementation LynxKeyframeManager {
  NSArray<LynxAnimationInfo*>* _infos;
  NSMutableDictionary<NSString*, LynxKeyframeAnimator*>* _animators;
  NSMutableDictionary<NSNumber*, LynxKeyframeAnimator*>* _platformAnimators;
  NSMutableDictionary<NSNumber*, NSNumber*>* _platformGenerations;
  NSMutableArray<NSNumber*>* _platformAnimationOrder;
}

- (instancetype)initWithUI:(LynxUI*)ui {
  self = [super init];
  if (self) {
    _ui = ui;
    _infos = nil;
    _animators = nil;
    _platformAnimators = [[NSMutableDictionary alloc] init];
    _platformGenerations = [[NSMutableDictionary alloc] init];
    _platformAnimationOrder = [[NSMutableArray alloc] init];
    _autoResumeAnimation = YES;
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(resumeAnimation)
                                                 name:UIApplicationWillEnterForegroundNotification
                                               object:nil];
  }
  // When view has keyframe animation, we should disable CALayer implicit animation firstly.
  [_ui.backgroundManager setImplicitAnimation:NO];
  return self;
}

- (void)forEachAnimator:(void (^)(LynxKeyframeAnimator*))action {
  for (LynxKeyframeAnimator* animator in _animators.allValues) {
    action(animator);
  }
  for (NSNumber* key in [_platformAnimationOrder copy]) {
    LynxKeyframeAnimator* animator = _platformAnimators[key];
    if (animator != nil) {
      action(animator);
    }
  }
}

- (void)setAutoResumeAnimation:(BOOL)autoResume {
  _autoResumeAnimation = autoResume;
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    animator.autoResumeAnimation = autoResume;
  }];
}

- (void)applyAnimationInfo:(LynxAnimationInfo*)info
         keyframesProvider:(LynxTypedKeyframesProvider)provider
            reuseKeyframes:(BOOL)reuseKeyframes
               animationID:(uint64_t)animationID
                generation:(uint32_t)generation
                    cancel:(BOOL)cancel {
  if (_platformAnimators == nil) {
    _platformAnimators = [[NSMutableDictionary alloc] init];
    _platformGenerations = [[NSMutableDictionary alloc] init];
    _platformAnimationOrder = [[NSMutableArray alloc] init];
  }
  NSNumber* key = @(animationID);
  NSNumber* currentGeneration = _platformGenerations[key];
  if (currentGeneration != nil && generation < currentGeneration.unsignedIntValue) {
    return;
  }

  LynxKeyframeAnimator* animator = _platformAnimators[key];
  if (cancel) {
    [animator destroy];
    [_platformAnimators removeObjectForKey:key];
    [_platformAnimationOrder removeObject:key];
    _platformGenerations[key] = @(generation);
    return;
  }
  if (currentGeneration != nil && generation == currentGeneration.unsignedIntValue) {
    return;
  }
  if (animator == nil) {
    animator = [[LynxKeyframeAnimator alloc] initWithUI:_ui];
    animator.autoResumeAnimation = _autoResumeAnimation;
    _platformAnimators[key] = animator;
  }
  _platformGenerations[key] = @(generation);
  [_platformAnimationOrder removeObject:key];
  [_platformAnimationOrder addObject:key];
  [animator applyAnimationInfo:info
             keyframesProvider:provider
                reuseKeyframes:reuseKeyframes
                    generation:generation];
}

- (void)setAnimations:(NSArray<LynxAnimationInfo*>*)infos {
  _infos = infos;
}

- (void)setAnimation:(LynxAnimationInfo*)info {
  _infos = @[ info ];
}
- (void)notifyAnimationUpdated {
  for (NSNumber* key in [_platformAnimationOrder copy]) {
    [_platformAnimators[key] reapply];
  }
  if (_infos == nil || (_ui.frame.size.height == 0 && _ui.frame.size.width == 0)) {
    return;
  }
  NSMutableDictionary<NSString*, LynxKeyframeAnimator*>* animators =
      [[NSMutableDictionary alloc] init];

  for (LynxAnimationInfo* info in _infos) {
    if (info == nil || info.name == nil || [info.name isEqualToString:@""]) {
      continue;
    }
    LynxKeyframeAnimator* animator = (_animators != nil ? _animators[info.name] : nil);
    if (animator == nil) {
      animator = [[LynxKeyframeAnimator alloc] initWithUI:_ui];
      animator.autoResumeAnimation = _autoResumeAnimation;
    } else {
      [_animators removeObjectForKey:info.name];
    }
    animators[info.name] = animator;
  }

  // Should destroy all animators that be removed by user firstly, then apply info to other
  // animators.
  if (_animators != nil) {
    [_animators
        enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
          [animator destroy];
        }];
  }

  // Should ensure that animators apply info in order.
  for (LynxAnimationInfo* info in _infos) {
    if (info == nil || info.name == nil || [info.name isEqualToString:@""]) {
      continue;
    }
    [animators[info.name] apply:info];
  }

  _animators = animators;
}

- (void)notifyBGLayerAdded {
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    [animator notifyBGLayerAdded];
  }];
}

- (void)notifyPropertyUpdated:(NSString*)name value:(id)value {
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    [animator notifyPropertyUpdated:name value:value];
  }];
}

- (void)endAllAnimation {
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    [animator destroy];
  }];
  _animators = nil;
  _infos = nil;
  _platformAnimators = nil;
  _platformGenerations = nil;
  _platformAnimationOrder = nil;
}

// Only use for list to reset cell keyframe animation when it prepare for reusing cell.
- (void)resetAnimation {
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    [animator cancel];
  }];
  // Keep both input forms so a reused cell can restart its animations.
}

// Only use for list to restart cell keyframe animation when it reuse cell successful.
- (void)restartAnimation {
  // Make sure animation has been reset.
  [self resetAnimation];
  [self notifyAnimationUpdated];
}

- (void)resumeAnimation {
  if (_autoResumeAnimation) {
    [self notifyAnimationUpdated];
  }
}

- (BOOL)hasAnimationRunning {
  __block BOOL running = NO;
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    running |= [animator isRunning];
  }];
  return running;
}

- (void)detachFromUI {
  _ui = nil;
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    [animator detachFromUI];
  }];
}

- (void)attachToUI:(LynxUI*)ui {
  _ui = ui;
  [self forEachAnimator:^(LynxKeyframeAnimator* animator) {
    [animator attachToUI:ui];
  }];
}

@end
