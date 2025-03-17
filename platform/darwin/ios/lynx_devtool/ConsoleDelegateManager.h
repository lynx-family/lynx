// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>

#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"

NS_ASSUME_NONNULL_BEGIN

@interface ConsoleDelegateManager : NSObject

- (instancetype)initWithDevToolPlatformFacade:
    (const std::shared_ptr<lynx::devtool::DevToolPlatformFacade> &)facade;

- (void)setLynxInspectorConsoleDelegate:(id _Nonnull)delegate;

- (void)getConsoleObject:(NSString *)objectId
           needStringify:(BOOL)stringify
           resultHandler:(void (^)(NSString *detail))handler;

- (void)onConsoleMessage:(const std::string &)message;

- (void)onConsoleObject:(const std::string &)detail callbackId:(int)callbackId;

@end

NS_ASSUME_NONNULL_END
