// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWBUILDER_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWBUILDER_H_

#import <Foundation/Foundation.h>

#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxCustomVsyncMonitor.h>
#import <Lynx/LynxImageConfig.h>
#import <Lynx/LynxViewGroup.h>

/**
 * Key for set platform config to LynxViewConfig
 */
static NSString* _Nonnull const KEY_LYNX_PLATFORM_CONFIG = @"platform_config";

/**
 * Key for set image fetchUIImage to LynxViewConfig. Migrate to LynxBaseConfigurator, will be
 * deleted later.
 */
static NSString* _Nonnull const KEY_LYNX_ENABLE_FETCH_UIIMAGE = @"lynx_enable_fetch_UIImage";

@interface LynxViewBuilder : LynxBaseConfigurator

/**
 * Disables automatic warmup of the MTS runtime pool during render setup.
 * Defaults to NO.
 */
@property(nonatomic, assign) BOOL disableMTSPoolWarmup;

// legacy props that needs to be removed.
@property(nonatomic, assign) BOOL enableRadonCompatible;
@property(nonatomic, nullable) id<LynxDynamicComponentFetcher> fetcher;

/**
 * enable LynxResourceService loader injection
 */
@property(nonatomic, assign) BOOL enableLynxResourceServiceLoaderInjection;

@property(nonatomic, assign) BOOL isUIRunningMode __attribute__((deprecated(
    "try to set 'threadStrategy' variable if you want to change the thread strategy for rendering"))
);

@property(nonatomic, assign) CGRect frame;
@property(nonatomic, nullable) LynxBackgroundRuntime* lynxBackgroundRuntime;

/**
 * The frame source for this View and the runtime it creates. Nil uses the system source.
 * Only runtimes created by the View support a custom source. A pre-created
 * lynxBackgroundRuntime is not supported with custom VSync in this version.
 * Configure before building the View.
 * Lynx strongly retains the monitor while it is configured on the View.
 * Controls consumers that request Lynx VSync. Forced frames and independently clocked
 * platform animations keep their existing behavior.
 * Custom VSync drives Clay raster frames only with the asynchronous compositor.
 * Selecting a source does not change the compositor configuration.
 * Views reusing a Core engine must configure the same monitor instance, or all use nil.
 */
- (void)setCustomVsyncMonitor:(nullable id<LynxCustomVsyncMonitor>)monitor;

@property(nonatomic, nullable) LynxViewGroup* lynxViewGroup;
@property(nonatomic, nullable, strong) NSMutableDictionary<NSString*, id>* lynxViewConfig;

/**
 * Image configuration to be parsed by LynxUIImage when loading images.
 */
@property(nonatomic, nullable) LynxImageConfig* imageConfig;

/**
 * Pass extra data to LynxModule, the usage of data depends on module's implementation
 */
@property(nonatomic, nullable) id lynxModuleExtraData;

/**
 * URL for LynxView, which will be parsed by LynxServiceTrailProtocol in building LynxView.
 */
@property(nonatomic, strong, nullable) NSURL* uri;

/**
 * insert a key-value pair into lynxViewConfig if the key does not exist, otherwise ignored.
 */
- (void)insertLynxViewConfig:(id _Nonnull)config forKey:(NSString* _Nonnull)key;

@end

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWBUILDER_H_
