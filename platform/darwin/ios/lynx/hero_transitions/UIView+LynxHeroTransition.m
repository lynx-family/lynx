// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <objc/runtime.h>
#import "LynxAnimationInfo.h"
#import "LynxHeroTransition.h"
#import "UIView+LynxHeroTransition.h"

@implementation LynxHeroViewConfig

- (instancetype)initWithView:(UIView *)view {
  self = [super init];
  if (self) {
    _view = view;
    _crossPage = YES;
  }
  return self;
}

@end

@implementation UIView (LynxHeroTransition)

- (LynxHeroViewConfig *)lynxHeroConfig {
  LynxHeroViewConfig *config = objc_getAssociatedObject(self, @selector(lynxHeroConfig));
  if (!config) {
    config = [[LynxHeroViewConfig alloc] initWithView:self];
    objc_setAssociatedObject(self, @selector(lynxHeroConfig), config,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return config;
}

@end
