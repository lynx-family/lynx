// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceResourceProtocol.h>
#import <Lynx/LynxUIOwner.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxResourceModule.h"

@interface LynxResourceModule (Testing)
- (void)requestResourcePrefetch:(NSDictionary *)params callback:(LynxCallbackBlock)callback;
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

  [module requestResourcePrefetch:requestData callback:nil];

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
                         }];

  OCMVerifyAll(mockFontFaceManager);
}

@end
