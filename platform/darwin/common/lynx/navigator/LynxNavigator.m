// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxNavigator.h"
#import "LynxCardManager.h"

@implementation LynxNavigator {
  NSMutableArray<LynxCardManager *> *_innerCardManagerStack;
}

+ (instancetype)sharedInstance {
  static LynxNavigator *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxNavigator alloc] init];
  });

  return _instance;
}

- (void)setCapacity:(NSInteger)capacity {
  self.capacity = capacity;
}

- (void)setSchemaInterceptor:(id<LynxSchemaInterceptor>)interceptor {
  self.interceptor = interceptor;
}

- (void)registerLynxHolder:(id<LynxHolder>)lynxHolder {
  [self registerLynxHolder:lynxHolder initLynxView:nil];
}

- (void)registerLynxHolder:(id<LynxHolder>)lynxHolder initLynxView:(LynxView *)lynxView {
  LynxCardManager *cardManager = [[LynxCardManager alloc] init:lynxHolder WithCapacity:_capacity];
  if (lynxView) {
    [cardManager registerInitLynxView:lynxView];
  }
  [_innerCardManagerStack addObject:cardManager];
}

- (void)unregisterLynxHolder:(id<LynxHolder>)lynxHolder {
  //  LynxCardManager *cardManager = [_pageDict objectForKey:lynxHolder];
  //  [_pageDict removeObjectForKey:lynxHolder];
  LynxCardManager *cardManager = [self getCurrentCardManager];
  if (cardManager) {
    [cardManager clearCaches];
    [_innerCardManagerStack removeObject:cardManager];
  }
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _capacity = NSIntegerMax;
    _innerCardManagerStack = [[NSMutableArray alloc] init];
  }
  return self;
}

- (void)navigate:(NSString *)name withParam:(NSDictionary *)param {
  if (_interceptor && [_interceptor intercept:name withParam:param]) {
    return;
  }
  LynxCardManager *cardManager = [self getCurrentCardManager];
  if (cardManager) {
    [cardManager push:name param:param];
  }
}

- (void)replace:(NSString *)name withParam:(NSDictionary *)param {
  LynxCardManager *cardManager = [self getCurrentCardManager];
  if (cardManager) {
    [cardManager replace:name param:param];
  }
}

- (void)goBack {
  LynxCardManager *cardManager = [self getCurrentCardManager];
  if (cardManager) {
    [cardManager pop];
  }
}

- (BOOL)onNavigateBack {
  LynxCardManager *cardManager = [self getCurrentCardManager];
  if (cardManager) {
    return [cardManager onNavigateBack];
  }
  return NO;
}

- (LynxCardManager *)getCurrentCardManager {
  return [_innerCardManagerStack lastObject];
}

- (void)didEnterForeground:(id<LynxHolder>)lynxHolder {
  [[self getCurrentCardManager] didEnterForeground];
}

- (void)didEnterBackground:(id<LynxHolder>)lynxHolder {
  [[self getCurrentCardManager] didEnterBackground];
}

@end
