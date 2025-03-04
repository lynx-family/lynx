// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxHttpService.h"
#import <Foundation/Foundation.h>

@LynxServiceRegister(LynxHttpService) @implementation LynxHttpService

static const NSInteger SDK_ERROR_STATUS_CODE = 499;

+ (LynxServiceScope)serviceScope {
  return LynxServiceScopeDefault;
}

+ (NSUInteger)serviceType {
  return kLynxServiceHttp;
}

+ (NSString *)serviceBizID {
  return DEFAULT_LYNX_SERVICE;
}

- (void)invokeWithRequest:(LynxHttpRequest *)request callback:(LynxHttpCallback)callback {
  NSURL *url = [NSURL URLWithString:request.url];

  NSMutableURLRequest *nsRequest = [NSMutableURLRequest requestWithURL:url];
  nsRequest.HTTPMethod = request.httpMethod;
  for (NSString *headerKey in request.httpHeaders) {
    [nsRequest setValue:request.httpHeaders[headerKey] forHTTPHeaderField:headerKey];
  }
  nsRequest.HTTPBody = request.httpBody;

  NSURLSession *session = [NSURLSession sharedSession];
  NSURLSessionDataTask *dataTask =
      [session dataTaskWithRequest:nsRequest
                 completionHandler:^(NSData *_Nullable data, NSURLResponse *_Nullable response,
                                     NSError *_Nullable error) {
                   LynxHttpResponse *resp = [[LynxHttpResponse alloc] init];
                   resp.url = response.URL.absoluteString;
                   resp.httpBody = data;

                   if (error) {
                     resp.statusCode = SDK_ERROR_STATUS_CODE;
                     resp.statusText = error.localizedDescription;
                   } else {
                     NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
                     resp.statusText = @"OK";
                     resp.httpHeaders = httpResponse.allHeaderFields;
                     resp.statusCode = httpResponse.statusCode;
                   }
                   callback(resp);
                 }];

  [dataTask resume];
}

- (BOOL)setHttpInterceptor:(id<LynxHttpInterceptor>)interceptor {
  return NO;
}

@end
