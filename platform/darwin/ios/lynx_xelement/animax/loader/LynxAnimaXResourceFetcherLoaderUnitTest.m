// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import <AnimaX/AnimaXLoaderRequest.h>
#import <AnimaX/AnimaXLoaderResponse.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxResourceFetcher.h>
#import <Lynx/LynxResourceRequest.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <XElement/LynxAnimaXResourceFetcherLoader.h>

@interface MockGenericResourceFetcher : NSObject <LynxGenericResourceFetcher>
- (dispatch_block_t)fetchResource:(LynxResourceRequest *)request
                       onComplete:(LynxGenericResourceCompletionBlock)callback;
- (dispatch_block_t)fetchResourcePath:(LynxResourceRequest *)request
                           onComplete:(LynxGenericResourcePathCompletionBlock)callback;
@end

@implementation MockGenericResourceFetcher
- (dispatch_block_t)fetchResource:(LynxResourceRequest *)request
                       onComplete:(LynxGenericResourceCompletionBlock)callback {
  return nil;
}

- (dispatch_block_t)fetchResourcePath:(LynxResourceRequest *)request
                           onComplete:(LynxGenericResourcePathCompletionBlock)callback {
  return nil;
}
@end

@interface MockLynxResourceFetcher : NSObject <LynxResourceFetcher>
- (dispatch_block_t)loadResourceWithURL:(NSURL *)url
                                   type:(LynxFetchResType)type
                             completion:(LynxResourceLoadCompletionBlock)completionBlock;
@end

@implementation MockLynxResourceFetcher
- (dispatch_block_t)loadResourceWithURL:(NSURL *)url
                                   type:(LynxFetchResType)type
                             completion:(LynxResourceLoadCompletionBlock)completionBlock {
  return nil;
}
@end

@interface LynxAnimaXResourceFetcherLoaderUnitTest : XCTestCase
@property(nonatomic) NSData *expectedData;
@property(nonatomic) NSError *expectedError;
@property(nonatomic) NSURL *expectedURL;
@property(nonatomic) id mockFetcher;
@property(nonatomic) id mockGenericFetcher;
@property(nonatomic) id mockContext;
@property(nonatomic) id<AnimaXLoaderProtocol> loader;
@end

@implementation LynxAnimaXResourceFetcherLoaderUnitTest

- (void)setUp {
  [super setUp];
  _expectedData = nil;
  _expectedError = nil;
  _expectedURL = nil;
  _mockFetcher = OCMProtocolMock(@protocol(LynxResourceFetcher));
  _mockGenericFetcher = OCMProtocolMock(@protocol(LynxGenericResourceFetcher));
  _mockContext = OCMClassMock([LynxUIContext class]);

  OCMStub([_mockContext resourceFetcher]).andReturn(_mockFetcher);
  OCMStub([_mockContext genericResourceFetcher]).andReturn(_mockGenericFetcher);

  _loader = [LynxAnimaXResourceFetcherLoader loaderWithLynxUIContext:self.mockContext];
}

- (void)tearDown {
  [_mockFetcher stopMocking];
  [_mockGenericFetcher stopMocking];
  [_mockContext stopMocking];
  _mockFetcher = nil;
  _mockGenericFetcher = nil;
  _mockContext = nil;
  [super tearDown];
}

- (void)testHandleRequestWithValidURL {
  NSString *urlString = @"http://example.com/resource";
  NSURL *expectedURL = [NSURL URLWithString:urlString];
  NSData *expectedData = [@"Example Data" dataUsingEncoding:NSUTF8StringEncoding];

  OCMStub([self.mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:expectedData, [NSNull null], nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];

  XCTestExpectation *expectation = [self expectationWithDescription:@"Completion handler called"];
  [self.loader handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.payload, @"Payload should not be nil");
                    XCTAssertEqualObjects(response.payload, expectedData,
                                          @"Payload should match expected data");
                    XCTAssertNil(response.error, @"Error should be nil");
                    [expectation fulfill];
                  }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleRequestWithValidURL_ReturnFileNSURL {
  NSString *urlString = @"http://example.com/resource";
  NSURL *resultURL = [NSURL URLWithString:@"/tmp/user/a.json"];

  OCMStub([self.mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:[resultURL path], [NSNull null], nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];

  XCTestExpectation *expectation = [self expectationWithDescription:@"Completion handler called"];
  [self.loader handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.payload);
                    XCTAssertEqualObjects(response.payload, [resultURL path]);
                    XCTAssertNil(response.error);
                    [expectation fulfill];
                  }];

  [self waitForExpectationsWithTimeout:1.0 handler:nil];
}

- (void)testHandleRequestWithInvalidURL {
  NSString *invalidURLString = @"http://";
  NSError *expectedError = [NSError errorWithDomain:NSURLErrorDomain
                                               code:NSURLErrorBadURL
                                           userInfo:nil];

  OCMStub([self.mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:[NSNull null], expectedError, nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:invalidURLString];

  XCTestExpectation *expectation = [self expectationWithDescription:@"Completion handler called"];
  [self.loader handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.error, @"Error should not be nil");
                    XCTAssertNil(response.payload, @"Payload should be nil");
                    [expectation fulfill];
                  }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleRequestWithNilRequest {
  AnimaXLoaderRequest *nilRequest = nil;

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Completion handler called for nil request"];
  [self.loader handleRequest:nilRequest
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.error, @"Error should be present for nil request");
                    XCTAssertNil(response.payload, @"There should be no payload for nil request");
                    [expectation fulfill];
                  }];
  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleRequestWithNilURL {
  AnimaXLoaderRequest *requestWithNilURL = [AnimaXLoaderRequest requestWithURL:nil];

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Completion handler called for request with nil URL"];
  [self.loader
      handleRequest:requestWithNilURL
         completion:^(AnimaXLoaderResponse *response) {
           XCTAssertNotNil(response.error, @"Error should be present for request with nil URL");
           XCTAssertNil(response.payload, @"There should be no payload for request with nil URL");
           [expectation fulfill];
         }];
  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleRequestWithEmptyURL {
  AnimaXLoaderRequest *requestWithEmptyURL = [AnimaXLoaderRequest requestWithURL:@""];

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Completion handler called for request with empty URL"];
  [self.loader
      handleRequest:requestWithEmptyURL
         completion:^(AnimaXLoaderResponse *response) {
           XCTAssertNotNil(response.error, @"Error should be present for request with empty URL");
           XCTAssertNil(response.payload, @"There should be no payload for request with empty URL");
           [expectation fulfill];
         }];
  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleNilResponse {
  OCMStub([self.mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:[NSNull null], [NSNull null], nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:@"https://example.com"];

  XCTestExpectation *expectation = [self expectationWithDescription:@"Completion handler called"];
  [self.loader handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.error, @"Error should be present");
                    [expectation fulfill];
                  }];
  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleRequestWithGenericFetcher {
  NSString *urlString = @"http://example.com/resource";
  NSData *expectedData = [@"Example Data" dataUsingEncoding:NSUTF8StringEncoding];

  OCMStub([self.mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:expectedData, [NSNull null], nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Generic fetcher completion called"];
  [self.loader handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.payload, @"Payload should not be nil");
                    XCTAssertEqualObjects(response.payload, expectedData,
                                          @"Payload should match expected data");
                    XCTAssertNil(response.error, @"Error should be nil");
                    [expectation fulfill];
                  }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testHandleRequestWithGenericFetcherError {
  NSString *urlString = @"http://example.com/resource";
  NSError *expectedError = [NSError errorWithDomain:NSURLErrorDomain
                                               code:NSURLErrorBadURL
                                           userInfo:nil];

  OCMStub([self.mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:[NSNull null], expectedError, nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Generic fetcher error completion called"];
  [self.loader handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.error, @"Error should not be nil");
                    XCTAssertNil(response.payload, @"Payload should be nil");
                    XCTAssertEqualObjects(response.error, expectedError,
                                          @"Error should match expected error");
                    [expectation fulfill];
                  }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testFallbackToLegacyFetcherWhenGenericFetcherIsNull {
  id mockContextLocal = OCMClassMock([LynxUIContext class]);
  OCMStub([mockContextLocal resourceFetcher]).andReturn(self.mockFetcher);

  id<AnimaXLoaderProtocol> loaderLocal =
      [LynxAnimaXResourceFetcherLoader loaderWithLynxUIContext:mockContextLocal];

  NSString *urlString = @"http://example.com/resource";
  NSURL *expectedURL = [NSURL URLWithString:urlString];
  NSData *expectedData = [@"Legacy Data" dataUsingEncoding:NSUTF8StringEncoding];

  OCMStub([self.mockFetcher
      loadResourceWithURL:[OCMArg isEqual:expectedURL]
                     type:LynxFetchResUnknown
               completion:([OCMArg invokeBlockWithArgs:@(YES), expectedData, [NSNull null],
                                                       [NSNull null], nil])]);

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];

  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Legacy fetcher fallback called"];
  [loaderLocal handleRequest:request
                  completion:^(AnimaXLoaderResponse *response) {
                    XCTAssertNotNil(response.payload, @"Payload should not be nil");
                    XCTAssertEqualObjects(response.payload, expectedData,
                                          @"Payload should match expected data");
                    XCTAssertNil(response.error, @"Error should be nil");
                    [expectation fulfill];
                  }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];

  [mockContextLocal stopMocking];
}

- (void)testLoaderWithLynxGenericFetcher {
  id<LynxGenericResourceFetcher> mockGenericFetcher =
      OCMProtocolMock(@protocol(LynxGenericResourceFetcher));
  NSString *urlString = @"http://example.com/resource";
  NSURL *expectedURL = [NSURL URLWithString:urlString];
  NSData *expectedData = [@"test data" dataUsingEncoding:NSUTF8StringEncoding];

  OCMStub([mockGenericFetcher
      fetchResource:[OCMArg any]
         onComplete:([OCMArg invokeBlockWithArgs:expectedData, [NSNull null], nil])]);

  LynxAnimaXResourceFetcherLoader *loader =
      [LynxAnimaXResourceFetcherLoader loaderWithLynxGenericFetcher:mockGenericFetcher];

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];
  XCTestExpectation *expectation = [self expectationWithDescription:@"Completion handler called"];

  [loader handleRequest:request
             completion:^(AnimaXLoaderResponse *response) {
               XCTAssertNotNil(response.payload, @"Payload should not be nil");
               XCTAssertEqualObjects(response.payload, expectedData,
                                     @"Payload should match expected data");
               XCTAssertNil(response.error, @"Error should be nil");
               [expectation fulfill];
             }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

- (void)testLoaderWithLynxResourceFetcher {
  id<LynxResourceFetcher> mockResourceFetcher = OCMProtocolMock(@protocol(LynxResourceFetcher));
  NSString *urlString = @"http://example.com/resource";
  NSURL *expectedURL = [NSURL URLWithString:urlString];
  NSURL *resultURL = [NSURL URLWithString:@"/tmp/user/a.json"];

  OCMStub([mockResourceFetcher
      loadResourceWithURL:[OCMArg isEqual:expectedURL]
                     type:LynxFetchResUnknown
               completion:([OCMArg invokeBlockWithArgs:@(YES), [NSNull null], [NSNull null],
                                                       resultURL, nil])]);

  LynxAnimaXResourceFetcherLoader *loader =
      [LynxAnimaXResourceFetcherLoader loaderWithLynxResourceFetcher:mockResourceFetcher];

  AnimaXLoaderRequest *request = [AnimaXLoaderRequest requestWithURL:urlString];
  XCTestExpectation *expectation = [self expectationWithDescription:@"Completion handler called"];

  [loader handleRequest:request
             completion:^(AnimaXLoaderResponse *response) {
               XCTAssertNotNil(response.payload, @"Payload should not be nil");
               XCTAssertEqualObjects(response.payload, [resultURL path],
                                     @"Payload should match expected path");
               XCTAssertNil(response.error, @"Error should be nil");
               [expectation fulfill];
             }];

  [self waitForExpectationsWithTimeout:5.0 handler:nil];
}

@end
