// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <AnimaX/AnimaXLoaderProtocol.h>
#import <Foundation/Foundation.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxResourceFetcher.h>
#import <Lynx/LynxUIContext.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxAnimaXResourceFetcherLoader : NSObject <AnimaXLoaderProtocol>

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)new NS_UNAVAILABLE;

/**
 * Creates a loader with a LynxUIContext.
 * This method is used by LynxUI to initialize the resource loading service.
 * It will try to get both generic and legacy fetchers from the context.
 *
 * @param context The LynxUIContext containing resource fetchers
 * @return A new loader instance
 */
+ (LynxAnimaXResourceFetcherLoader *)loaderWithLynxUIContext:(LynxUIContext *)context;
/**
 * Creates a loader with a generic resource fetcher.
 * This is the recommended way to create a loader as it uses the new fetching mechanism.
 *
 * @param genericFetcher The generic resource fetcher to use
 * @return A new loader instance
 */
+ (LynxAnimaXResourceFetcherLoader *)loaderWithLynxGenericFetcher:
    (nullable id<LynxGenericResourceFetcher>)genericFetcher;
/**
 * Creates a loader with a legacy resource fetcher.
 * @deprecated This method uses the legacy fetching mechanism and is not recommended for new code.
 * Use loaderWithLynxGenericFetcher: instead.
 *
 * @param resourceFetcher The legacy resource fetcher to use
 * @return A new loader instance
 */
+ (LynxAnimaXResourceFetcherLoader *)loaderWithLynxResourceFetcher:
    (nullable id<LynxResourceFetcher>)resourceFetcher __deprecated;

- (void)handleRequest:(AnimaXLoaderRequest *)request
           completion:(AnimaXLoaderCompletionHandler)completion;
- (AnimaXLoaderScheme)getScheme;

@end

NS_ASSUME_NONNULL_END
