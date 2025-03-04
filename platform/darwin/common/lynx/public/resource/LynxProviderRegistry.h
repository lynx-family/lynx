// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXPROVIDERREGISTRY_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXPROVIDERREGISTRY_H_

#import <Foundation/Foundation.h>
#import "LynxResourceProvider.h"

NS_ASSUME_NONNULL_BEGIN

// type for lynx resource provider
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_IMAGE;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_FONT;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_LOTTIE;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_VIDEO;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_SVG;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_TEMPLATE;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_LYNX_CORE_JS;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_DYNAMIC_COMPONENT;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_I18N_TEXT;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_THEME;
FOUNDATION_EXPORT NSString* const LYNX_PROVIDER_TYPE_EXTERNAL_JS;

@interface LynxProviderRegistry : NSObject

- (void)addLynxResourceProvider:(NSString*)key provider:(id<LynxResourceProvider>)provider;

- (id<LynxResourceProvider>)getResourceProviderByKey:(NSString*)key;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXPROVIDERREGISTRY_H_
