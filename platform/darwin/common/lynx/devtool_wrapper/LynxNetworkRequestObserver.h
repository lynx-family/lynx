// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/** Internal bridge from the Darwin Fetch implementation to its owning DevTool instance. */
@protocol LynxNetworkRequestObserver <NSObject>

- (BOOL)isEnabled;

- (NSString *)requestWillBeSent:(nullable NSString *)url
                         method:(nullable NSString *)method
                        headers:(nullable NSDictionary *)headers
                           body:(nullable NSData *)body;

- (void)responseReceived:(NSString *)requestId
                     url:(nullable NSString *)url
                  status:(NSInteger)status
              statusText:(nullable NSString *)statusText
                 headers:(nullable NSDictionary *)headers;

- (void)dataReceived:(NSString *)requestId data:(nullable NSData *)data;

- (void)loadingFinished:(NSString *)requestId;

- (void)loadingFailed:(NSString *)requestId
            errorText:(nullable NSString *)errorText
             canceled:(BOOL)canceled;

@end

NS_ASSUME_NONNULL_END
