// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxBaseCardManager.h"
#import "LynxBaseCardManager+Private.h"
#import "LynxHolder.h"
#import "LynxLruCache.h"
#import "LynxRoute.h"
#import "LynxView.h"

@implementation LynxBaseCardManager

#pragma mark - Public

- (instancetype)init:(id)lynxHolder WithCapacity:(NSInteger)capacity {
  self = [super init];
  if (self) {
    _routeStack = [[NSMutableArray alloc] init];
    _holder = lynxHolder;
    __weak __typeof(self) weakSelf = self;
    _lruCache = [[LynxLruCache alloc] initWithCapacity:capacity
        recreate:^(LynxRoute* route) {
          __strong typeof(self) strongSelf = weakSelf;
          if (!strongSelf) {
            LynxView* lynxView = nil;
            return lynxView;
          }
          return [strongSelf buildLynxView:route];
        }
        viewEvicted:^(LynxView* lynxView) {
          __strong typeof(self) strongSelf = weakSelf;
          if (!strongSelf) {
            return;
          }
          [strongSelf->_holder hideLynxView:lynxView];
        }];
  }
  return self;
}

- (void)registerInitLynxView:(LynxView*)lynxView {
  _lynxView = lynxView;
}

- (void)push:(NSString*)templateUrl param:(NSDictionary*)param {
  LynxRoute* route = [[LynxRoute alloc] initWithUrl:templateUrl param:param];
  LynxView* lynxView = [self buildLynxView:route];
  if (lynxView) {
    if ([_routeStack count] > 0) {
      LynxRoute* previousRoute = [_routeStack lastObject];
      LynxView* previousLynxView = [_lruCache objectForKey:previousRoute];
      // TODO
      [self invokeOnHide:previousLynxView];
    } else {
      [self invokeOnHide:_lynxView];
    }
    [_routeStack addObject:route];
    [_holder showLynxView:lynxView name:templateUrl];
  }
}

- (void)replace:(NSString*)templateUrl param:(NSDictionary*)param {
  LynxRoute* route = [[LynxRoute alloc] initWithUrl:templateUrl param:param];
  LynxView* lynxView = [self buildLynxView:route];
  if (lynxView) {
    if ([_routeStack count] > 0) {
      LynxRoute* lastRoute = [_routeStack lastObject];
      [_routeStack removeLastObject];
      LynxView* previousLynxView = [_lruCache removeObjectForKey:lastRoute];
      [self hideLynxView:previousLynxView];
    } else {
      [self hideLynxView:_lynxView];
    }
    [_routeStack addObject:route];
    [_holder showLynxView:lynxView name:templateUrl];
  }
}

- (void)pop {
  if (_routeStack.count > 0) {
    LynxRoute* lastRoute = [_routeStack lastObject];
    [_routeStack removeLastObject];
    LynxView* lastView = [_lruCache removeObjectForKey:lastRoute];
    [self hideLynxView:lastView];
  }
}

- (BOOL)onNavigateBack {
  if (_routeStack.count > 0) {
    LynxRoute* lastRoute = [_routeStack lastObject];
    [_routeStack removeLastObject];
    LynxView* lastView = [_lruCache removeObjectForKey:lastRoute];
    [self hideLynxView:lastView];
    return YES;
  }
  return NO;
}

- (void)didEnterForeground {
  if ([_routeStack count] > 0) {
    LynxRoute* route = [_routeStack lastObject];
    LynxView* lynxView = [_lruCache objectForKey:route];
    if (lynxView) {
      [lynxView onEnterForeground];
    }
  } else {
    [_lynxView onEnterForeground];
  }
}

- (void)didEnterBackground {
  if ([_routeStack count] > 0) {
    LynxRoute* route = [_routeStack lastObject];
    LynxView* lynxView = [_lruCache objectForKey:route];
    if (lynxView) {
      [lynxView onEnterBackground];
    }
  } else {
    [_lynxView onEnterBackground];
  }
}

- (void)clearCaches {
  [_routeStack removeAllObjects];
  _holder = nil;
  _lruCache = nil;
  _routeStack = nil;
  _lynxView = nil;
}

#pragma mark - Private

- (LynxView*)buildLynxView:(LynxRoute*)lynxRoute {
  LynxView* lynxView = [_holder createLynxView:lynxRoute];
  if (lynxView) {
    [_lruCache setObject:lynxView forKey:lynxRoute];
  }
  return lynxView;
}

- (void)hideLynxView:(LynxView*)lynxView {
  // implemented by subclass
}

- (void)invokeOnShow {
  // implemented by subclass
}

- (void)invokeOnHide:(LynxView*)lynxView {
  // implemented by subclass
}

@end
