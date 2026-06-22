// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxNodeAPIModule.h"
#include "LynxNodeAPI.h"

static NSMapTable<id, NSValue *> *s_envMap = nil;

@implementation LynxNodeAPIModule {
  __weak id _token;
}

+ (void)initialize {
  if (self == [LynxNodeAPIModule class]) {
    s_envMap = [NSMapTable weakToStrongObjectsMapTable];
  }
}

+ (NSString *)name {
  return @"LynxNodeAPI";
}

- (instancetype)initWithToken:(id)token {
  if (self = [super init]) {
    _token = token;
  }
  return self;
}

+ (void)putEnv:(void *)napiEnv forToken:(id)token {
  if (token && napiEnv) {
    @synchronized(s_envMap) {
      [s_envMap setObject:[NSValue valueWithPointer:napiEnv] forKey:token];
    }
  }
}

+ (void)removeEnvForToken:(id)token {
  if (token) {
    @synchronized(s_envMap) {
      [s_envMap removeObjectForKey:token];
    }
  }
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{@"requireNodeAddon" : NSStringFromSelector(@selector(requireNodeAddon:))};
}

- (void)requireNodeAddon:(NSString *)addonName {
  if (_token && addonName) {
    NSValue *val = nil;
    @synchronized(s_envMap) {
      val = [s_envMap objectForKey:_token];
    }
    if (val) {
      void *napiEnv = [val pointerValue];
      lynx::explorer::LynxNodeAPI::GetInstance().RequireNodeAddon(napiEnv, [addonName UTF8String]);
    }
  }
}

@end
