// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_
#define DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_H_

#import <Lynx/LynxBytecodeResponseBlock.h>
#import <Lynx/LynxResourceHandle.h>
#import <Lynx/LynxServiceSecurityProtocol.h>
#import <Lynx/LynxTemplateBundleOption.h>

/**
 * @apidoc
 * @brief `TemplateBundle` is the output product of the PreDecode capability
 * provided by the Lynx SDK. Client developers can parse the Lynx App Bundle product
 * in advance to obtain the `TemplateBundle` object and consume the App Bundle product.
 */
@interface LynxTemplateBundle : NSObject <LynxSecurityTarget>

@property(nonatomic, readonly, nullable) NSString* url;

/**
 * @apidoc
 * @brief Input Lynx template binary content and return the parsed `TemplateBundle` object.
 * @param tem Template binary content.
 * @return The `TemplateBundle` object.
 * @note When the input `tem` is not a correct `Lynx` template data, or is `nil`, an invalid
 * `TemplateBundle` is returned
 */
- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem NS_SWIFT_NAME(init(_:));
- (instancetype _Nullable)initWithTemplate:(nonnull NSData*)tem
                                    option:(nullable LynxTemplateBundleOption*)option
    NS_SWIFT_NAME(init(_:option:));
+ (instancetype _Nullable)_swiftTemplateBundleWithTemplate:(nonnull NSData*)tem
    NS_SWIFT_NAME(init(template:));
+ (instancetype _Nullable)_swiftTemplateBundleWithTemplate:(nonnull NSData*)tem
                                                    option:
                                                        (nullable LynxTemplateBundleOption*)option
    NS_SWIFT_NAME(init(template:option:));

/**
 * @apidoc
 * @brief Swift-oriented two-stage initializer entry for a `TemplateBundle` instance created by
 * `init`.
 * @param data Template binary content.
 * @return The current `TemplateBundle` object.
 */
- (instancetype _Nullable)_swiftInitWithData:(nonnull NSData*)data
    __attribute__((objc_method_family(none)))NS_SWIFT_NAME(initWith(_:));
- (instancetype _Nullable)_swiftInitWithData:(nonnull NSData*)data
                                      option:(nullable LynxTemplateBundleOption*)option
    __attribute__((objc_method_family(none)))NS_SWIFT_NAME(initWith(_:option:));

/**
 * @apidoc
 * @brief Reads and parses a Lynx Bundle from a reusable resource handle.
 * @param handle The resource handle that describes the Lynx Bundle.
 * @return The parsed `LynxTemplateBundle`, or `nil` when `handle` is `nil`.
 * @note The handle remains valid and reusable after this method returns.
 * @note The resource file path is used as the bundle URL.
 * @note A registered native C++ security service is required. If unavailable, the returned bundle
 * is invalid and contains a missing-service error.
 */
- (instancetype _Nullable)initWithResourceHandle:(nullable LynxResourceHandle*)handle
    NS_SWIFT_NAME(init(resourceHandle:));

/**
 * @apidoc
 * @brief Reads and parses a Lynx Bundle from a reusable resource handle with bundle options.
 * @param handle The resource handle that describes the Lynx Bundle.
 * @param option Options used while parsing and initializing the bundle. A non-null option URL takes
 * precedence over the resource file path.
 * @return The parsed `LynxTemplateBundle`, or `nil` when `handle` is `nil`.
 * @note The handle remains valid and reusable after this method returns.
 * @note A registered native C++ security service is required. Missing-service failure preserves
 * the caller's handle and does not invoke platform byte verification.
 */
- (instancetype _Nullable)initWithResourceHandle:(nullable LynxResourceHandle*)handle
                                          option:(nullable LynxTemplateBundleOption*)option
    NS_SWIFT_NAME(init(resourceHandle:option:));

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
 * @brief Returns the custom section associated with the specified key.
 * @param key The key of the custom section.
 * @return The raw bytes of the custom section, or `nil` when the bundle is invalid, the key does
 * not exist, or the custom section has an unsupported type.
 */
- (NSData* _Nullable)customSectionForKey:(nonnull NSString*)key;

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
