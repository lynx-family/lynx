// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWBUILDER_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWBUILDER_H_

#import <Foundation/Foundation.h>

#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxBaseConfigurator.h>
#import <Lynx/LynxViewGroup.h>

/**
 * Key for set platform config to LynxViewConfig
 */
static NSString* _Nonnull const KEY_LYNX_PLATFORM_CONFIG = @"platform_config";

/**
 * Key for set image fetchUIImage to LynxViewConfig
 */
static NSString* _Nonnull const KEY_LYNX_ENABLE_FETCH_UIIMAGE = @"lynx_enable_fetch_UIImage";

/**
 * Key for set image preview hash metadata to LynxViewConfig
 */
static NSString* _Nonnull const KEY_LYNX_IMAGE_PREVIEW_HASH_METADATA =
    @"lynx_image_preview_hash_metadata";

@interface LynxViewBuilder : LynxBaseConfigurator

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
@property(nonatomic, nullable) LynxViewGroup* lynxViewGroup;
@property(nonatomic, nullable, strong) NSMutableDictionary<NSString*, id>* lynxViewConfig;

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
