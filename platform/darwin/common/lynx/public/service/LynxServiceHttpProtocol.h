// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHttpRequest.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <LynxServiceAPI/ServiceAPI.h>

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

@optional

/**
 Same as `invokeWithRequest:callback:`, except that the request can be canceled.
 If implemented, it is used instead of `invokeWithRequest:callback:` by the Fetch API, and the
 returned block is invoked when the `AbortSignal` of the fetch is aborted.

 @return: A block which can cancel the request if it is not finished. nil if cancel action is not
 supported.
 */
- (dispatch_block_t)invokeCancelableWithRequest:(LynxHttpRequest*)request
                                       callback:(LynxHttpCallback)callback;

/**
 Same as `invokeStreamingWithRequest:callback:withDelegate:`, except that the request can be
 canceled. If implemented, it is used instead of `invokeStreamingWithRequest:callback:withDelegate:`
 by the Fetch API, and the returned block is invoked when the `AbortSignal` of the fetch is aborted.

 @return: A block which can cancel the request if it is not finished. nil if cancel action is not
 supported.
 */
- (dispatch_block_t)invokeCancelableStreamingWithRequest:(LynxHttpRequest*)request
                                                callback:(LynxHttpCallback)callback
                                            withDelegate:(LynxHttpStreamingDelegate*)delegate;

@end
