// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxHttpRequest.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <Lynx/LynxNetworkRequestObserver.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceHttpProtocol.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxFetchModule.h"

@interface LynxFetchModule (UnitTest)
- (void)fetch:(NSDictionary *)request
      resolve:(LynxCallbackBlock)resolve
       reject:(LynxCallbackBlock)reject;
- (void)cancel:(NSString *)requestId;
@end

@interface LynxNetworkRequestObserverMock : NSObject <LynxNetworkRequestObserver>
@property(nonatomic, strong) NSMutableArray<NSString *> *events;
@property(nonatomic, copy) NSString *requestURL;
@property(nonatomic, copy) NSString *requestMethod;
@property(nonatomic, copy) NSDictionary *requestHeaders;
@property(nonatomic, copy) NSData *requestBody;
@end

@implementation LynxNetworkRequestObserverMock

- (instancetype)init {
  if (self = [super init]) {
    _events = [[NSMutableArray alloc] init];
  }
  return self;
}

- (BOOL)isEnabled {
  return YES;
}

- (NSString *)requestWillBeSent:(nullable NSString *)url
                         method:(nullable NSString *)method
                        headers:(nullable NSDictionary *)headers
                           body:(nullable NSData *)body {
  [self.events addObject:@"request"];
  self.requestURL = url;
  self.requestMethod = method;
  self.requestHeaders = headers;
  self.requestBody = body;
  return @"request-id";
}

- (void)responseReceived:(NSString *)requestId
                     url:(nullable NSString *)url
                  status:(NSInteger)status
              statusText:(nullable NSString *)statusText
                 headers:(nullable NSDictionary *)headers {
  [self.events addObject:@"response"];
}

- (void)dataReceived:(NSString *)requestId data:(nullable NSData *)data {
  [self.events addObject:@"data"];
}

- (void)loadingFinished:(NSString *)requestId {
  [self.events addObject:@"finished"];
}

- (void)loadingFailed:(NSString *)requestId
            errorText:(nullable NSString *)errorText
             canceled:(BOOL)canceled {
  [self.events addObject:@"failed"];
}

@end

@interface LynxFetchModuleEventSenderMock : LynxFetchModuleEventSender
@property(nonatomic, strong) LynxNetworkRequestObserverMock *observer;
@property(nonatomic, strong) NSMutableArray<NSDictionary *> *globalEvents;
@end

@implementation LynxFetchModuleEventSenderMock

- (instancetype)init {
  if (self = [super init]) {
    _observer = [[LynxNetworkRequestObserverMock alloc] init];
    _globalEvents = [[NSMutableArray alloc] init];
  }
  return self;
}

- (id<LynxNetworkRequestObserver>)networkRequestObserver {
  return self.observer;
}

- (void)sendGlobalEvent:(NSString *)name withParams:(nullable NSArray *)params {
  [self.globalEvents addObject:@{
    @"name" : name,
    @"params" : params ?: @[],
  }];
}

@end

@interface LynxServiceHttpMock : NSObject <LynxServiceHttpProtocol>
@property(nonatomic, strong) LynxHttpRequest *lastRequest;
@property(nonatomic, assign) BOOL usedStreaming;
@end

@implementation LynxServiceHttpMock

- (LynxHttpResponse *)response {
  LynxHttpResponse *response = [[LynxHttpResponse alloc] init];
  response.url = @"https://example.com/response";
  response.statusCode = 200;
  response.statusText = @"OK";
  response.httpHeaders = @{@"Content-Type" : @"application/json"};
  response.httpBody = [@"response-body" dataUsingEncoding:NSUTF8StringEncoding];
  return response;
}

- (void)invokeWithRequest:(LynxHttpRequest *)request callback:(LynxHttpCallback)callback {
  self.lastRequest = request;
  callback([self response]);
}

- (void)invokeStreamingWithRequest:(LynxHttpRequest *)request
                          callback:(LynxHttpCallback)callback
                      withDelegate:(LynxHttpStreamingDelegate *)delegate {
  self.lastRequest = request;
  self.usedStreaming = YES;
  callback([self response]);
  [delegate onData:[@"chunk" dataUsingEncoding:NSUTF8StringEncoding]];
  [delegate onEnd];
}

- (BOOL)setHttpInterceptor:(id<LynxHttpInterceptor>)interceptor {
  return NO;
}

@end

@interface LynxCancelableServiceHttpMock : LynxServiceHttpMock
@property(nonatomic, copy) LynxHttpCallback pendingCallback;
@property(nonatomic, strong) LynxHttpStreamingDelegate *pendingDelegate;
@property(nonatomic, assign) NSInteger cancelCount;
@end

@implementation LynxCancelableServiceHttpMock

- (dispatch_block_t)invokeCancelableWithRequest:(LynxHttpRequest *)request
                                       callback:(LynxHttpCallback)callback {
  self.lastRequest = request;
  self.pendingCallback = callback;
  return ^{
    self.cancelCount++;
  };
}

- (dispatch_block_t)invokeCancelableStreamingWithRequest:(LynxHttpRequest *)request
                                                callback:(LynxHttpCallback)callback
                                            withDelegate:(LynxHttpStreamingDelegate *)delegate {
  self.lastRequest = request;
  self.usedStreaming = YES;
  self.pendingCallback = callback;
  self.pendingDelegate = delegate;
  return ^{
    self.cancelCount++;
  };
}

@end

@interface LynxFetchModuleUnitTest : XCTestCase
@end

@implementation LynxFetchModuleUnitTest {
  LynxFetchModuleEventSenderMock *_sender;
  LynxServiceHttpMock *_httpService;
  LynxFetchModule *_module;
  id _lynxServicesMock;
}

- (void)setUp {
  [super setUp];
  _sender = [[LynxFetchModuleEventSenderMock alloc] init];
  _httpService = [[LynxServiceHttpMock alloc] init];
  _module = [[LynxFetchModule alloc] initWithParam:_sender];
  _lynxServicesMock = OCMClassMock(LynxServices.class);
  OCMStub(
      ClassMethod([_lynxServicesMock getInstanceWithProtocol:@protocol(LynxServiceHttpProtocol)]))
      .andReturn(_httpService);
}

- (void)tearDown {
  [_lynxServicesMock stopMocking];
  [super tearDown];
}

- (void)testNormalFetchReportsNetworkLifecycleAndRequestData {
  NSData *requestBody = [@"request-body" dataUsingEncoding:NSUTF8StringEncoding];
  NSDictionary *requestHeaders = @{@"Content-Type" : @"application/json"};
  __block NSDictionary *result = nil;

  [_module fetch:@{
    @"url" : @"https://example.com/request",
    @"method" : @"POST",
    @"headers" : requestHeaders,
    @"body" : requestBody,
  }
      resolve:^(id value) {
        result = value;
      }
      reject:^(id error) {
        XCTFail(@"Unexpected Fetch rejection: %@", error);
      }];

  XCTAssertEqualObjects(_sender.observer.events,
                        (@[ @"request", @"response", @"data", @"finished" ]));
  XCTAssertEqualObjects(_sender.observer.requestURL, @"https://example.com/request");
  XCTAssertEqualObjects(_sender.observer.requestMethod, @"POST");
  XCTAssertEqualObjects(_sender.observer.requestHeaders, requestHeaders);
  XCTAssertEqualObjects(_sender.observer.requestBody, requestBody);
  XCTAssertEqualObjects(result[@"body"], [@"response-body" dataUsingEncoding:NSUTF8StringEncoding]);
  XCTAssertFalse(_httpService.usedStreaming);
}

- (void)testStreamingFetchReportsNetworkLifecycleAndKeepsStreamingEvents {
  __block NSDictionary *result = nil;

  [_module fetch:@{
    @"url" : @"https://example.com/stream",
    @"method" : @"GET",
    @"lynxExtension" : @{@"enableFetchAPIStandardStreaming" : @YES},
  }
      resolve:^(id value) {
        result = value;
      }
      reject:^(id error) {
        XCTFail(@"Unexpected Fetch rejection: %@", error);
      }];

  XCTAssertEqualObjects(_sender.observer.events,
                        (@[ @"request", @"response", @"data", @"finished" ]));
  XCTAssertTrue(_httpService.usedStreaming);
  XCTAssertNotNil(result[@"lynxExtension"][@"streamingId"]);
  XCTAssertEqual(_sender.globalEvents.count, 2u);
  XCTAssertEqualObjects(_sender.globalEvents[0][@"params"][0][@"event"], @"onData");
  XCTAssertEqualObjects(_sender.globalEvents[1][@"params"][0][@"event"], @"onEnd");
}

- (void)testCancelWithoutCancelableHttpServiceIsNoOp {
  __block NSDictionary *result = nil;

  [_module fetch:@{
    @"requestId" : @"1",
    @"url" : @"https://example.com/request",
    @"method" : @"GET",
  }
      resolve:^(id value) {
        result = value;
      }
      reject:^(id error) {
        XCTFail(@"Unexpected Fetch rejection: %@", error);
      }];
  [_module cancel:@"1"];

  XCTAssertNotNil(result);
}

@end

@interface LynxFetchModuleCancelUnitTest : XCTestCase
@end

@implementation LynxFetchModuleCancelUnitTest {
  LynxFetchModuleEventSenderMock *_sender;
  LynxCancelableServiceHttpMock *_httpService;
  LynxFetchModule *_module;
  id _lynxServicesMock;
}

- (void)setUp {
  [super setUp];
  _sender = [[LynxFetchModuleEventSenderMock alloc] init];
  _httpService = [[LynxCancelableServiceHttpMock alloc] init];
  _module = [[LynxFetchModule alloc] initWithParam:_sender];
  _lynxServicesMock = OCMClassMock(LynxServices.class);
  OCMStub(
      ClassMethod([_lynxServicesMock getInstanceWithProtocol:@protocol(LynxServiceHttpProtocol)]))
      .andReturn(_httpService);
}

- (void)tearDown {
  [_lynxServicesMock stopMocking];
  [super tearDown];
}

- (void)fetchWithRequestId:(NSString *)requestId
                 streaming:(BOOL)streaming
                   resolve:(LynxCallbackBlock)resolve {
  [_module fetch:@{
    @"requestId" : requestId,
    @"url" : @"https://example.com/request",
    @"method" : @"GET",
    @"lynxExtension" : @{@"enableFetchAPIStandardStreaming" : @(streaming)},
  }
         resolve:resolve
          reject:^(id error) {
            XCTFail(@"Unexpected Fetch rejection: %@", error);
          }];
}

- (void)testCancelPendingRequestInvokesCancelBlockOnce {
  [self fetchWithRequestId:@"1"
                 streaming:NO
                   resolve:^(id value){
                   }];
  XCTAssertNotNil(_httpService.pendingCallback);
  [_module cancel:@"unknown"];
  XCTAssertEqual(_httpService.cancelCount, 0);

  [_module cancel:@"1"];
  [_module cancel:@"1"];

  XCTAssertEqual(_httpService.cancelCount, 1);
}

- (void)testCancelFinishedRequestIsNoOp {
  __block NSDictionary *result = nil;
  [self fetchWithRequestId:@"1"
                 streaming:NO
                   resolve:^(id value) {
                     result = value;
                   }];

  _httpService.pendingCallback([_httpService response]);
  [_module cancel:@"1"];

  XCTAssertNotNil(result);
  XCTAssertEqual(_httpService.cancelCount, 0);
}

- (void)testCancelStreamingRequestUntilEnd {
  [self fetchWithRequestId:@"1"
                 streaming:YES
                   resolve:^(id value){
                   }];
  XCTAssertTrue(_httpService.usedStreaming);
  // A streaming request is still in flight after the response is received.
  _httpService.pendingCallback([_httpService response]);
  [_module cancel:@"1"];
  XCTAssertEqual(_httpService.cancelCount, 1);

  [self fetchWithRequestId:@"2"
                 streaming:YES
                   resolve:^(id value){
                   }];
  _httpService.pendingCallback([_httpService response]);
  [_httpService.pendingDelegate onEnd];
  [_module cancel:@"2"];
  XCTAssertEqual(_httpService.cancelCount, 1);
}

@end
