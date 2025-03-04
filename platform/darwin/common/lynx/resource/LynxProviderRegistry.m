// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxProviderRegistry.h"

NSString* const LYNX_PROVIDER_TYPE_IMAGE = @"IMAGE";
NSString* const LYNX_PROVIDER_TYPE_FONT = @"FONT";
NSString* const LYNX_PROVIDER_TYPE_LOTTIE = @"LOTTIE";
NSString* const LYNX_PROVIDER_TYPE_VIDEO = @"VIDEO";
NSString* const LYNX_PROVIDER_TYPE_SVG = @"SVG";
NSString* const LYNX_PROVIDER_TYPE_TEMPLATE = @"TEMPLATE";
NSString* const LYNX_PROVIDER_TYPE_LYNX_CORE_JS = @"LYNX_CORE_JS";
NSString* const LYNX_PROVIDER_TYPE_DYNAMIC_COMPONENT = @"DYNAMIC_COMPONENT";
NSString* const LYNX_PROVIDER_TYPE_I18N_TEXT = @"I18N_TEXT";
NSString* const LYNX_PROVIDER_TYPE_THEME = @"THEME";
NSString* const LYNX_PROVIDER_TYPE_EXTERNAL_JS = @"EXTERNAL_JS_SOURCE";

/**
 * Lynx Resource Provider Manager
 */
@implementation LynxProviderRegistry {
  NSMutableDictionary<NSString*, id<LynxResourceProvider>>* _providers;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _providers = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)addLynxResourceProvider:(NSString*)key provider:(id<LynxResourceProvider>)provider {
  _providers[key] = provider;
}

- (id<LynxResourceProvider>)getResourceProviderByKey:(NSString*)key {
  return _providers[key];
}

@end
