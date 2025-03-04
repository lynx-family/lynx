// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "LynxAnimationInfo.h"
#import "LynxHeroAnimator.h"
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxViewAnimFinishCallback)(void);

@interface LynxHeroTransition : NSObject <UIViewControllerTransitioningDelegate,
                                          UITabBarControllerDelegate,
                                          UINavigationControllerDelegate,
                                          UIViewControllerInteractiveTransitioning,
                                          UIViewControllerAnimatedTransitioning,
                                          LynxHeroAnimatorDelegate>

@property(nonatomic, nullable, readonly, weak) UIViewController *toViewController;
@property(nonatomic, nullable, readonly, weak) UIViewController *fromViewController;
@property(nonatomic, assign, readonly) BOOL isInNavigationController;
@property(nonatomic, assign, readonly) BOOL isPresenting;

+ (instancetype)sharedInstance;

- (void)executeEnterTransition:(LynxView *)lynxView;
- (void)executeExitTransition:(LynxView *)lynxView
               finishCallback:(LynxViewAnimFinishCallback)callback;
- (void)executePauseTransition:(LynxView *)lynxView;
- (void)executeResumeTransition:(LynxView *)lynxView;

- (void)registerEnterTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim;
- (void)registerExitTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim;
- (void)registerPauseTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim;
- (void)registerResumeTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim;
- (void)registerSharedElementsUI:(LynxUI *)lynxUI shareTag:(NSString *)tag;
- (void)registerSharedElements:(UIView *)view shareTag:(NSString *)tag;

- (void)unregisterLynxView:(LynxView *)lynxView;
@end

@interface ShareElementPair : NSObject

@property(nonatomic, nullable, weak) UIView *fromView;
@property(nonatomic, nullable, weak) UIView *toView;
@property(nonatomic, nullable, weak) UIView *originParent;
@property(nonatomic, assign) CGRect originFrame;
@property(nonatomic, assign) CGPoint originPoint;
@property(nonatomic, assign) CGRect originBounds;
@property(nonatomic, assign) NSTimeInterval duration;
@property(nonatomic, assign) BOOL crossPage;

- (instancetype)initWithFrom:(UIView *)from to:(UIView *)to;
- (void)backToOrigin;
@end

NS_ASSUME_NONNULL_END
