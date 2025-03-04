// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLayoutAnimationManager.h"
#import "LynxAnimationDelegate.h"
#import "LynxAnimationUtils.h"
#import "LynxUI+Internal.h"
#import "LynxUI.h"
#import "LynxUIScroller.h"

@interface LynxLayoutAnimationManager ()
@property(nonatomic, weak) LynxUI* ui;
@property(nonatomic, nullable)
    NSMutableDictionary<NSString*, LynxAnimationDelegate*>* animationDelegates;

- (BOOL)isCreate:(CGRect)oldFrame newFrame:(CGRect)newFrame;
- (BOOL)isUpdate:(CGRect)oldFrame newFrame:(CGRect)newFrame;
- (BOOL)isDelete:(CGRect)oldFrame newFrame:(CGRect)newFrame;

- (void)performCreateAnimationsWithFrame:(CGRect)newFrame;
- (void)performUpdateAnimationsWithFrame:(CGRect)newFrame;
- (void)performDeleteAnimationsWithFrame:(CGRect)newFrame;

- (void)forceStop:(LynxAnimationInfo*)info;
- (LynxAnimationDelegate*)createDelegate:(LynxAnimationInfo*)info;
- (void)addAnimation:(CAAnimation*)animation withInfo:(LynxAnimationInfo*)info;

@end

@implementation LynxLayoutAnimationManager

- (instancetype)initWithLynxUI:(LynxUI*)ui {
  if (self = [super init]) {
    _ui = ui;
  }
  return self;
}

- (LynxAnimationInfo*)createConfig {
  if (!_createConfig) {
    _createConfig = [[LynxAnimationInfo alloc] initWithName:@"layout-animation-create"];
    _createConfig.prop = OPACITY;
  }
  return _createConfig;
}

- (LynxAnimationInfo*)updateConfig {
  if (!_updateConfig) {
    _updateConfig = [[LynxAnimationInfo alloc] initWithName:@"layout-animation-update"];
    _updateConfig.prop = OPACITY;
  }
  return _updateConfig;
}

- (LynxAnimationInfo*)deleteConfig {
  if (!_deleteConfig) {
    _deleteConfig = [[LynxAnimationInfo alloc] initWithName:@"layout-animation-delete"];
    _deleteConfig.prop = OPACITY;
  }
  return _deleteConfig;
}

- (BOOL)isCreate:(CGRect)oldFrame newFrame:(CGRect)newFrame {
  return [self isConfigValid:_createConfig] && CGSizeEqualToSize(oldFrame.size, CGSizeZero) &&
         !CGSizeEqualToSize(newFrame.size, CGSizeZero);
}
- (BOOL)isUpdate:(CGRect)oldFrame newFrame:(CGRect)newFrame {
  return [self isConfigValid:_updateConfig] && !CGSizeEqualToSize(oldFrame.size, CGSizeZero) &&
         !CGSizeEqualToSize(newFrame.size, CGSizeZero);
}
- (BOOL)isDelete:(CGRect)oldFrame newFrame:(CGRect)newFrame {
  return [self isConfigValid:_deleteConfig] && !CGSizeEqualToSize(oldFrame.size, CGSizeZero) &&
         CGSizeEqualToSize(newFrame.size, CGSizeZero);
}

- (BOOL)isConfigValid:(LynxAnimationInfo*)config {
  return nil != config && config.duration > 0.001;
}

- (BOOL)maybeUpdateFrameWithLayoutAnimation:(CGRect)newFrame
                                withPadding:(UIEdgeInsets)padding
                                     border:(UIEdgeInsets)border
                                     margin:(UIEdgeInsets)margin {
  __weak LynxLayoutAnimationManager* weakSelf = self;
  if ([self isCreate:[_ui getPresentationLayer].frame newFrame:newFrame]) {
    CGRect startFrame = [_ui getPresentationLayer].frame;
    [_ui updateFrameWithoutLayoutAnimation:newFrame
                               withPadding:padding
                                    border:border
                                    margin:margin];
    CGFloat finalOpacity = _ui.view.layer.opacity;
    CATransform3D finalTransform = _ui.view.layer.transform;
    NSTimeInterval duration = [self createConfig].duration;
    [self createConfig].completeBlock = ^(BOOL finished) {
      __strong LynxLayoutAnimationManager* strongSelf = weakSelf;
      if (strongSelf) {
        strongSelf.ui.view.layer.opacity = finalOpacity;
        strongSelf.ui.view.layer.transform = finalTransform;
        [strongSelf.ui onAnimationEnd:@"create"
                           startFrame:startFrame
                           finalFrame:newFrame
                             duration:duration];
      }
      strongSelf = nil;
    };
    [_ui onAnimationStart:@"create" startFrame:startFrame finalFrame:newFrame duration:duration];
    [self performCreateAnimationsWithFrame:newFrame];
    return YES;
  } else if ([self isDelete:[_ui getPresentationLayer].frame newFrame:newFrame]) {
    CGFloat finalOpacity = _ui.view.layer.opacity;
    CATransform3D finalTransform = _ui.view.layer.transform;
    CGRect startFrame = [_ui getPresentationLayer].frame;
    NSTimeInterval duration = [self deleteConfig].duration;
    [self deleteConfig].completeBlock = ^(BOOL finished) {
      __strong LynxLayoutAnimationManager* strongSelf = weakSelf;
      if (strongSelf) {
        strongSelf.ui.view.layer.opacity = finalOpacity;
        strongSelf.ui.view.layer.transform = finalTransform;
        // All delete animations need to be removed, otherwise the next state will be incorrect.
        [strongSelf.ui.view.layer removeAllAnimations];
        [strongSelf.ui.backgroundManager removeAllAnimations];
        [strongSelf.ui updateFrameWithoutLayoutAnimation:newFrame
                                             withPadding:padding
                                                  border:border
                                                  margin:margin];
        [strongSelf.ui onAnimationEnd:@"delete"
                           startFrame:startFrame
                           finalFrame:newFrame
                             duration:duration];
      }
      strongSelf = nil;
    };
    [_ui onAnimationStart:@"delete" startFrame:startFrame finalFrame:newFrame duration:duration];
    [self performDeleteAnimationsWithFrame:newFrame];
    return YES;
  } else if ([self isUpdate:[_ui getPresentationLayer].frame newFrame:newFrame]) {
    CGRect startFrame = _ui.frame;
    NSTimeInterval duration = [self updateConfig].duration;
    [self updateConfig].completeBlock = ^(BOOL finished) {
      __strong LynxLayoutAnimationManager* strongSelf = weakSelf;
      if (strongSelf) {
        [strongSelf.ui updateFrameWithoutLayoutAnimation:newFrame
                                             withPadding:padding
                                                  border:border
                                                  margin:margin];
        [strongSelf.ui onAnimationEnd:@"update"
                           startFrame:startFrame
                           finalFrame:newFrame
                             duration:duration];
        // TODO: find a more general solution (suitable to other scrollable UIs such as
        // LynxUICollection/Swiper) to this problem
        if ([strongSelf.ui.parent isKindOfClass:[LynxUIScroller class]]) {
          [strongSelf.ui.parent layoutDidFinished];
        }
      }
      strongSelf = nil;
    };
    [_ui onAnimationStart:@"update" startFrame:startFrame finalFrame:newFrame duration:duration];
    [self performUpdateAnimationsWithFrame:newFrame];
    return YES;
  }
  return NO;
}

#pragma mark - Create

- (void)performCreateAnimationsWithFrame:(CGRect)newFrame {
  CAAnimationGroup* group = [[CAAnimationGroup alloc] init];
  NSMutableArray* animateArray = [[NSMutableArray alloc] init];
  LynxAnimationInfo* config = _createConfig;
  if (config.prop == OPACITY) {
    [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"opacity"
                                                                from:[NSNumber numberWithFloat:0.0]
                                                                  to:[NSNumber numberWithFloat:1.0]
                                                                info:config]];
  } else {
    id fromNumber = [NSNumber numberWithFloat:0.001];
    id toNumber = [NSNumber numberWithFloat:1.0];
    if (config.prop == SCALE_X) {
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.x"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
    } else if (config.prop == SCALE_Y) {
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.y"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
    } else if (config.prop == SCALE_XY) {
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.x"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.y"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
    }
  }

  [group setAnimations:animateArray];
  [self addAnimation:group withInfo:config];
}

#pragma mark - Update

- (void)performUpdateAnimationsWithFrame:(CGRect)newFrame {
  LynxAnimationInfo* config = _updateConfig;
  CAAnimationGroup* group = [[CAAnimationGroup alloc] init];
  CABasicAnimation* posAnimation = [LynxAnimationUtils
      createBasicAnimation:@"position"
                      from:[NSValue valueWithCGPoint:
                                        CGPointMake(
                                            [_ui getPresentationLayer].frame.origin.x +
                                                [_ui getPresentationLayer].frame.size.width / 2,
                                            [_ui getPresentationLayer].frame.origin.y +
                                                [_ui getPresentationLayer].frame.size.height / 2)]
                        to:[NSValue
                               valueWithCGPoint:CGPointMake(
                                                    newFrame.origin.x + newFrame.size.width / 2,
                                                    newFrame.origin.y + newFrame.size.height / 2)]
                      info:config];

  CABasicAnimation* boundsAnimation = [LynxAnimationUtils
      createBasicAnimation:@"bounds.size"
                      from:[NSValue
                               valueWithCGSize:CGSizeMake(
                                                   [_ui getPresentationLayer].frame.size.width,
                                                   [_ui getPresentationLayer].frame.size.height)]
                        to:[NSValue valueWithCGSize:CGSizeMake(newFrame.size.width,
                                                               newFrame.size.height)]
                      info:config];
  [group setAnimations:[NSArray arrayWithObjects:posAnimation, boundsAnimation, nil]];
  [self addAnimation:group withInfo:config];
}

#pragma mark - Delete

- (void)performDeleteAnimationsWithFrame:(CGRect)newFrame {
  LynxAnimationInfo* config = _deleteConfig;
  CAAnimationGroup* group = [[CAAnimationGroup alloc] init];
  NSMutableArray* animateArray = [[NSMutableArray alloc] init];
  if (config.prop == OPACITY) {
    [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"opacity"
                                                                from:[NSNumber numberWithFloat:1.0]
                                                                  to:[NSNumber numberWithFloat:0.0]
                                                                info:config]];
  } else {
    id fromNumber = [NSNumber numberWithFloat:1.0];
    id toNumber = [NSNumber numberWithFloat:0.001];
    if (config.prop == SCALE_X) {
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.x"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
    } else if (config.prop == SCALE_Y) {
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.y"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
    } else if (config.prop == SCALE_XY) {
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.x"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
      [animateArray addObject:[LynxAnimationUtils createBasicAnimation:@"transform.scale.y"
                                                                  from:fromNumber
                                                                    to:toNumber
                                                                  info:config]];
    }
  }

  [group setAnimations:animateArray];
  [self addAnimation:group withInfo:config];
}

#pragma mark - Utils

// Process all unfinished animation callbacks to prevent multiple repeated calls to the animations,
// which could cause state issues.
- (void)removeAllLayoutAnimation {
  [self forceStop:_createConfig];
  [self forceStop:_updateConfig];
  [self forceStop:_deleteConfig];
  [LynxAnimationUtils removeAnimation:_ui withName:_createConfig.name];
  [LynxAnimationUtils removeAnimation:_ui withName:_updateConfig.name];
  [LynxAnimationUtils removeAnimation:_ui withName:_deleteConfig.name];
}

- (void)forceStop:(LynxAnimationInfo*)info {
  if (info) {
    LynxAnimationDelegate* delegate = [_animationDelegates objectForKey:info.name];
    if (delegate) {
      [delegate forceStop];
      [_animationDelegates removeObjectForKey:info.name];
    }
  }
}

- (LynxAnimationDelegate*)createDelegate:(LynxAnimationInfo*)info {
  __weak LynxLayoutAnimationManager* weakSelf = self;
  return [LynxAnimationDelegate
      initWithDidStart:^(CAAnimation* animation) {
        __strong LynxLayoutAnimationManager* strongSelf = weakSelf;
        if ([strongSelf.ui.eventSet valueForKey:(NSString*)kAnimationEventStart]) {
          NSDictionary* dict = [NSDictionary dictionaryWithObject:info.name
                                                           forKey:@"animation_type"];
          [strongSelf.ui.context.eventEmitter
              dispatchCustomEvent:[[LynxCustomEvent alloc]
                                      initWithName:(NSString*)kAnimationEventStart
                                        targetSign:strongSelf.ui.sign
                                            params:dict]];
        }
      }
      didStop:^(CAAnimation* animation, BOOL finished) {
        if (info.completeBlock) {
          info.completeBlock(finished);
          info.completeBlock = nil;
          __strong LynxLayoutAnimationManager* strongSelf = weakSelf;
          if ([strongSelf.ui.eventSet valueForKey:(NSString*)kAnimationEventEnd]) {
            NSDictionary* dict =
                @{@"animation_type" : info.name, @"finished" : [NSNumber numberWithBool:finished]};
            [strongSelf.ui.context.eventEmitter
                dispatchCustomEvent:[[LynxCustomEvent alloc]
                                        initWithName:(NSString*)kAnimationEventEnd
                                          targetSign:strongSelf.ui.sign
                                              params:dict]];
          }
        }
      }];
}

- (void)addAnimation:(CAAnimation*)animation withInfo:(LynxAnimationInfo*)info {
  [LynxAnimationUtils applyAnimationProperties:animation withInfo:info forLayer:_ui.view.layer];
  LynxAnimationDelegate* delegate = [self createDelegate:info];
  [_animationDelegates setObject:delegate forKey:info.name];
  animation.delegate = delegate;
  [LynxAnimationUtils attachTo:_ui animation:animation forKey:info.name];
}

@end
