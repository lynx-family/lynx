// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxBaseInspectorOwnerNG.h>

#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxDevToolNGDarwinDelegate : NSObject
- (instancetype)init;

- (int)getSessionId;

- (bool)isAttachToDebugRouter;

- (void)onBackgroundRuntimeCreated:(LynxBackgroundRuntime *)runtime
                   groupThreadName:(NSString *)groupThreadName;

- (void)onTemplateAssemblerCreated:(intptr_t)ptr;

- (int)attachToDebug:(NSString *)url;

- (void)detachToDebug;

- (void)setDevToolPlatformAbility:
    (std::shared_ptr<lynx::devtool::DevToolPlatformFacade>)devtool_platform_facade;

- (void)sendMessageToDebugPlatform:(NSString *)msg withType:(NSString *)type;

- (void)invokeCDPFromSDK:(NSString *)msg withCallback:(CDPResultCallback)callback;

- (void)subscribeMessage:(NSString *)type withHandler:(id<MessageHandler>)handler;

- (void)unsubscribeMessage:(NSString *)type;

@end

NS_ASSUME_NONNULL_END
