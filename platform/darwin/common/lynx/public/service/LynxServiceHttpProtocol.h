// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHttpRequest.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <Lynx/LynxServiceProtocol.h>

@protocol LynxHttpInterceptor
// TODO(huzhanbo.luc): let interceptRequest support streaming response by chunk

- (LynxHttpResponse*)interceptRequest:(LynxHttpRequest*)request;

- (void)onRequest:(LynxHttpRequest*)request;

- (void)onResponse:(LynxHttpResponse*)response withRequest:(LynxHttpRequest*)request;

@end

typedef void (^LynxHttpCallback)(LynxHttpResponse* result);

@protocol LynxServiceHttpProtocol <LynxServiceProtocol>

- (void)invokeWithRequest:(LynxHttpRequest*)request callback:(LynxHttpCallback)callback;

- (void)invokeStreamingWithRequest:(LynxHttpRequest*)request
                          callback:(LynxHttpCallback)callback
                      withDelegate:(LynxHttpStreamingDelegate*)delegate;

- (BOOL)setHttpInterceptor:(id<LynxHttpInterceptor>)interceptor;

@end
