// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxService/LynxNSUrlSessionDelegate.h>

@implementation LynxNSUrlSessionDelegate {
  LynxHttpStreamingDelegate *_httpDelegate;
  LynxHttpCallback _callback;
  NSMutableData *_buffer;
  BOOL _useDeprecatedStreamingConfig;
  BOOL _isSseResponse;  // 响应 content-type 是 text/event-stream
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
    _isSseResponse = NO;
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
  // 检测 SSE 响应：content-type 含 text/event-stream
  NSString *contentType = [[httpResponse valueForHTTPHeaderField:@"Content-Type"] lowercaseString] ?: @"";
  if ([contentType containsString:@"text/event-stream"]) {
    _isSseResponse = YES;
  }
  _callback(resp);
  completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session
          dataTask:(NSURLSessionDataTask *)dataTask
    didReceiveData:(NSData *)data {
  // SSE 响应或非 deprecated 配置：走 processStreamingData（直接 onData 传整块，JS 端按 SSE 解析）。
  // 否则（deprecated chunked 配置且非 SSE）：走 processChunkedData（按 chunked 编码解析）。
  // 修复：原逻辑 useStreaming 时一律走 chunked，对 SSE 数据解析失败丢弃。
  if (_useDeprecatedStreamingConfig && !_isSseResponse) {
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
