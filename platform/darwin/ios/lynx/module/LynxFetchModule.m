// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFetchModule.h"
#import <Foundation/Foundation.h>
#import <Lynx/LynxHttpRequest.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <Lynx/LynxModule.h>
#import <Lynx/LynxNetworkRequestObserver.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceHttpProtocol.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventWrapper.h>
#import <objc/runtime.h>
#import <stdatomic.h>
#import "LynxTraceEventDef.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxFetchModuleEventSender ()
- (nullable id<LynxNetworkRequestObserver>)networkRequestObserver;
@end

@interface LynxHttpStreamingDelegate ()
- (instancetype)initWithParam:(LynxFetchModuleEventSender *)sender
              withStreamingId:(NSString *)streamingId
              networkObserver:(nullable id<LynxNetworkRequestObserver>)networkObserver
             networkRequestId:(NSString *)networkRequestId;
@end

NS_ASSUME_NONNULL_END

@implementation LynxFetchModule {
  LynxFetchModuleEventSender *_eventSender;
}

static atomic_long streamingCounter;
NSString *const streamingEventNamePrefix = @"LynxFetchModuleStreamingEvent";
NSString *const deprecatedUseStreamingFlag = @"useStreaming";
NSString *const standardStreamingFlag = @"enableFetchAPIStandardStreaming";

- (instancetype)initWithParam:(id)param {
  if (self = [super init]) {
    _eventSender = param;
  }
  return self;
}

+ (NSString *)name {
  return @"LynxFetchModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"fetch" : NSStringFromSelector(@selector(fetch:resolve:reject:)),
  };
}

- (void)request:(LynxHttpRequest *)httpRequest
         withResolve:(LynxCallbackBlock)resolve
     withHttpService:(id<LynxServiceHttpProtocol>)httpService
     networkObserver:(nullable id<LynxNetworkRequestObserver>)networkObserver
    networkRequestId:(NSString *)networkRequestId {
  LynxHttpCallback block = ^(LynxHttpResponse *response) {
    NSData *responseBody = response.httpBody ?: [[NSData alloc] init];
    NSDictionary *responseHeaders = response.httpHeaders ?: @{};
    NSString *statusText = response.statusText ?: @"";
    if (networkRequestId.length > 0) {
      [networkObserver responseReceived:networkRequestId
                                    url:response.url
                                 status:response.statusCode
                             statusText:statusText
                                headers:responseHeaders];
      [networkObserver dataReceived:networkRequestId data:responseBody];
      [networkObserver loadingFinished:networkRequestId];
    }

    resolve(@{
      @"url" : httpRequest.url ?: @"",
      @"body" : responseBody,
      @"headers" : responseHeaders,
      @"status" : @(response.statusCode),
      @"statusText" : statusText,
      @"lynxExtension" : response.customInfo ?: @{},
    });
  };

  [httpService invokeWithRequest:httpRequest callback:block];
}

- (void)requestStreaming:(LynxHttpRequest *)httpRequest
             withResolve:(LynxCallbackBlock)resolve
         withHttpService:(id<LynxServiceHttpProtocol>)httpService
         networkObserver:(nullable id<LynxNetworkRequestObserver>)networkObserver
        networkRequestId:(NSString *)networkRequestId {
  NSString *streamingId = [NSString
      stringWithFormat:@"%@%ld", streamingEventNamePrefix, atomic_fetch_add(&streamingCounter, 1)];
  LynxHttpCallback block = ^(LynxHttpResponse *response) {
    NSDictionary *responseHeaders = response.httpHeaders ?: @{};
    NSString *statusText = response.statusText ?: @"";
    if (networkRequestId.length > 0) {
      [networkObserver responseReceived:networkRequestId
                                    url:response.url
                                 status:response.statusCode
                             statusText:statusText
                                headers:responseHeaders];
    }

    NSMutableDictionary *customInfo = [response.customInfo ?: @{} mutableCopy];
    customInfo[@"streamingId"] = streamingId;

    resolve(@{
      @"url" : httpRequest.url ?: @"",
      @"body" : [[NSData new] init],
      @"headers" : responseHeaders,
      @"status" : @(response.statusCode),
      @"statusText" : statusText,
      @"lynxExtension" : customInfo,
    });
  };

  LynxHttpStreamingDelegate *delegate =
      [[LynxHttpStreamingDelegate alloc] initWithParam:_eventSender
                                       withStreamingId:streamingId
                                       networkObserver:networkObserver
                                      networkRequestId:networkRequestId];

  [httpService invokeStreamingWithRequest:httpRequest callback:block withDelegate:delegate];
}

- (void)traceFetchRequestWithURL:(id)url {
  if (![LynxTraceEvent categoryEnabled:LYNX_TRACE_CATEGORY_WRAPPER]) {
    return;
  }
  LYNX_TRACE_INSTANT_WITH_DEBUG_INFO(
      LYNX_TRACE_CATEGORY_WRAPPER, NATIVE_MODULE_NETWORK_REQUEST, (@{
        @"url" : [url isKindOfClass:NSString.class] ? (NSString *)url : @"",
        @"module_name" : @"LynxFetchModule",
        @"method_name" : @"fetch",
      }));
}

- (void)fetch:(NSDictionary *)request
      resolve:(LynxCallbackBlock)resolve
       reject:(LynxCallbackBlock)reject {
  LynxHttpRequest *httpRequest = [[LynxHttpRequest alloc] init];
  httpRequest.httpMethod = request[@"method"];
  httpRequest.url = request[@"url"];
  [self traceFetchRequestWithURL:httpRequest.url];
  httpRequest.originUrl = request[@"origin"];
  httpRequest.httpHeaders = request[@"headers"];
  httpRequest.httpBody = request[@"body"];
  httpRequest.customConfig = request[@"lynxExtension"];

  BOOL enableFetchApiStandardStreaming = NO;
  {
    NSObject *value = httpRequest.customConfig[standardStreamingFlag];
    if ([value isKindOfClass:[NSNumber class]]) {
      enableFetchApiStandardStreaming = [(NSNumber *)value boolValue];
    }
  }

  BOOL useDeprecatedStreamingConfig = NO;
  {
    NSObject *value = httpRequest.customConfig[deprecatedUseStreamingFlag];
    if ([value isKindOfClass:[NSNumber class]]) {
      useDeprecatedStreamingConfig = [(NSNumber *)value boolValue];
    }
  }

  id<LynxNetworkRequestObserver> networkObserver = [_eventSender networkRequestObserver];
  NSString *networkRequestId = @"";
  if (networkObserver.isEnabled) {
    networkRequestId = [networkObserver requestWillBeSent:httpRequest.url
                                                   method:httpRequest.httpMethod
                                                  headers:httpRequest.httpHeaders
                                                     body:httpRequest.httpBody];
  }

  id<LynxServiceHttpProtocol> httpService = LynxService(LynxServiceHttpProtocol);
  if (!httpService) {
    if (networkRequestId.length > 0) {
      [networkObserver loadingFailed:networkRequestId
                           errorText:@"Lynx Http Service not registered"
                            canceled:NO];
    }
    reject(@{
      @"message" : @"Lynx Http Service not registered",
    });
    return;
  }

  if (!useDeprecatedStreamingConfig && !enableFetchApiStandardStreaming) {
    [self request:httpRequest
             withResolve:resolve
         withHttpService:httpService
         networkObserver:networkObserver
        networkRequestId:networkRequestId];
  } else {
    [self requestStreaming:httpRequest
               withResolve:resolve
           withHttpService:httpService
           networkObserver:networkObserver
          networkRequestId:networkRequestId];
  }
}

@end
