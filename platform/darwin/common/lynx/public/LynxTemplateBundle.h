// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_
#define DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_

#import <Lynx/LynxBytecodeResponseBlock.h>
#import <Lynx/LynxTemplateBundleOption.h>

/**
 * @apidoc
 * @brief `TemplateBundle` is the output product of the PreDecode capability
 * provided by the Lynx SDK. Client developers can parse the Lynx App Bundle product
 * in advance to obtain the `TemplateBundle` object and consume the App Bundle product.
 */
@interface LynxTemplateBundle : NSObject

@property(nonatomic, readonly, nullable) NSString* url;

/**
 * @apidoc
 * @brief Input Lynx template binary content and return the parsed `TemplateBundle` object.
 * @param tem Template binary content.
 * @return The `TemplateBundle` object.
 * @note When the input `tem` is not a correct `Lynx` template data, or is `nil`, an invalid
 * `TemplateBundle` is returned
 */
- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem;
- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem
                                    option:(nullable LynxTemplateBundleOption*)option;

/**
 * @apidoc
 * @brief When `TemplateBundle` is an invalid object, use this method to
 * obtain the exception information that occurred during template parsing
 * @return The exception information, if `nil` is returned, it proves that the `LynxTemplateBundle`
 * is normal
 */
- (NSString* _Nullable)errorMsg;

/**
 * @apidoc
 * @brief Read the content of the `extraInfo` field configured in the `pageConfig` of the front-end
 * template.
 *
 * @return When the front-end does not configure `extraInfo` or is called on an empty
 * `TemplateBundle` object, it returns `nil`, else it returns the `extraInfo` field.
 */
- (NSDictionary* _Nullable)extraInfo;

/**
 * @apidoc
 * @brief Whether the TemplateBundle contains a Valid ElementBundle.
 *
 * @return True if the TemplateBundle contains a Valid ElementBundle, otherwise false.
 */
- (BOOL)isElementBundleValid;

/**
 * @apidoc
 * @brief Start a sub-thread task to generate the `js code cache` of the current template.
 * @param bytecodeSourceUrl The source url of the template.
 */
- (void)postJsCacheGenerationTask:(nonnull NSString*)bytecodeSourceUrl;

/**
 * Post a task to generate bytecode for a given template bundle.
 * The task will be executed in a background thread.
 * @param bytecodeSourceUrl The source url of the template.
 * @param callback When generate finished, this will response the result.
 */
- (void)postJsCacheGenerationTask:(nonnull NSString*)bytecodeSourceUrl
                         callback:(nullable LynxBytecodeResponseBlock)callback;

@end

#endif  // DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_
