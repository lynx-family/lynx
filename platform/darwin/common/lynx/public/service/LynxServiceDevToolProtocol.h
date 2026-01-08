// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_
#define DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_

#import <Foundation/Foundation.h>
#import <LynxServiceAPI/ServiceAPI.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxView;
@class LynxConfig;
@protocol LynxLogBoxProtocol;
@protocol LynxBaseInspectorOwner;
@protocol LynxBasePerfMonitor;
@protocol LynxContextModule;
@protocol LynxDebuggerProtocol;
@protocol LynxServiceDevToolProtocol <LynxServiceProtocol>

@required

// Preset boolean values indicating whether certain features are enabled by default.
//
// Note:
// - Services are registered during `LynxEnv` initialization, so these values should only be set
// after `LynxEnv` has been initialized.
// - Setting `lynxDebugPresetValue` to `YES` does not initialize `LynxDevToolEnv`. If you need to
// initialize `LynxDevToolEnv`, please call `[[LynxEnv sharedInstance] setLynxDebugEnabled: YES]`.
@property(nonatomic, readwrite) BOOL lynxDebugPresetValue;
@property(nonatomic, readwrite) BOOL logBoxPresetValue;

- (id<LynxBaseInspectorOwner>)createInspectorOwnerWithLynxView:(LynxView *)lynxView
                                                    debuggable:(BOOL)debuggable;

- (id<LynxLogBoxProtocol>)createLogBoxWithLynxView:(LynxView *)lynxView;

- (Class<LynxContextModule>)devtoolSetModuleClass;

- (Class<LynxContextModule>)lynxWebSocketModuleClass;

- (Class<LynxContextModule>)devtoolTrailModuleClass;

- (nullable Class<LynxBaseInspectorOwner>)inspectorOwnerClass;

- (Class<LynxDebuggerProtocol>)debuggerBridgeClass;

- (id)devtoolEnvSharedInstance;

- (void)devtoolEnvSetValue:(BOOL)value forKey:(NSString *)key;

- (BOOL)devtoolEnvGetValue:(NSString *)key withDefaultValue:(BOOL)value;

- (void)devtoolEnvSet:(NSSet *)newGroupValues forGroup:(NSString *)groupKey;

- (NSSet *)devtoolEnvGetGroup:(NSString *)groupKey;

- (void)enableAllSessions;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_
