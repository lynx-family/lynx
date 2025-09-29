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

@interface LynxViewBuilder : LynxBaseConfigurator

@property(nonatomic, nullable) LynxBackgroundRuntime* lynxBackgroundRuntime;
@property(nonatomic, nullable) LynxViewGroup* lynxViewGroup;

/**
 * Pass extra data to LynxModule, the usage of data depends on module's implementation
 */
@property(nonatomic, nullable) id lynxModuleExtraData;

@end

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXVIEWBUILDER_H_
