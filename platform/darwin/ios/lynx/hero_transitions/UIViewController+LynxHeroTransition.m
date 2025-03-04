// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "LynxHeroTransition.h"
#import "UIViewController+LynxHeroTransition.h"

@implementation LynxHeroViewControllerConfig

- (instancetype)initWithVC:(UIViewController *)vc {
  self = [super init];
  if (self) {
    _vc = vc;
  }
  return self;
}

- (void)setEnableHeroTransition:(BOOL)enable {
  _enableHeroTransition = enable;
  if (enable) {
    if (_vc.transitioningDelegate == [LynxHeroTransition sharedInstance]) {
      return;
    }
    _vc.transitioningDelegate = [LynxHeroTransition sharedInstance];
    if ([_vc isKindOfClass:UINavigationController.class]) {
      self.previousNavigationDelegate = ((UINavigationController *)_vc).delegate;
      ((UINavigationController *)_vc).delegate = [LynxHeroTransition sharedInstance];
    } else if ([_vc isKindOfClass:UITabBarController.class]) {
      self.previousTabBarDelegate = ((UITabBarController *)_vc).delegate;
      ((UITabBarController *)_vc).delegate = [LynxHeroTransition sharedInstance];
    }
  } else {
    _vc.transitioningDelegate = nil;
    if ([_vc isKindOfClass:UINavigationController.class]) {
      ((UINavigationController *)_vc).delegate = self.previousNavigationDelegate;
    } else if ([_vc isKindOfClass:UITabBarController.class]) {
      ((UITabBarController *)_vc).delegate = self.previousTabBarDelegate;
    }
  }
}

@end

@implementation UIViewController (LynxHeroTransition)

- (LynxHeroViewControllerConfig *)lynxHeroConfig {
  LynxHeroViewControllerConfig *config = objc_getAssociatedObject(self, @selector(lynxHeroConfig));
  if (!config) {
    config = [[LynxHeroViewControllerConfig alloc] initWithVC:self];
    objc_setAssociatedObject(self, @selector(lynxHeroConfig), config,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return config;
}

@end
