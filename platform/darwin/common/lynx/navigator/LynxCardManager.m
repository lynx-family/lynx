// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxCardManager.h"
#import "LynxBaseCardManager+Private.h"
#import "LynxHolder.h"
#import "LynxLruCache.h"
#import "LynxRoute.h"
#import "LynxView.h"
#if OS_IOS
#import "LynxHeroTransition.h"
#endif

@implementation LynxCardManager

@synthesize routeStack = _routeStack;
@synthesize lruCache = _lruCache;
@synthesize lynxView = _lynxView;
@synthesize holder = _holder;

- (void)hideLynxView:(LynxView*)lynxView {
#if OS_IOS
  if (lynxView) {
    [[LynxHeroTransition sharedInstance] executeExitTransition:lynxView
                                                finishCallback:^{
                                                  [self->_holder hideLynxView:lynxView];
                                                }];
    //        [_holder hideLynxView:lynxView];
  }
#endif
  [self invokeOnShow];
}

- (void)invokeOnShow {
  if ([_routeStack count] > 0) {
    LynxRoute* route = [_routeStack lastObject];
    LynxView* lynxView = [_lruCache objectForKey:route];
    if (lynxView) {
      if ([lynxView superview] == nil) {
        [_holder showLynxView:lynxView name:[route routeName]];
        // todo
        [lynxView onEnterForeground];
      } else {
        [self pauseTransition:lynxView];
        [lynxView onEnterForeground];
      }
    }
  } else {
    if (_lynxView) {
      [self pauseTransition:_lynxView];
      [_lynxView onEnterForeground];
    }
  }
}

- (void)invokeOnHide:(LynxView*)lynxView {
  if (lynxView) {
    [self pauseTransition:lynxView];
    [lynxView onEnterBackground];
  }
}

- (void)pauseTransition:(LynxView*)lynxView {
#if OS_IOS
  [[LynxHeroTransition sharedInstance] executePauseTransition:lynxView];
#endif
}

@end
