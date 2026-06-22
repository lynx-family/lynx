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
@protocol LynxBaseInspectorController;
@protocol LynxBasePerfMonitor;
@protocol LynxContextModule;

typedef void (^LynxOpenCardCallback)(NSString *);

@protocol LynxServiceDevToolProtocol <LynxServiceProtocol>

@required

// Deprecated compatibility APIs for bootstrap defaults.
// Use [DevToolSettings sharedInstance].bootstrap instead.
@property(nonatomic, readwrite) BOOL lynxDebugPresetValue
    __attribute__((deprecated("Use [DevToolSettings sharedInstance].bootstrap.lynxDebugEnabled")));
@property(nonatomic, readwrite) BOOL logBoxPresetValue
    __attribute__((deprecated("Use [DevToolSettings sharedInstance].bootstrap.logBoxEnabled")));

- (id<LynxBaseInspectorController>)createInspectorOwnerWithLynxView:(LynxView *)lynxView
                                                         debuggable:(BOOL)debuggable;

- (id<LynxLogBoxProtocol>)createLogBoxWithLynxView:(LynxView *)lynxView;

- (Class<LynxContextModule>)devtoolSetModuleClass;

- (Class<LynxContextModule>)lynxWebSocketModuleClass;

- (Class<LynxContextModule>)devtoolTrailModuleClass;

- (nullable Class<LynxBaseInspectorController>)inspectorOwnerClass;

- (BOOL)enable:(NSURL *)url withOptions:(NSDictionary *)options;

- (void)addOpenCardCallback:(LynxOpenCardCallback)callback;

- (BOOL)hasSetOpenCardCallback;

- (void)onPerfMetricsEvent:(NSString *)eventName
                  withData:(NSDictionary<NSString *, NSObject *> *)data
                instanceId:(int32_t)instanceId;

- (void)setAppInfo:(NSDictionary *)options;

- (id)devtoolEnvSharedInstance;

- (void)enableAllSessions;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_SERVICE_LYNXSERVICEIDEVTOOLPROTOCOL_H_
