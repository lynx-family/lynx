// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXEXTERNALRESOURCEFETCHERWRAPPER_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXEXTERNALRESOURCEFETCHERWRAPPER_H_

#import "LynxDynamicComponentFetcher.h"
#import "LynxResourceServiceFetcher.h"

NS_ASSUME_NONNULL_BEGIN

typedef void (^LoadedBlock)(NSData* _Nullable data, NSError* _Nullable error);

// TODO(zhoupeng.z): support for more types of resource requests
// TODO(zhoupeng.z): consider to remove this wrapper after the lynx resource service fetcher is
// stable or deprecated

@interface LynxExternalResourceFetcherWrapper : NSObject

@property(atomic, readwrite) BOOL enableLynxService;

- (instancetype)initWithDynamicComponentFetcher:(id<LynxDynamicComponentFetcher>)fetcher;

- (void)fetchResource:(NSString*)url withLoadedBlock:(LoadedBlock)block;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXEXTERNALRESOURCEFETCHERWRAPPER_H_
