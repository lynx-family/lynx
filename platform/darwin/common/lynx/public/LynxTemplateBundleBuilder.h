// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_BUILDER_H_
#define DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_BUILDER_H_

#import <Lynx/LynxTemplateBundle.h>

/**
 * @apidoc
 * @brief Builds a `TemplateBundle` from separated template sources.
 */
@interface LynxTemplateBundleBuilder : NSObject

/**
 * @apidoc
 * @brief Creates a `TemplateBundleBuilder`.
 */
+ (nonnull instancetype)create;

/**
 * @apidoc
 * @brief Sets the main-thread script source.
 */
- (nonnull instancetype)main:(nullable NSString*)script NS_SWIFT_NAME(main(_:));

/**
 * @apidoc
 * @brief Sets the background script source.
 */
- (nonnull instancetype)background:(nullable NSString*)script NS_SWIFT_NAME(background(_:));

/**
 * @apidoc
 * @brief Builds a `TemplateBundle`.
 */
- (nullable LynxTemplateBundle*)build;

@end

#endif  // DARWIN_COMMON_LYNX_LYNX_TEMPLATE_BUNDLE_BUILDER_H_
