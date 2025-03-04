// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCEFETCHER_H_
#define DARWIN_COMMON_LYNX_RESOURCE_LYNXTEMPLATERESOURCEFETCHER_H_

#import "LynxResourceRequest.h"
#import "LynxResourceResponse.h"
#import "LynxTemplateResource.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^LynxTemplateResourceCompletionBlock)(LynxTemplateResource* _Nullable data,
                                                    NSError* _Nullable error);

typedef void (^LynxSSRResourceCompletionBlock)(NSData* _Nullable data, NSError* _Nullable error);

@protocol LynxTemplateResourceFetcher <NSObject>

/**
 * fetch template resource of lynx & dynamic component etc.
 *
 * @param request
 * @param callback response with the requiring content file: NSData* or TemplateBundle
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
