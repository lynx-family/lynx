// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "JSModule+Internal.h"
#import "LynxDefines.h"

#include "core/value_wrapper/darwin/value_impl_darwin.h"

@implementation JSModule {
  std::weak_ptr<lynx::shell::JSProxyDarwin> proxy_;
}

LYNX_NOT_IMPLEMENTED(-(instancetype)init)

- (instancetype)initWithModuleName:(nonnull NSString*)moduleName {
  self = [super init];
  self.moduleName = moduleName;
  return self;
}

- (void)fire:(nonnull NSString*)methodName withParams:(NSArray*)args {
  auto proxy = proxy_.lock();
  if (proxy != nullptr) {
    proxy->CallJSFunction([self.moduleName UTF8String], [methodName UTF8String],
                          std::make_unique<lynx::pub::ValueImplDarwin>(args));
  }
}

- (void)setJSProxy:(const std::shared_ptr<lynx::shell::JSProxyDarwin>&)proxy {
  proxy_ = proxy;
}

@end
