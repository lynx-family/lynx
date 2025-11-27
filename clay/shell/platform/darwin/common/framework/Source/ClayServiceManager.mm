// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "ClayServiceManager.h"
#include "clay/fml/logging.h"

#import <Foundation/Foundation.h>

@implementation ClayServiceManager {
  NSMutableDictionary<NSString *, id> *_serviceMap;
}

- (instancetype)init {
  if (self = [super init]) {
    _serviceMap = [[NSMutableDictionary alloc] init];
  }
  return self;
}

+ (instancetype)sharedInstance {
  static ClayServiceManager *sharedInstance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

+ (void)registerGlobalService:(Protocol *)protocol service:(id)service {
  if (!protocol || !service || ![service conformsToProtocol:protocol]) {
    FML_LOG(ERROR) << "registerService failed: param error";
    return;
  }
  [[ClayServiceManager sharedInstance] registerService:protocol service:service];
}
- (void)registerService:(Protocol *)protocol service:(id)service {
  if (!protocol || !service || ![service conformsToProtocol:protocol]) {
    FML_LOG(ERROR) << "registerService failed: param error";
    return;
  }
  @synchronized(self) {
    [_serviceMap setObject:service forKey:NSStringFromProtocol(protocol)];
  }
}
- (id)getService:(Protocol *)protocol {
  id service = nil;
  @synchronized(self) {
    service = [_serviceMap objectForKey:NSStringFromProtocol(protocol)];
  }
  if (service) {
    return service;
  } else {
    if (self != [ClayServiceManager sharedInstance]) {
      return [[ClayServiceManager sharedInstance] getService:protocol];
    }
    return nil;
  }
}

@end
