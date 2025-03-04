// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxHeroTransition.h"
#import "CALayer+LynxHeroTransition.h"
#import "UIView+LynxHeroTransition.h"
#import "UIViewController+LynxHeroTransition.h"

@interface LynxHeroTransition ()

@property(nonatomic, weak) id<UIViewControllerContextTransitioning> transitionContext;
@property(nonatomic, weak) UIView *containerView;
;

@end

@implementation LynxHeroTransition {
  LynxHeroAnimator *_heroAnimator;
  NSMutableArray<UIView *> *_toViewsWithSharedElements;
  NSMutableArray<UIView *> *_fromViewsWithSharedElements;
  NSMutableArray<UIView *> *_viewsWithEnterTransition;
  NSMutableArray<UIView *> *_viewsWithExitTransition;

  NSMapTable<LynxUI *, LynxAnimationInfo *> *_enterTransitionMap;
  NSMapTable<LynxUI *, LynxAnimationInfo *> *_exitTransitionMap;
  NSMapTable<LynxUI *, LynxAnimationInfo *> *_pauseTransitionMap;
  NSMapTable<LynxUI *, LynxAnimationInfo *> *_resumeTransitionMap;
  NSMapTable<LynxUI *, NSString *> *_sharedElementsMap;
  NSMapTable<UIView *, NSString *> *_sharedElementsViewMap;

  ShareElementPair *pair;
}

+ (instancetype)sharedInstance {
  static LynxHeroTransition *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxHeroTransition alloc] init];
  });

  return _instance;
}

#pragma mark UINavigationControllerDelegate

- (id<UIViewControllerAnimatedTransitioning>)
               navigationController:(UINavigationController *)navigationController
    animationControllerForOperation:(UINavigationControllerOperation)operation
                 fromViewController:(UIViewController *)fromVC
                   toViewController:(UIViewController *)toVC {
  _fromViewController = _fromViewController ? _fromViewController : fromVC;
  _toViewController = _toViewController ? _toViewController : toVC;
  _isInNavigationController = YES;
  _isPresenting = operation == UINavigationControllerOperationPush;
  return self;
}

- (void)navigationController:(UINavigationController *)navigationController
      willShowViewController:(UIViewController *)viewController
                    animated:(BOOL)animated {
  id<UINavigationControllerDelegate> previousNavigationDelegate =
      navigationController.lynxHeroConfig.previousNavigationDelegate;
  if (previousNavigationDelegate) {
    [previousNavigationDelegate navigationController:navigationController
                              willShowViewController:viewController
                                            animated:animated];
  }
}

- (void)navigationController:(UINavigationController *)navigationController
       didShowViewController:(UIViewController *)viewController
                    animated:(BOOL)animated {
  id<UINavigationControllerDelegate> previousNavigationDelegate =
      navigationController.lynxHeroConfig.previousNavigationDelegate;
  if (previousNavigationDelegate) {
    [previousNavigationDelegate navigationController:navigationController
                               didShowViewController:viewController
                                            animated:animated];
  }
}

- (id<UIViewControllerInteractiveTransitioning>)navigationController:
                                                    (UINavigationController *)navigationController
                         interactionControllerForAnimationController:
                             (id<UIViewControllerAnimatedTransitioning>)animationController {
  return nil;
}

#pragma mark UIViewControllerTransitioningDelegate

- (id<UIViewControllerAnimatedTransitioning>)
    animationControllerForPresentedController:(UIViewController *)presented
                         presentingController:(UIViewController *)presenting
                             sourceController:(UIViewController *)source {
  _fromViewController = _fromViewController ? _fromViewController : presenting;
  _toViewController = _toViewController ? _toViewController : presented;
  _isPresenting = YES;
  return self;
}

- (id<UIViewControllerAnimatedTransitioning>)animationControllerForDismissedController:
    (UIViewController *)dismissed {
  _fromViewController = _fromViewController ? _fromViewController : dismissed;
  _isPresenting = NO;
  return self;
}

- (id<UIViewControllerInteractiveTransitioning>)interactionControllerForDismissal:
    (id<UIViewControllerAnimatedTransitioning>)animator {
  // Todo, no interactive;
  return nil;
}

- (id<UIViewControllerInteractiveTransitioning>)interactionControllerForPresentation:
    (id<UIViewControllerAnimatedTransitioning>)animator {
  // Todo, no interactive;
  return nil;
}

#pragma mark UIViewControllerAnimatedTransitioning

- (void)prepareTransitionContainer {
  _fromViewController =
      _fromViewController
          ? _fromViewController
          : [_transitionContext viewControllerForKey:UITransitionContextFromViewControllerKey];
  _toViewController =
      _toViewController
          ? _toViewController
          : [_transitionContext viewControllerForKey:UITransitionContextToViewControllerKey];
  _containerView = _transitionContext.containerView;
  [_containerView addSubview:_toViewController.view];
  [_containerView setNeedsLayout];
  [_containerView layoutIfNeeded];
}

- (void)startAnimatorWithDuration:(double)duration {
  if (!_heroAnimator) {
    _heroAnimator = [LynxHeroAnimator new];
    _heroAnimator.delegate = self;
  }
  [_heroAnimator startWithTimePassed:0 totalTime:duration isReversed:!self.isPresenting];
}

- (void)registerEnterTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim {
  if (!_enterTransitionMap) {
    _enterTransitionMap = [NSMapTable weakToStrongObjectsMapTable];
  }
  [_enterTransitionMap setObject:anim forKey:lynxUI];
}

- (void)unregisterEnterTransition:(LynxUI *)lynxUI {
  if (_enterTransitionMap) {
    [_enterTransitionMap removeObjectForKey:lynxUI];
  }
}

- (void)executeEnterTransition:(LynxView *)lynxView {
  if (_enterTransitionMap) {
    for (LynxUI *lynxUI in _enterTransitionMap) {
      LynxAnimationInfo *anim = [_enterTransitionMap objectForKey:lynxUI];
      if (lynxUI.context.rootView == lynxView) {
        NSString *sharedElementName = lynxUI.view.lynxHeroConfig.sharedElementName;
        if (sharedElementName && [sharedElementName length] > 0) {
          [self applySharedElementEnter:sharedElementName LynxUI:lynxUI];
        } else {
          [lynxUI prepareKeyframeManager];
          [lynxUI.animationManager setAnimation:anim];
          [lynxUI.animationManager notifyAnimationUpdated];
        }
      }
    }
  }
}

- (void)registerExitTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim {
  if (!_exitTransitionMap) {
    _exitTransitionMap = [NSMapTable weakToStrongObjectsMapTable];
  }
  [_exitTransitionMap setObject:anim forKey:lynxUI];
}

- (void)unregisterExitTransition:(LynxUI *)lynxUI {
  if (_exitTransitionMap) {
    [_exitTransitionMap removeObjectForKey:lynxUI];
  }
}

- (void)executeExitTransition:(LynxView *)lynxView
               finishCallback:(LynxViewAnimFinishCallback)callback {
  NSTimeInterval animLength = 0;
  if (_exitTransitionMap) {
    for (LynxUI *lynxUI in _exitTransitionMap) {
      LynxAnimationInfo *anim = [_exitTransitionMap objectForKey:lynxUI];
      if (lynxUI.context.rootView == lynxView) {
        [lynxUI prepareKeyframeManager];
        [lynxUI.animationManager setAnimation:anim];
        NSTimeInterval duration = anim.duration;
        NSString *sharedElementName = lynxUI.view.lynxHeroConfig.sharedElementName;
        if (sharedElementName && [sharedElementName length] > 0) {
          [self applySharedElementExit:sharedElementName LynxUI:lynxUI];
        } else {
          [lynxUI prepareKeyframeManager];
          [lynxUI.animationManager setAnimation:anim];
          if (duration > animLength) {
            animLength = duration;
          }
          [lynxUI.animationManager notifyAnimationUpdated];
        }
      }
    }
    if (callback) {
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, animLength * NSEC_PER_SEC),
                     dispatch_get_main_queue(), ^{
                       callback();
                     });
    }
  }
}

- (void)registerPauseTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim {
  if (!_pauseTransitionMap) {
    _pauseTransitionMap = [NSMapTable weakToStrongObjectsMapTable];
  }
  [_pauseTransitionMap setObject:anim forKey:lynxUI];
}

- (void)unregisterPauseTransiiton:(LynxUI *)lynxUI {
  if (_pauseTransitionMap) {
    [_pauseTransitionMap removeObjectForKey:lynxUI];
  }
}

- (void)executePauseTransition:(LynxView *)lynxView {
  if (_pauseTransitionMap) {
    for (LynxUI *lynxUI in _pauseTransitionMap) {
      [lynxUI prepareKeyframeManager];
      LynxAnimationInfo *anim = [_pauseTransitionMap objectForKey:lynxUI];
      if (lynxUI.context.rootView == lynxView) {
        [lynxUI.animationManager setAnimation:anim];
        [lynxUI.animationManager notifyAnimationUpdated];
      }
    }
  }
}

- (void)registerResumeTransition:(LynxUI *)lynxUI anim:(LynxAnimationInfo *)anim {
  if (!_resumeTransitionMap) {
    _resumeTransitionMap = [NSMapTable weakToStrongObjectsMapTable];
  }
  [_resumeTransitionMap setObject:anim forKey:lynxUI];
}

- (void)unregisterResumeTransiiton:(LynxUI *)lynxUI {
  if (_resumeTransitionMap) {
    [_resumeTransitionMap removeObjectForKey:lynxUI];
  }
}

- (void)registerSharedElementsUI:(LynxUI *)lynxUI shareTag:(NSString *)tag {
  if (!_sharedElementsMap) {
    _sharedElementsMap = [NSMapTable new];
  }
  [_sharedElementsMap setObject:tag forKey:lynxUI];
}

- (void)registerSharedElements:(UIView *)view shareTag:(NSString *)tag {
  if (!_sharedElementsViewMap) {
    _sharedElementsViewMap = [NSMapTable weakToWeakObjectsMapTable];
  }
  [_sharedElementsViewMap setObject:tag forKey:view];
}

- (void)unregisterSharedElementsUI:(LynxUI *)lynxUI {
  if (_sharedElementsMap) {
    [_sharedElementsMap removeObjectForKey:lynxUI];
  }
}

- (void)unregisterLynxView:(LynxView *)lynxView {
  [self removeFromTransitionMap:_enterTransitionMap LynxView:lynxView];
  [self removeFromTransitionMap:_exitTransitionMap LynxView:lynxView];
  [self removeFromTransitionMap:_pauseTransitionMap LynxView:lynxView];
  [self removeFromTransitionMap:_resumeTransitionMap LynxView:lynxView];
}

- (void)removeFromTransitionMap:(NSMapTable<LynxUI *, LynxAnimationInfo *> *)map
                       LynxView:(LynxView *)lynxView {
  if (map) {
    NSMutableArray<LynxUI *> *viewTobeDeleted = [NSMutableArray new];
    for (LynxUI *lynxUI in map) {
      if (lynxUI.context.rootView == lynxView) {
        [viewTobeDeleted addObject:lynxUI];
      }
    }

    for (LynxUI *lynxUI in viewTobeDeleted) {
      [map removeObjectForKey:lynxUI];
    }
  }
}

- (void)executeResumeTransition:(LynxView *)lynxView {
  if (_resumeTransitionMap) {
    for (LynxUI *lynxUI in _resumeTransitionMap) {
      [lynxUI prepareKeyframeManager];
      LynxAnimationInfo *anim = [_resumeTransitionMap objectForKey:lynxUI];
      if (lynxUI.context.rootView == lynxView) {
        [lynxUI.animationManager setAnimation:anim];
        [lynxUI.animationManager notifyAnimationUpdated];
      }
    }
  }
}

- (void)animateTransition:(id<UIViewControllerContextTransitioning>)transitionContext {
  _transitionContext = transitionContext;
  [self animateTransition];
}

- (UIView *)getSharedElementByTag:(NSString *)shareTag LynxUI:(LynxUI *)lynxUI {
  // first find in native registed Map;
  for (UIView *tmp in _sharedElementsViewMap) {
    NSString *tag = [_sharedElementsViewMap objectForKey:tmp];
    if (tmp != [lynxUI view] && [shareTag isEqual:tag]) {
      return tmp;
    }
  }

  for (LynxUI *tmp in _sharedElementsMap) {
    NSString *tag = [_sharedElementsMap objectForKey:tmp];
    if (tmp != lynxUI && [shareTag isEqual:tag]) {
      return [tmp view];
    }
  }
  return nil;
}

- (void)applySharedElementEnter:(NSString *)shareTag LynxUI:(LynxUI *)lynxUI {
  UIView *fromView = [self getSharedElementByTag:shareTag LynxUI:lynxUI];
  UIView *toView = [lynxUI view];
  pair = [[ShareElementPair alloc] initWithFrom:fromView to:toView];
  pair.crossPage = lynxUI.view.lynxHeroConfig.crossPage;
  [lynxUI prepareKeyframeManager];
  [lynxUI.animationManager setAnimation:lynxUI.view.lynxHeroConfig.enterTransitionName];
  pair.duration = lynxUI.view.lynxHeroConfig.enterTransitionName.duration;
  if (!pair.crossPage) {
    UIWindow *window = [[UIApplication sharedApplication] keyWindow];
    CGPoint fromPoint = [window convertPoint:fromView.layer.position fromView:fromView.superview];
    [window addSubview:toView];
    toView.frame = fromView.frame;
    toView.layer.position = fromPoint;
    toView.layer.bounds = fromView.layer.bounds;
    [lynxUI.animationManager notifyAnimationUpdated];
    [self startAnimatorWithDuration:pair.duration];
  }
}

- (void)applySharedElementExit:(NSString *)shareTag LynxUI:(LynxUI *)lynxUI {
  UIView *fromView = [self getSharedElementByTag:shareTag LynxUI:lynxUI];
  UIView *toView = [lynxUI view];
  pair = [[ShareElementPair alloc] initWithFrom:fromView to:toView];
  pair.crossPage = lynxUI.view.lynxHeroConfig.crossPage;
  [lynxUI prepareKeyframeManager];
  [lynxUI.animationManager setAnimation:lynxUI.view.lynxHeroConfig.enterTransitionName];
  pair.duration = lynxUI.view.lynxHeroConfig.enterTransitionName.duration;
  if (!pair.crossPage) {
    UIWindow *window = [[UIApplication sharedApplication] keyWindow];
    CGPoint fromPoint = [window convertPoint:fromView.layer.position fromView:fromView.superview];
    [window addSubview:toView];
    toView.frame = fromView.frame;
    toView.layer.position = fromPoint;
    toView.layer.bounds = fromView.layer.bounds;
    [lynxUI.animationManager notifyAnimationUpdated];
    [self startAnimatorWithDuration:pair.duration];
  }
}

- (void)animateTransition {
  [self prepareTransitionContainer];
  if (!pair || !pair.crossPage) {
    [_transitionContext completeTransition:YES];
    return;
  }
  double duration = pair.duration;
  UIView *toView = pair.toView;
  UIView *fromView = pair.fromView;
  CGPoint fromPoint = [_containerView convertPoint:fromView.layer.position
                                          fromView:fromView.superview];
  CGSize fromSize = fromView.layer.bounds.size;
  CGRect fromBounds = fromView.layer.bounds;
  CGPoint toPoint = [_containerView convertPoint:pair.originPoint fromView:toView.superview];
  CGSize toSize = pair.originBounds.size;
  [_containerView addSubview:toView];
  if (_isPresenting) {
    toView.layer.position = fromPoint;
    toView.layer.bounds = fromBounds;
    CAKeyframeAnimation *kanim = [CAKeyframeAnimation animationWithKeyPath:@"position"];
    [kanim setValues:@[ [NSValue valueWithCGPoint:fromPoint], [NSValue valueWithCGPoint:toPoint] ]];
    kanim.duration = duration;
    kanim.fillMode = kCAFillModeBoth;
    kanim.removedOnCompletion = NO;
    [toView.layer addAnimation:kanim forKey:@"position"];
    CABasicAnimation *banim = [CABasicAnimation animationWithKeyPath:@"bounds.size"];
    banim.duration = duration;
    banim.fromValue = [NSValue valueWithCGSize:fromSize];
    banim.toValue = [NSValue valueWithCGSize:toSize];
    banim.fillMode = kCAFillModeBoth;
    banim.removedOnCompletion = NO;
    [toView.layer addAnimation:banim forKey:@"bounds.size"];
  } else {
    CAKeyframeAnimation *kanim = [CAKeyframeAnimation animationWithKeyPath:@"position"];
    [kanim setValues:@[ [NSValue valueWithCGPoint:toPoint], [NSValue valueWithCGPoint:fromPoint] ]];
    kanim.duration = duration;
    kanim.fillMode = kCAFillModeBoth;
    kanim.removedOnCompletion = NO;
    [toView.layer addAnimation:kanim forKey:@"position"];
    CABasicAnimation *banim = [CABasicAnimation animationWithKeyPath:@"bounds.size"];
    banim.duration = duration;
    banim.fromValue = [NSValue valueWithCGSize:toSize];
    banim.toValue = [NSValue valueWithCGSize:fromSize];
    banim.fillMode = kCAFillModeBoth;
    banim.removedOnCompletion = NO;
    [toView.layer addAnimation:banim forKey:@"bounds.size"];
  }

  [self startAnimatorWithDuration:duration];
}

- (BOOL)findViewsWithTransition {
  NSMutableDictionary<NSString *, UIView *> *totalFromViewsWithSharedElements =
      [NSMutableDictionary new];
  NSMutableDictionary<NSString *, UIView *> *totalToViewsWithSharedElements =
      [NSMutableDictionary new];
  [self findViewWithSharedElements:_toViewController.view
                      toDictionary:totalToViewsWithSharedElements];
  [self findViewWithSharedElements:_fromViewController.view
                      toDictionary:totalFromViewsWithSharedElements];
  _toViewsWithSharedElements = [NSMutableArray new];
  _fromViewsWithSharedElements = [NSMutableArray new];
  for (NSString *sharedElementKey in totalToViewsWithSharedElements) {
    if ([totalFromViewsWithSharedElements valueForKey:sharedElementKey]) {
      [_toViewsWithSharedElements
          addObject:[totalToViewsWithSharedElements valueForKey:sharedElementKey]];
      [_fromViewsWithSharedElements
          addObject:[totalFromViewsWithSharedElements valueForKey:sharedElementKey]];
    }
  }
  _viewsWithEnterTransition = [NSMutableArray new];
  _viewsWithExitTransition = [NSMutableArray new];
  [self findViewWithEnterTransition:_toViewController.view toArray:_viewsWithEnterTransition];
  [self findViewWithExitTransition:_fromViewController.view toArray:_viewsWithExitTransition];
  if (_fromViewsWithSharedElements.count == 0 && _viewsWithEnterTransition.count == 0 &&
      _viewsWithExitTransition.count == 0) {
    return NO;
  }
  return YES;
}

- (void)findViewWithSharedElements:(UIView *)parent
                      toDictionary:(NSMutableDictionary<NSString *, UIView *> *)dictionary {
  if (parent.lynxHeroConfig.sharedElementName) {
    [dictionary setValue:parent forKey:parent.lynxHeroConfig.sharedElementName];
  }
  for (UIView *child in parent.subviews) {
    [self findViewWithSharedElements:child toDictionary:dictionary];
  }
}

- (void)findViewWithEnterTransition:(UIView *)parent toArray:(NSMutableArray<UIView *> *)array {
  if (parent.lynxHeroConfig.enterTransitionName) {
    [array addObject:parent];
  }
  for (UIView *child in parent.subviews) {
    [self findViewWithEnterTransition:child toArray:array];
  }
}

- (void)findViewWithExitTransition:(UIView *)parent toArray:(NSMutableArray<UIView *> *)array {
  if (parent.lynxHeroConfig.exitTransitionName || parent.lynxHeroConfig.enterTransitionName) {
    [array addObject:parent];
  }
  for (UIView *child in parent.subviews) {
    [self findViewWithExitTransition:child toArray:array];
  }
}

- (NSTimeInterval)transitionDuration:(id<UIViewControllerContextTransitioning>)transitionContext {
  return 0.375;  // doesn't matter, real duration will be calculated later
}

- (void)animationEnded:(BOOL)transitionCompleted {
}

#pragma mark UIViewControllerInteractiveTransitioning
- (BOOL)wantsInteractiveStart {
  return NO;
}

- (void)startInteractiveTransition:(id<UIViewControllerContextTransitioning>)transitionContext {
  [self animateTransition:transitionContext];
}

#pragma mark LynxHeroAnimatorDelegate
- (void)updateProgress:(double)progress {
  if (_transitionContext) {
    [_transitionContext updateInteractiveTransition:progress];
  }
}

- (void)complete:(BOOL)finished {
  [pair backToOrigin];
  [_transitionContext completeTransition:YES];
}

@end

#pragma mark ShareElementPair
@implementation ShareElementPair

- (instancetype)initWithFrom:(UIView *)from to:(UIView *)to {
  self = [super init];
  if (self) {
    self.fromView = from;
    self.toView = to;
    self.originFrame = to.frame;
    self.originPoint = to.layer.position;
    self.originBounds = to.layer.bounds;
    self.originParent = to.superview;
  }
  return self;
}

- (void)backToOrigin {
  [_originParent addSubview:_toView];
  [_toView.layer removeAllAnimations];
  _toView.frame = _originFrame;
  _toView.layer.position = _originPoint;
}

@end
