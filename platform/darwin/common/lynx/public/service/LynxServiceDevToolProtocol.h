// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_

#import <Foundation/Foundation.h>
#import "LynxServiceProtocol.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxView;

@protocol LynxBaseLogBoxProxy;
@protocol LynxBaseInspectorOwner;
@protocol LynxBasePerfMonitor;
@protocol LynxContextModule;
@protocol LynxDebuggerProtocol;
@protocol LynxServiceDevToolProtocol <LynxServiceProtocol>

@required

- (id<LynxBaseInspectorOwner>)createInspectorOwnerWithLynxView:(LynxView *)lynxView;

- (id<LynxBaseLogBoxProxy>)createLogBoxProxyWithLynxView:(LynxView *)lynxView;

- (Class<LynxContextModule>)devtoolSetModuleClass;

- (Class<LynxContextModule>)devtoolWebSocketModuleClass;

- (Class<LynxContextModule>)devtoolTrailModuleClass;

- (nullable Class<LynxBaseInspectorOwner>)inspectorOwnerClass;

- (Class<LynxDebuggerProtocol>)debuggerBridgeClass;

- (id)devtoolEnvSharedInstance;

- (void)devtoolEnvPrepareWithConfig:(LynxConfig *)lynxConfig;

- (void)devtoolEnvSetValue:(BOOL)value forKey:(NSString *)key;

- (BOOL)devtoolEnvGetValue:(NSString *)key withDefaultValue:(BOOL)value;

- (void)devtoolEnvSet:(NSSet *)newGroupValues forGroup:(NSString *)groupKey;

- (NSSet *)devtoolEnvGetGroup:(NSString *)groupKey;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_
