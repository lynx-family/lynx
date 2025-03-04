// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxKeyframeManager.h"
#import "LynxKeyframeAnimator.h"
#import "LynxLog.h"
#import "LynxPropsProcessor.h"
#import "LynxUI.h"

@implementation LynxKeyframeManager {
  NSArray<LynxAnimationInfo*>* _infos;
  NSMutableDictionary<NSString*, LynxKeyframeAnimator*>* _animators;
}

- (instancetype)initWithUI:(LynxUI*)ui {
  self = [super init];
  if (self) {
    _ui = ui;
    _infos = nil;
    _animators = nil;
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

- (void)setAutoResumeAnimation:(BOOL)autoResume {
  _autoResumeAnimation = autoResume;
  if (_animators != nil) {
    [_animators
        enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
          animator.autoResumeAnimation = autoResume;
        }];
  }
}

- (void)setAnimations:(NSArray<LynxAnimationInfo*>*)infos {
  _infos = infos;
}

- (void)setAnimation:(LynxAnimationInfo*)info {
  _infos = @[ info ];
}
- (void)notifyAnimationUpdated {
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
  if (_animators == nil) {
    return;
  }
  [_animators
      enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
        [animator notifyBGLayerAdded];
      }];
}

- (void)notifyPropertyUpdated:(NSString*)name value:(id)value {
  if (_animators == nil) {
    return;
  }
  [_animators
      enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
        [animator notifyPropertyUpdated:name value:value];
      }];
}

- (void)endAllAnimation {
  if (_animators == nil) {
    return;
  }
  [_animators
      enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
        [animator destroy];
      }];
  _animators = nil;
  _infos = nil;
}

// Only use for list to reset cell keyframe animation when it prepare for reusing cell.
- (void)resetAnimation {
  if (_animators == nil) {
    return;
  }
  [_animators
      enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
        [animator cancel];
      }];
  // Don't reset _infos here, because list need _infos to restart animation later.
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
  if (_animators != nil) {
    NSArray<LynxKeyframeAnimator*>* allAnimators = [_animators allValues];
    for (LynxKeyframeAnimator* animator in allAnimators) {
      if ([animator isRunning]) {
        return YES;
      }
    }
  }
  return NO;
}

- (void)detachFromUI {
  _ui = nil;
  if (_animators == nil) {
    return;
  }
  [_animators
      enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
        [animator detachFromUI];
      }];
}

- (void)attachToUI:(LynxUI*)ui {
  _ui = ui;
  if (_animators == nil) {
    return;
  }
  [_animators
      enumerateKeysAndObjectsUsingBlock:^(id key, LynxKeyframeAnimator* animator, BOOL* stop) {
        [animator attachToUI:ui];
      }];
}

@end
