// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHttpStreamingDelegate.h>
#import <LynxService/LynxNSUrlSessionDelegate.h>
#import <XCTest/XCTest.h>

@interface LynxHttpStreamingDelegateMock : LynxHttpStreamingDelegate
@property(nonatomic, assign) NSInteger sseCalls;
@property(nonatomic, assign) NSInteger chunkedCalls;
@property(nonatomic, assign) NSInteger streamingCalls;
@end

@implementation LynxHttpStreamingDelegateMock

- (void)processSseData:(NSMutableData *)buffer withData:(NSData *)data {
  self.sseCalls++;
}

- (void)processChunkedData:(NSMutableData *)buffer withData:(NSData *)data {
  self.chunkedCalls++;
}

- (void)processStreamingData:(NSData *)data {
  self.streamingCalls++;
}

@end

@interface LynxNSUrlSessionDelegateUnitTest : XCTestCase
@end

@implementation LynxNSUrlSessionDelegateUnitTest

- (LynxHttpStreamingDelegateMock *)streamingDelegate {
  return
      [[LynxHttpStreamingDelegateMock alloc] initWithParam:[[LynxFetchModuleEventSender alloc] init]
                                           withStreamingId:@"stream"];
}

- (void)deliverResponseWithContentType:(NSString *)contentType
                            toReceiver:(LynxNSUrlSessionDelegate *)receiver {
  NSHTTPURLResponse *response =
      [[NSHTTPURLResponse alloc] initWithURL:[NSURL URLWithString:@"https://example.com/stream"]
                                  statusCode:200
                                 HTTPVersion:@"HTTP/1.1"
                                headerFields:@{@"cOnTeNt-TyPe" : contentType}];
  __block NSURLSessionResponseDisposition disposition = NSURLSessionResponseCancel;
  [receiver URLSession:nil
                dataTask:nil
      didReceiveResponse:response
       completionHandler:^(NSURLSessionResponseDisposition value) {
         disposition = value;
       }];
  XCTAssertEqual(disposition, NSURLSessionResponseAllow);
}

- (void)testContentTypeRoutesSseAndKeepsNonSseStreamingBehavior {
  NSData *data = [@"data: message\n\n" dataUsingEncoding:NSUTF8StringEncoding];

  LynxHttpStreamingDelegateMock *sseDelegate = [self streamingDelegate];
  LynxNSUrlSessionDelegate *sseReceiver =
      [[LynxNSUrlSessionDelegate alloc] initWithDelegate:sseDelegate
                                            withCallback:^(LynxHttpResponse *response) {
                                            }
                            useDeprecatedStreamingConfig:YES];
  [self deliverResponseWithContentType:@"Text/Event-Stream; charset=utf-8" toReceiver:sseReceiver];
  [sseReceiver URLSession:nil dataTask:nil didReceiveData:data];
  XCTAssertEqual(sseDelegate.sseCalls, 1);
  XCTAssertEqual(sseDelegate.chunkedCalls, 0);
  XCTAssertEqual(sseDelegate.streamingCalls, 0);

  LynxHttpStreamingDelegateMock *chunkedDelegate = [self streamingDelegate];
  LynxNSUrlSessionDelegate *chunkedReceiver =
      [[LynxNSUrlSessionDelegate alloc] initWithDelegate:chunkedDelegate
                                            withCallback:^(LynxHttpResponse *response) {
                                            }
                            useDeprecatedStreamingConfig:YES];
  [self deliverResponseWithContentType:@"application/octet-stream" toReceiver:chunkedReceiver];
  [chunkedReceiver URLSession:nil dataTask:nil didReceiveData:data];
  XCTAssertEqual(chunkedDelegate.sseCalls, 0);
  XCTAssertEqual(chunkedDelegate.chunkedCalls, 1);
  XCTAssertEqual(chunkedDelegate.streamingCalls, 0);

  LynxHttpStreamingDelegateMock *streamingDelegate = [self streamingDelegate];
  LynxNSUrlSessionDelegate *streamingReceiver =
      [[LynxNSUrlSessionDelegate alloc] initWithDelegate:streamingDelegate
                                            withCallback:^(LynxHttpResponse *response) {
                                            }
                            useDeprecatedStreamingConfig:NO];
  [self deliverResponseWithContentType:@"application/json" toReceiver:streamingReceiver];
  [streamingReceiver URLSession:nil dataTask:nil didReceiveData:data];
  XCTAssertEqual(streamingDelegate.sseCalls, 0);
  XCTAssertEqual(streamingDelegate.chunkedCalls, 0);
  XCTAssertEqual(streamingDelegate.streamingCalls, 1);
}

@end
