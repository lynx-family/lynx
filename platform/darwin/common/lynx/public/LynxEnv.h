// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#import "LynxEnvKey.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxConfig;
@class LynxLifecycleDispatcher;
@protocol LynxViewLifecycle;
@protocol LynxResourceProvider;

/*!
 LynxEnv  can be reused for multiple LynxViews
*/
@interface LynxEnv : NSObject

@property(nonatomic, readonly) LynxConfig *config;
@property(nonatomic, readwrite) NSString *locale;
@property(nonatomic, readonly) LynxLifecycleDispatcher *lifecycleDispatcher;
@property(nonatomic, readonly)
    NSMutableDictionary<NSString *, id<LynxResourceProvider>> *resoureProviders;
@property(nonatomic, readwrite) BOOL lynxDebugEnabled;
/*!
 * mDevtoolComponentAttach: indicates whether DevTool Component is attached to the host.
 * mDevtoolEnabled: control whether to enable DevTool Debug
 *
 * eg:
 * when host client attach DevTool, mDevtoolComponentAttach is set true by reflection to find class
 * defined in DevTool and now if we set mDevtoolEnabled switch true, DevTool Debug is usable. if set
 * mDevtoolEnabled false, DevTool Debug is unavailable.
 *
 * when host client doesn't attach DevTool, can't find class defined in DevTool and
 * mDevtoolComponentAttach is set false in this case, no matter mDevtoolEnabled switch is set true
 * or false ,DevTool Debug is unavailable
 *
 * To sum up, mDevtoolComponentAttach indicates host package type, online package without DevTool or
 * localtest with DevTool mDevtoolEnabled switch is controlled by user to enable/disable DevTool
 * Debug, and useless is host doesn't attach DevTool
 */
@property(nonatomic, readonly) BOOL devtoolComponentAttach;
@property(nonatomic, readwrite) BOOL devtoolEnabled;
@property(nonatomic, readwrite) BOOL devtoolEnabledForDebuggableView;
@property(nonatomic, readwrite) BOOL redBoxEnabled
    __attribute__((deprecated("Please use logBoxEnabled to instead")));
@property(nonatomic, readwrite) BOOL logBoxEnabled;
@property(nonatomic, readwrite) BOOL highlightTouchEnabled;
@property(nonatomic, readwrite) BOOL automationEnabled;
@property(nonatomic, readwrite) BOOL layoutOnlyEnabled;
@property(nonatomic, readwrite) BOOL autoResumeAnimation;
@property(nonatomic, readwrite) BOOL enableNewTransformOrigin;
@property(nonatomic, readwrite) BOOL recordEnable;
@property(nonatomic, readwrite) BOOL launchRecordEnabled;

// use for ttnet by reject way .
@property(nonatomic, readwrite) void *cronetEngine;
// use for ttnet by reject way .
@property(nonatomic, readwrite) void *cronetServerConfig;

@property(nonatomic, readwrite) BOOL enableDevMenu
    __attribute__((deprecated("Use unified flag enableDevtoolDebug")));

@property(nonatomic, readwrite) BOOL enableJSDebug
    __attribute__((deprecated("Use unified flag enableDevtoolDebug")));

@property(nonatomic, readwrite) BOOL enableDevtoolDebug
    __attribute__((deprecated("Use devtoolEnabled")));

@property(nonatomic, readwrite) BOOL enableLogBox __attribute__((deprecated("Use logBoxEnabled")));

// values from settings
@property(nonatomic, readonly) BOOL switchRunloopThread;

+ (instancetype)sharedInstance;

- (void)prepareConfig:(LynxConfig *)config;
- (void)reportModuleCustomError:(NSString *)error;
- (void)onPiperInvoked:(NSString *)module
                method:(NSString *)method
              paramStr:(NSString *)paramStr
                   url:(NSString *)url
             sessionID:(NSString *)sessionID;
- (void)onPiperResponsed:(NSString *)module
                  method:(NSString *)method
                     url:(NSString *)url
                response:(NSDictionary *)response
               sessionID:(NSString *)sessionID;
- (void)updateSettings:(NSDictionary *)settings;
- (void)addResoureProvider:(NSString *)key provider:(id<LynxResourceProvider>)provider;

- (BOOL)boolFromExternalEnv:(LynxEnvKey)key defaultValue:(BOOL)defaultValue;
- (NSString *)stringFromExternalEnv:(LynxEnvKey)key;

- (void)setLocalEnv:(NSString *)value forKey:(NSString *)key;

- (void)setDevtoolEnv:(BOOL)value forKey:(NSString *)key;
- (BOOL)getDevtoolEnv:(NSString *)key withDefaultValue:(BOOL)value;

- (void)setDevtoolEnv:(NSSet *)newGroupValues forGroup:(NSString *)groupKey;
- (NSSet *)getDevtoolEnvWithGroupName:(NSString *)groupKey;

- (void)setEnableRadonCompatible:(BOOL)value
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3.")));
- (BOOL)getEnableRadonCompatible
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3.")));

- (void)setEnableLayoutOnly:(BOOL)value;
- (BOOL)getEnableLayoutOnly;

- (void)setPiperMonitorState:(BOOL)state;
- (void)initLayoutConfig:(CGSize)screenSize;

- (void)setAutoResumeAnimation:(BOOL)value;
- (BOOL)getAutoResumeAnimation;

- (void)setEnableNewTransformOrigin:(BOOL)value;
- (BOOL)getEnableNewTransformOrigin;

- (void)setCronetEngine:(void *)engine;
- (void)setCronetServerConfig:(void *)config;

- (void)enableFluencyTracer:(BOOL)value;

- (BOOL)enableComponentStatisticReport;

- (BOOL)enableImageEventReport;

- (BOOL)enableGenericResourceFetcher;

- (BOOL)enableTextContainerOpt;

- (NSDictionary<NSString *, NSString *> *)cppEnvDebugDescription;

- (NSDictionary<NSString *, NSString *> *)platformEnvDebugDescription;

/**
 * Get the version of the SSR API. You should always include the SSR API version when generating SSR
 *data with the SSR server, otherwise you may encounter compatibility issues.
 *
 *@return ssr api version.
 */
+ (NSString *_Nonnull)getSSRApiVersion;

/**
 * Clear bytecode for bytecodeSourceUrl.
 * When bytecodeSourceUrl is empty, that means clear all bytecode.
 */
+ (void)clearBytecode:(nonnull NSString *)bytecodeSourceUrl;
@end

NS_ASSUME_NONNULL_END
