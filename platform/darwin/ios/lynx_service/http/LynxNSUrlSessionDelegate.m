// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxService/LynxNSUrlSessionDelegate.h>

@implementation LynxNSUrlSessionDelegate {
  LynxHttpStreamingDelegate *_httpDelegate;
  LynxHttpCallback _callback;
  NSMutableData *_buffer;
}

- (instancetype)initWithDelegate:(LynxHttpStreamingDelegate *)httpDelegate
                    withCallback:(LynxHttpCallback)callback {
  self = [super init];
  if (self) {
    _httpDelegate = httpDelegate;
    _callback = callback;
    _buffer = [[NSMutableData alloc] init];
  }
  return self;
}

- (void)URLSession:(NSURLSession *)session
              dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveResponse:(NSURLResponse *)response
     completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
  LynxHttpResponse *resp = [[LynxHttpResponse alloc] init];
  resp.url = response.URL.absoluteString;

  NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
  resp.statusText = @"OK";
  resp.httpHeaders = httpResponse.allHeaderFields;
  resp.statusCode = httpResponse.statusCode;
  _callback(resp);
  completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {
  [_httpDelegate processChunkedData:_buffer withData:data];
}

- (void)URLSession:(NSURLSession *)session
                    task:(NSURLSessionTask *)task
    didCompleteWithError:(NSError *)error {
  if (error) {
    [_httpDelegate onError:error.localizedDescription];
  }
  [_httpDelegate onEnd];
}

@end
