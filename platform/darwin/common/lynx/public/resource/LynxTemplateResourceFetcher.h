// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCEFETCHER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCEFETCHER_H_

#import <Lynx/LynxResourceRequest.h>
#import <Lynx/LynxResourceResponse.h>
#import <Lynx/LynxTemplateResource.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxTemplateResourceCompletionBlock)(LynxTemplateResource* _Nullable data,
                                                    NSError* _Nullable error);

typedef void (^LynxSSRResourceCompletionBlock)(NSData* _Nullable data, NSError* _Nullable error);

/**
 * @apidoc
 * @brief `LynxTemplateResourceFetcher` is defined inside LynxEngine and
 * injected from outside to implement the resource loading interface of
 * [Bundle](/guide/spec.html#lynx-bundle-or-bundle) and [Lazy Bundle](/guide/spec.html#lazy-bundle)
 * etc.
 */
@protocol LynxTemplateResourceFetcher <NSObject>

/**
 * @apidoc
 * @brief `LynxEngine` internally calls this method to obtain the contents of Bundle
 * and Lazy Bundle. The returned result type can contain template data, `TemplateBundle`, or
 * `LynxResourceHandle`. Resource handles are supported for templates only; lazy bundles must
 * return template data or a pre-parsed `TemplateBundle`.
 *
 * @param request Request for the requiring content file.
 * @param callback Response with the requiring template resource.
 * @note This method must be implemented.
 */
@required
- (void)fetchTemplate:(LynxResourceRequest* _Nonnull)request
           onComplete:(LynxTemplateResourceCompletionBlock _Nonnull)callback;

/**
 * fetch SSRData of lynx.
 *
 * @param request
 * @param callback response with the requiring SSR data.
 */
@required
- (void)fetchSSRData:(LynxResourceRequest* _Nonnull)request
          onComplete:(LynxSSRResourceCompletionBlock _Nonnull)callback;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCEFETCHER_H_
