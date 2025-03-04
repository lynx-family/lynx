// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxFetchModule.h"
#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import "LynxHttpRequest.h"
#import "LynxModule.h"
#import "LynxService.h"
#import "LynxServiceHttpProtocol.h"

@implementation LynxFetchModule

+ (NSString *)name {
  return @"LynxFetchModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"fetch" : NSStringFromSelector(@selector(fetch:resolve:reject:)),
  };
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

  id<LynxServiceHttpProtocol> httpService = LynxService(LynxServiceHttpProtocol);
  if (!httpService) {
    reject(@{
      @"message" : @"Lynx Http Service not registered",
    });
    return;
  }

  [httpService invokeWithRequest:httpRequest callback:block];
}

@end
