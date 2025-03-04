// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxView.h>
#include "core/shell/ios/js_proxy_darwin.h"

@class LynxUIIntersectionObserverManager;
@class LynxUIOwner;

@interface LynxContext () {
 @public
  std::shared_ptr<lynx::shell::JSProxyDarwin> proxy_;
}

@property(nonatomic, weak) LynxUIOwner* _Nullable uiOwner;
@property(nonatomic, weak) LynxUIIntersectionObserverManager* _Nullable intersectionManager;
@property(nonatomic, weak) LynxView* _Nullable lynxView;

// Generated in the LynxShell, id of template instance.
// instanceId is a value greater than or equal to 0, the initial value is -1.
@property(nonatomic, assign) int32_t instanceId;

- (nonnull instancetype)initWithLynxView:(LynxView* _Nullable)lynxView;
- (void)setJSProxy:(const std::shared_ptr<lynx::shell::JSProxyDarwin>&)proxy;

- (nullable NSDictionary*)extentionModules;

@end
