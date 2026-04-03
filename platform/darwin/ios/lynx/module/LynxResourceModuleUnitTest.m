// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceResourceProtocol.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxUIOwner.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxResourceModule.h"

@interface LynxResourceModule (Testing)
- (void)requestResourcePrefetch:(NSDictionary *)params
                       callback:(nullable LynxCallbackBlock)callback
                         config:(nullable NSDictionary *)config;
@end

@interface LynxResourceModuleUnitTest : XCTestCase {
  LynxResourceModule *module;
  id mockContext;
  id mockUIOwner;
  id mockFontFaceContext;
  id mockFontFaceManager;
}
@end

@implementation LynxResourceModuleUnitTest

- (void)setUp {
  [super setUp];
  mockContext = OCMClassMock([LynxContext class]);
  mockUIOwner = OCMClassMock([LynxUIOwner class]);
  mockFontFaceContext = OCMClassMock([LynxFontFaceContext class]);

  // Setup Context -> UIOwner -> FontFaceContext chain
  OCMStub([mockContext uiOwner]).andReturn(mockUIOwner);
  OCMStub([mockUIOwner fontFaceContext]).andReturn(mockFontFaceContext);

  // Mock LynxFontFaceManager singleton
  mockFontFaceManager = OCMClassMock([LynxFontFaceManager class]);
  OCMStub([mockFontFaceManager sharedManager]).andReturn(mockFontFaceManager);
  OCMStub([mockContext runOnJSThread:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    __unsafe_unretained dispatch_block_t task = nil;
    [invocation getArgument:&task atIndex:2];
    if (task) {
      task();
    }
  });

  module = [[LynxResourceModule alloc] initWithLynxContext:mockContext];
}

- (void)tearDown {
  [mockFontFaceManager stopMocking];
  module = nil;
  [super tearDown];
}

- (void)testRequestResourcePrefetch_Font_Success {
  NSString *uri = @"http://example.com/font.ttf";
  NSString *fontFamily = @"MyFont";
  NSDictionary *params = @{@"font-family" : fontFamily};

  NSDictionary *requestData =
      @{@"data" : @[ @{@"uri" : uri, @"type" : @"font", @"params" : params} ]};

  // We expect generateFontWithSize to be called on the manager
  // Since we create a temp context inside, we accept any LynxFontFaceContext instance
  OCMExpect([mockFontFaceManager generateFontWithSize:12
                                               weight:UIFontWeightRegular
                                                style:LynxFontStyleNormal
                                       fontFamilyName:fontFamily
                                      fontFaceContext:[OCMArg any]
                                     fontFaceObserver:[OCMArg any]]);

  [module requestResourcePrefetch:requestData callback:nil config:nil];

  OCMVerifyAll(mockFontFaceManager);
}

- (void)testRequestResourcePrefetch_Font_MissingFamily {
  NSString *uri = @"http://example.com/font.ttf";
  NSDictionary *params = @{};  // Missing font-family

  NSDictionary *requestData =
      @{@"data" : @[ @{@"uri" : uri, @"type" : @"font", @"params" : params} ]};

  // Should call generateFontWithSize with uri as fontFamilyName
  [[mockFontFaceManager expect] generateFontWithSize:12
                                              weight:UIFontWeightRegular
                                               style:LynxFontStyleNormal
                                      fontFamilyName:uri
                                     fontFaceContext:[OCMArg any]
                                    fontFaceObserver:[OCMArg any]];

  [module requestResourcePrefetch:requestData
                         callback:^(id result) {
                           NSDictionary *res = result;
                           NSArray *details = res[@"details"];
                           XCTAssertEqual(details.count, 1);
                           NSDictionary *item = details[0];
                           NSNumber *code = item[@"code"];
                           NSString *msg = item[@"msg"];

                           // Verify success
                           XCTAssertEqual([code integerValue], 0);
                           XCTAssertEqualObjects(msg, @"");
                         }
                           config:nil];

  OCMVerifyAll(mockFontFaceManager);
}

- (void)testRequestResourcePrefetch_Font_AwaitComplete_Success {
  NSString *uri = @"http://example.com/font.ttf";
  NSString *fontFamily = @"MyFont";
  NSDictionary *params = @{@"font-family" : fontFamily};
  NSDictionary *requestData =
      @{@"data" : @[ @{@"uri" : uri, @"type" : @"font", @"params" : params} ]};
  NSDictionary *config = @{@"awaitComplete" : @YES, @"awaitTimeout" : @10000};

  XCTestExpectation *expectation = [self expectationWithDescription:@"await complete callback"];

  [[[mockFontFaceManager stub] ignoringNonObjectArgs] generateFontWithSize:12
                                                                    weight:UIFontWeightRegular
                                                                     style:LynxFontStyleNormal
                                                            fontFamilyName:fontFamily
                                                           fontFaceContext:[OCMArg any]
                                                          fontFaceObserver:[OCMArg any]
                                                           didRequestAsync:NULL];

  [module requestResourcePrefetch:requestData
                         callback:^(id result) {
                           NSDictionary *res = result;
                           XCTAssertEqualObjects(res[@"code"], @(0));
                           XCTAssertEqualObjects(res[@"msg"], @"");

                           NSArray *details = res[@"details"];
                           XCTAssertEqual(details.count, 1);

                           NSDictionary *item = details[0];
                           XCTAssertEqualObjects(item[@"uri"], uri);
                           XCTAssertEqualObjects(item[@"type"], @"font");
                           XCTAssertEqualObjects(item[@"code"], @(0));
                           XCTAssertEqualObjects(item[@"msg"], @"");
                           [expectation fulfill];
                         }
                           config:config];

  [self waitForExpectations:@[ expectation ] timeout:1];
  [[[mockFontFaceManager verify] ignoringNonObjectArgs] generateFontWithSize:12
                                                                      weight:UIFontWeightRegular
                                                                       style:LynxFontStyleNormal
                                                              fontFamilyName:fontFamily
                                                             fontFaceContext:[OCMArg any]
                                                            fontFaceObserver:[OCMArg any]
                                                             didRequestAsync:NULL];
}

- (void)testRequestResourcePrefetch_Font_AwaitTimeout {
  NSString *uri = @"http://example.com/font.ttf";
  NSString *fontFamily = @"MyFont";
  NSDictionary *params = @{@"font-family" : fontFamily};
  NSDictionary *requestData =
      @{@"data" : @[ @{@"uri" : uri, @"type" : @"font", @"params" : params} ]};
  NSDictionary *config = @{@"awaitComplete" : @YES, @"awaitTimeout" : @1};

  XCTestExpectation *expectation = [self expectationWithDescription:@"await timeout callback"];

  [[[[mockFontFaceManager stub] ignoringNonObjectArgs] andDo:^(NSInvocation *invocation) {
    BOOL *didRequestAsync = NULL;
    [invocation getArgument:&didRequestAsync atIndex:8];
    if (didRequestAsync) {
      *didRequestAsync = YES;
    }
  }] generateFontWithSize:12
                   weight:UIFontWeightRegular
                    style:LynxFontStyleNormal
           fontFamilyName:fontFamily
          fontFaceContext:[OCMArg any]
         fontFaceObserver:[OCMArg any]
          didRequestAsync:NULL];

  [module requestResourcePrefetch:requestData
                         callback:^(id result) {
                           NSDictionary *res = result;
                           XCTAssertEqualObjects(res[@"code"], @(ECLynxResourceModuleAwaitTimeout));
                           XCTAssertEqualObjects(
                               res[@"msg"],
                               @"The prefetch task did not complete within the specified timeout.");

                           NSArray *details = res[@"details"];
                           XCTAssertEqual(details.count, 0);
                           [expectation fulfill];
                         }
                           config:config];

  [self waitForExpectations:@[ expectation ] timeout:1];
}

@end
