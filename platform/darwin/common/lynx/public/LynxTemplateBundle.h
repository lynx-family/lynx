// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_
#define DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_

#import "LynxTemplateBundleOption.h"

@interface LynxTemplateBundle : NSObject

- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem;
- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem
                                    option:(nullable LynxTemplateBundleOption*)option;
- (NSString* _Nullable)errorMsg;

/**
 * get ExtraInfo of a `template.js`
 *
 * @return ExtraInfo of LynxTemplate
 */
- (NSDictionary* _Nullable)extraInfo;

/**
 * Whether the TemplateBundle contains a Valid ElementBundle.
 */
- (BOOL)isElementBundleValid;

/**
 * Post a task to generate bytecode for a given template bundle.
 * The task will be executed in a background thread.
 * @param bytecodeSourceUrl The source url of the template.
 */
- (void)postJsCacheGenerationTask:(nonnull NSString*)bytecodeSourceUrl;

@end

#endif  // DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_
