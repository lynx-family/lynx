// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxComponentRegistry.h"
#import "LynxConfig+Internal.h"
#import "LynxDefines.h"
#import "LynxEnv.h"
#import "LynxLog.h"

#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"

@implementation LynxConfig {
  std::shared_ptr<lynx::piper::ModuleFactoryDarwin> _moduleFactoryPtr;
}

LYNX_NOT_IMPLEMENTED(-(instancetype)init)

- (instancetype)initWithProvider:(id<LynxTemplateProvider>)provider {
  self = [super init];
  if (self) {
    _templateProvider = provider;
    _moduleFactoryPtr = std::make_shared<lynx::piper::ModuleFactoryDarwin>();
    _componentRegistry = [LynxComponentScopeRegistry new];
  }
  return self;
}

- (void)registerModule:(Class<LynxModule>)module {
  _moduleFactoryPtr->registerModule(module);
}

- (void)registerModule:(Class<LynxModule>)module param:(id)param {
  _moduleFactoryPtr->registerModule(module, param);
}

- (void)registerMethodAuth:(LynxMethodBlock)authBlock {
  _moduleFactoryPtr->registerMethodAuth(authBlock);
}

- (void)registerContext:(NSDictionary *)ctxDict sessionInfo:(LynxMethodSessionBlock)sessionInfo {
  if (!_contextDict) {
    _contextDict = [[NSMutableDictionary alloc] init];
  }
  [_contextDict addEntriesFromDictionary:ctxDict];
  _moduleFactoryPtr->registerExtraInfo(ctxDict);
  _moduleFactoryPtr->registerMethodSession(sessionInfo);
}

- (std::shared_ptr<lynx::piper::ModuleFactoryDarwin>)moduleFactoryPtr {
  return _moduleFactoryPtr;
}

- (void)registerUI:(Class)ui withName:(NSString *)name {
  [_componentRegistry registerUI:ui withName:name];
}

- (void)registerShadowNode:(Class)node withName:(NSString *)name {
  [_componentRegistry registerShadowNode:node withName:name];
}

+ (LynxConfig *)globalConfig {
  return [LynxEnv sharedInstance].config;
}

+ (void)prepareGlobalConfig:(LynxConfig *)config {
  [[LynxEnv sharedInstance] prepareConfig:config];
}

@end
