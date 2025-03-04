// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxAnimationUtils.h"
#import "LynxAnimationInfo.h"

#import <LynxEnv+Internal.h>
#import <LynxEnv.h>

@implementation LynxAnimationUtils

const NSString* const DUP_ANI_PREFIX = @"DUP-";
const NSString* const DUP_CONTENT_ANI_PREFIX = @"DUP-CONTENT-";
const NSString* const kAnimationEventStart = @"animationstart";
const NSString* const kAnimationEventEnd = @"animationend";
const NSString* const kAnimationEventCancel = @"animationcancel";

+ (CABasicAnimation*)createBasicAnimation:(NSString*)path
                                     from:(id)fromValue
                                       to:(id)toValue
                                     info:(LynxAnimationInfo*)info {
  CABasicAnimation* c = [CABasicAnimation animationWithKeyPath:path];
  c.fromValue = fromValue;
  c.toValue = toValue;
  c.timingFunction = info.timingFunction;
  return c;
}

+ (void)applyAnimationProperties:(CAAnimation*)animation
                        withInfo:(LynxAnimationInfo*)info
                        forLayer:(CALayer*)layer {
  animation.duration = info.duration;
  [animation setBeginTime:[layer convertTime:getSyncedTimestamp() fromLayer:nil] + info.delay];
  [animation setRepeatCount:1];
  [animation setRemovedOnCompletion:NO];
  // Transition is a special keyframe animation.
  // We need to make sure that the fillmode of the transition animation is both, otherwise the
  // animation will be weird when its delay is not zero.
  [animation setFillMode:kCAFillModeBoth];
}

+ (void)removeAnimation:(LynxUI*)ui withName:(NSString*)animationKey {
  [ui.view.layer removeAnimationForKey:animationKey];
  [ui.backgroundManager
      removeAnimationForKey:[NSString stringWithFormat:@"%@%@", DUP_ANI_PREFIX, animationKey]];
  [ui.backgroundManager
      removeAnimationForKey:[NSString
                                stringWithFormat:@"%@%@", DUP_CONTENT_ANI_PREFIX, animationKey]];
  if ([animationKey isEqualToString:[LynxConverter toLynxPropName:OPACITY]]) {
    if (ui.backgroundManager.opacityView) {
      [ui.backgroundManager.opacityView.layer removeAnimationForKey:animationKey];
    }
  }
}

+ (void)attachTo:(LynxUI*)ui animation:(CAAnimation*)animation forKey:(NSString*)animationName {
  CAAnimation* bgAnimation = [animation mutableCopy];

  // layer animation with callback
  [animation setValue:animationName forKey:@"animationName"];
  [ui.view.layer addAnimation:animation forKey:animationName];

  // ba animation with no callback
  bgAnimation.delegate = nil;
  NSString* bgName = [NSString stringWithFormat:@"%@%@", DUP_ANI_PREFIX, animationName];
  [bgAnimation setValue:bgName forKey:@"animationName"];
  [ui.backgroundManager addAnimation:bgAnimation forKey:bgName];
}

+ (void)attachOpacityTo:(LynxUI*)ui
              animation:(CAAnimation*)animation
                 forKey:(NSString*)animationName {
  [animation setValue:animationName forKey:@"animationName"];
  if (ui.backgroundManager.opacityView) {
    [ui.backgroundManager.opacityView.layer addAnimation:animation forKey:animationName];
  } else {
    [ui.view.layer addAnimation:animation forKey:@"animationName"];
  }
}

+ (void)addContentAnimationDelegateTo:(CABasicAnimation*)contentAnimation
                       forTargetLayer:(CALayer*)targetLayer
                          withContent:(UIImage*)content
                             withProp:(LynxAnimationProp)prop {
  __weak CALayer* weakLayer = targetLayer;
  contentAnimation.delegate = [LynxAnimationDelegate
      initWithDidStart:^(CAAnimation* animation) {
      }
      didStop:^(CAAnimation* animation, BOOL finished) {
        __strong CALayer* strongLayer = weakLayer;
        if (!strongLayer) {
          return;
        }
        // We should remove content animation from layer when animation end. Otherwise the content
        // animation on the layer will make background property invalid.
        [strongLayer
            removeAnimationForKey:[NSString stringWithFormat:@"%@%@", DUP_CONTENT_ANI_PREFIX,
                                                             [LynxConverter toLynxPropName:prop]]];
        strongLayer.contents = (id)content.CGImage;
      }];
}

+ (void)addPathAnimationDelegateTo:(CABasicAnimation*)pathAnimation
                    forTargetLayer:(CAShapeLayer*)targetLayer
                          withPath:(CGPathRef)path
                          withProp:(LynxAnimationProp)prop {
  __weak CAShapeLayer* weakLayer = targetLayer;
  pathAnimation.delegate = [LynxAnimationDelegate
      initWithDidStart:^(CAAnimation* animation) {
      }
      didStop:^(CAAnimation* animation, BOOL finished) {
        __strong CAShapeLayer* strongLayer = weakLayer;
        if (!strongLayer) {
          return;
        }
        // We should remove path animation from layer when animation end. Otherwise the path
        // animation on the layer will make border property invalid.
        [strongLayer
            removeAnimationForKey:[NSString stringWithFormat:@"%@%@", DUP_CONTENT_ANI_PREFIX,
                                                             [LynxConverter toLynxPropName:prop]]];
        strongLayer.path = path;
        CGPathRelease(path);
      }];
}

@end

#pragma mark - SyncedTimeHelper
CFTimeInterval getSyncedTimestamp(void) {
  if ([LynxEnv sharedInstance].enableAnimationSyncTimeOpt) {
    return [[OptimizedSyncedTimeHelper sharedInstance] getSyncedTime];
  }
  return [[SyncedTimeHelper sharedInstance] getSyncedTime];
}

@implementation SyncedTimeHelper {
  CADisplayLink* _displayLink;
  CFTimeInterval _syncedTimestamp;
}

+ (instancetype)sharedInstance {
  static SyncedTimeHelper* sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(setSyncedTime:)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    _syncedTimestamp = CACurrentMediaTime();
  }
  return self;
}

- (CFTimeInterval)getSyncedTime {
  return _syncedTimestamp;
}

- (void)setSyncedTime:(CADisplayLink*)link {
  _syncedTimestamp = CACurrentMediaTime();
}
@end

static const CFTimeInterval kInitTimestamp = 0.0f;

@implementation OptimizedSyncedTimeHelper {
  CADisplayLink* _displayLink;
  CFTimeInterval _syncedTimestamp;
}

+ (instancetype)sharedInstance {
  static OptimizedSyncedTimeHelper* sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(clearSyncedTime:)];
    [_displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
    _displayLink.paused = true;
    _syncedTimestamp = kInitTimestamp;
  }
  return self;
}

- (CFTimeInterval)getSyncedTime {
  // If _syncedTimestamp is in its initial state, assign a value to it and request the next VSync.
  // In the next VSync callback, clear _syncedTimestamp. This way, all animations before the next
  // VSync will use the same synced timestamp.
  if (_syncedTimestamp == kInitTimestamp) {
    _syncedTimestamp = CACurrentMediaTime();
    _displayLink.paused = false;
  }
  return _syncedTimestamp;
}

- (void)clearSyncedTime:(CADisplayLink*)link {
  _syncedTimestamp = kInitTimestamp;
  _displayLink.paused = true;
}
@end
