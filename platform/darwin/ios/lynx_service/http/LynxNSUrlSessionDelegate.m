// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxService/LynxNSUrlSessionDelegate.h>

static BOOL IsServerSentEventResponse(NSHTTPURLResponse *response) {
  __block NSString *contentType = nil;
  [response.allHeaderFields enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL *stop) {
    if ([[key description] caseInsensitiveCompare:@"Content-Type"] == NSOrderedSame) {
      contentType = [value description];
      *stop = YES;
    }
  }];
  if (contentType.length == 0) {
    return NO;
  }
  NSString *mediaType = [[[contentType componentsSeparatedByString:@";"] firstObject]
      stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
  return [mediaType caseInsensitiveCompare:@"text/event-stream"] == NSOrderedSame;
}

@implementation LynxNSUrlSessionDelegate {
  LynxHttpStreamingDelegate *_httpDelegate;
  LynxHttpCallback _callback;
  NSMutableData *_buffer;
  BOOL _useDeprecatedStreamingConfig;
  BOOL _isServerSentEvent;
}

- (instancetype)initWithDelegate:(LynxHttpStreamingDelegate *)httpDelegate
                    withCallback:(LynxHttpCallback)callback
    useDeprecatedStreamingConfig:(BOOL)useDeprecatedStreamingConfig {
  self = [super init];
  if (self) {
    _httpDelegate = httpDelegate;
    _callback = callback;
    _buffer = [[NSMutableData alloc] init];
    _useDeprecatedStreamingConfig = useDeprecatedStreamingConfig;
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
  _isServerSentEvent = IsServerSentEventResponse(httpResponse);
  resp.statusText = @"OK";
  resp.httpHeaders = httpResponse.allHeaderFields;
  resp.statusCode = httpResponse.statusCode;
  _callback(resp);
  completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {
  if (_isServerSentEvent) {
    [_httpDelegate processSseData:_buffer withData:data];
  } else if (_useDeprecatedStreamingConfig) {
    [_httpDelegate processChunkedData:_buffer withData:data];
  } else {
    [_httpDelegate processStreamingData:data];
  }
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
