// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxError.h>
#import <Lynx/LynxResourceFetcher.h>
#import <Lynx/LynxResourceServiceFetcher.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceResourceProtocol.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxResourceServiceFetcherUnitTest : XCTestCase

@end
@implementation LynxResourceServiceFetcherUnitTest {
  id<LynxResourceFetcher> _lynxResourceServiceFetcher;
  id _mockLynxServiceResourceProtocol;
  id _mockLynxServices;
}

- (void)setUp {
  _lynxResourceServiceFetcher = [[LynxResourceServiceFetcher alloc] init];

  // Register LynxServiceResource with the mock instance
  _mockLynxServiceResourceProtocol = OCMProtocolMock(@protocol(LynxServiceResourceProtocol));
  _mockLynxServices = OCMClassMock([LynxServices class]);
  OCMStub(
      ClassMethod([_mockLynxServices getInstanceWithProtocol:@protocol(LynxServiceResourceProtocol)
                                                       bizID:DEFAULT_LYNX_SERVICE]))
      .andReturn(_mockLynxServiceResourceProtocol);
}

- (void)tearDown {
}

- (void)testRequestAsyncWithResourceRequest {
  // Use mock data to test the success of the request
  Byte bytes[] = {1, 2, 3};
  NSInteger length = 3;
  NSData *data = [[NSData alloc] initWithBytes:bytes length:length];
  id<LynxServiceResourceResponseProtocol> resourceServiceResponse =
      OCMProtocolMock(@protocol(LynxServiceResourceResponseProtocol));
  OCMStub([resourceServiceResponse isSuccess]).andReturn(true);
  OCMStub([resourceServiceResponse data]).andReturn(data);

  id mockLynxServiceResourceRequestOperationProtocol =
      OCMProtocolMock(@protocol(LynxServiceResourceRequestOperationProtocol));

  OCMStub([_mockLynxServiceResourceProtocol
              fetchResourceAsync:[OCMArg isNotNil]
                      parameters:[OCMArg isNotNil]
                      completion:([OCMArg invokeBlockWithArgs:resourceServiceResponse,
                                                              [NSNull null], nil])])
      .andReturn(mockLynxServiceResourceRequestOperationProtocol);

  XCTestExpectation *expectation = [self expectationWithDescription:@"asyncMethod"];
  LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:@"url"];
  [_lynxResourceServiceFetcher
      requestAsyncWithResourceRequest:request
                                 type:LynxFetchResFontFace
       lynxResourceLoadCompletedBlock:^(LynxResourceResponse *_Nonnull response) {
         XCTAssertEqual(data, [response.data data]);
         [expectation fulfill];
       }];

  // wait for fulfill
  [self waitForExpectationsWithTimeout:3 handler:nil];
}

- (void)testRequestSyncWithResourceRequest {
  // Use mock data to test the success of the request
  Byte bytes[] = {1, 2, 3};
  NSInteger length = 3;
  NSData *data = [[NSData alloc] initWithBytes:bytes length:length];
  id<LynxServiceResourceResponseProtocol> resourceServiceResponse =
      OCMProtocolMock(@protocol(LynxServiceResourceResponseProtocol));
  OCMStub([resourceServiceResponse isSuccess]).andReturn(true);
  OCMStub([resourceServiceResponse data]).andReturn(data);

  OCMStub([_mockLynxServiceResourceProtocol fetchResourceSync:[OCMArg isNotNil]
                                                   parameters:[OCMArg isNotNil]
                                                        error:[OCMArg setTo:nil]])
      .andReturn(resourceServiceResponse);

  LynxResourceRequest *request = [[LynxResourceRequest alloc] initWithUrl:@"url"];
  LynxResourceResponse *response =
      [_lynxResourceServiceFetcher requestSyncWithResourceRequest:request
                                                             type:LynxFetchResFontFace];
  XCTAssertEqual(data, [response.data data]);
}

@end
