// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/StaticPageHost.h>

NS_ASSUME_NONNULL_BEGIN

typedef void (^StaticPageTaskDispatcher)(dispatch_block_t task);

@interface StaticPageHost ()

- (instancetype)initWithTaskDispatcher:(StaticPageTaskDispatcher)taskDispatcher;
- (nullable NSDictionary<NSString*, id>*)
    registerInstanceId:(int32_t)instanceId
                  data:(nullable NSDictionary<NSString*, id>*)data
           globalProps:(nullable NSDictionary<NSString*, id>*)globalProps;
- (BOOL)isRegistered;
- (void)updateMetaData:(nullable NSDictionary<NSString*, id>*)data
           globalProps:(nullable NSDictionary<NSString*, id>*)globalProps;
- (void)clear;

@end

NS_ASSUME_NONNULL_END
