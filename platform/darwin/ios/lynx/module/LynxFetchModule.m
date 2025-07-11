// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFetchModule.h"
#import <Foundation/Foundation.h>
#import <Lynx/LynxHttpRequest.h>
#import <Lynx/LynxModule.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceHttpProtocol.h>
#import <objc/runtime.h>
#import <stdatomic.h>

@implementation LynxFetchModule {
  LynxFetchModuleEventSender *_eventSender;
}

static atomic_long streamingCounter;
NSString *const streamingEventNamePrefix = @"LynxFetchModuleStreamingEvent";

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
    withHttpService:(id<LynxServiceHttpProtocol>)httpService {
  LynxHttpCallback block = ^(LynxHttpResponse *response) {
    resolve(@{
      @"url" : httpRequest.url ?: @"",
      @"body" : response.httpBody ?: [[NSData new] init],
      @"headers" : response.httpHeaders ?: @{},
      @"status" : @(response.statusCode),
      @"statusText" : response.statusText ?: @"",
      @"lynxExtension" : response.customInfo ?: @{},
    });
  };

  [httpService invokeWithRequest:httpRequest callback:block];
}

- (void)requestStreaming:(LynxHttpRequest *)httpRequest
             withResolve:(LynxCallbackBlock)resolve
         withHttpService:(id<LynxServiceHttpProtocol>)httpService {
  NSString *streamingId = [NSString
      stringWithFormat:@"%@%ld", streamingEventNamePrefix, atomic_fetch_add(&streamingCounter, 1)];
  LynxHttpCallback block = ^(LynxHttpResponse *response) {
    NSMutableDictionary *customInfo = [response.customInfo ?: @{} mutableCopy];
    customInfo[@"streamingId"] = streamingId;

    resolve(@{
      @"url" : httpRequest.url ?: @"",
      @"body" : [[NSData new] init],
      @"headers" : response.httpHeaders ?: @{},
      @"status" : @(response.statusCode),
      @"statusText" : response.statusText ?: @"",
      @"lynxExtension" : customInfo,
    });
  };

  LynxHttpStreamingDelegate *delegate =
      [[LynxHttpStreamingDelegate alloc] initWithParam:_eventSender withStreamingId:streamingId];

  [httpService invokeStreamingWithRequest:httpRequest callback:block withDelegate:delegate];
}

- (void)fetch:(NSDictionary *)request
      resolve:(LynxCallbackBlock)resolve
       reject:(LynxCallbackBlock)reject {
  LynxHttpRequest *httpRequest = [[LynxHttpRequest alloc] init];
  httpRequest.httpMethod = request[@"method"];
  httpRequest.url = request[@"url"];
  httpRequest.originUrl = request[@"origin"];
  httpRequest.httpHeaders = request[@"headers"];
  httpRequest.httpBody = request[@"body"];
  httpRequest.customConfig = request[@"lynxExtension"];
  BOOL useStreaming = httpRequest.customConfig[@"useStreaming"] ?: NO;

  id<LynxServiceHttpProtocol> httpService = LynxService(LynxServiceHttpProtocol);
  if (!httpService) {
    reject(@{
      @"message" : @"Lynx Http Service not registered",
    });
    return;
  }

  if (!useStreaming) {
    [self request:httpRequest withResolve:resolve withHttpService:httpService];
  } else {
    [self requestStreaming:httpRequest withResolve:resolve withHttpService:httpService];
  }
}

@end
