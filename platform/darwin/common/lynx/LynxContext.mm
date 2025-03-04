// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "JSModule+Internal.h"
#import "LynxContext+Internal.h"
#import "LynxExtensionModule.h"
#import "LynxLog.h"
#import "LynxSubErrorCode.h"
#import "LynxView+Internal.h"

NSString *const kDefaultComponentID = @"-1";

@interface LynxContext ()

@property(nonatomic, strong)
    NSMutableDictionary<NSString *, id<LynxExtensionModule>> *extentionModules;

@end

@implementation LynxContext

- (instancetype)initWithLynxView:(LynxView *)lynxView {
  if (self = [super init]) {
    _lynxView = lynxView;
    _instanceId = -1;
    _extentionModules = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)setJSProxy:(const std::shared_ptr<lynx::shell::JSProxyDarwin> &)proxy {
  proxy_ = proxy;
}

- (void)sendGlobalEvent:(nonnull NSString *)name withParams:(nullable NSArray *)params {
  auto eventEmitter = [self getJSModule:@"GlobalEventEmitter"];
  NSMutableArray *args = [[NSMutableArray alloc] init];
  // if name is nil, it will crash. To avoid crash, let name be @"";
  if (name == nil) {
    _LogW(@"Lynx sendGlobalEvent warning: name is nil");
    [args addObject:@""];
  } else {
    [args addObject:name];
  }
  // if params is nil, it will crash. To avoid crash, let params be [];
  if (params == nil) {
    _LogW(@"Lynx sendGlobalEvent warning: params is nil");
    [args addObject:[[NSArray alloc] init]];
  } else {
    [args addObject:params];
  }
  [eventEmitter fire:@"emit" withParams:args];
}

- (nullable JSModule *)getJSModule:(nonnull NSString *)name {
  auto module = [[JSModule alloc] initWithModuleName:name];
  [module setJSProxy:proxy_];
  return module;
}

- (NSNumber *)getLynxRuntimeId {
  if (proxy_) {
    return [NSNumber numberWithLongLong:proxy_->GetId()];
  }
  return @(-1);
}

// issue: #1510
- (void)reportModuleCustomError:(NSString *)message {
  [_lynxView.templateRender onErrorOccurred:ECLynxNativeModulesCustomError message:message];
}

- (nullable LynxView *)getLynxView {
  return _lynxView;
}

- (void)dealloc {
  _LogI(@"LynxContext destroy: %p", self);
}

- (void)runOnTasmThread:(dispatch_block_t)task {
  [_lynxView runOnTasmThread:task];
}

- (void)runOnJSThread:(dispatch_block_t)task {
  if (task && proxy_) {
    proxy_->RunOnJSThread(task);
  }
}

- (void)setExtensionModule:(nonnull id<LynxExtensionModule>)extensionModule
                    forKey:(nonnull NSString *)key {
  [_extentionModules setValue:extensionModule forKey:key];
}

- (nonnull id<LynxExtensionModule>)getExtensionModuleByKey:(nonnull NSString *)key {
  return [_extentionModules valueForKey:key];
}

@end
