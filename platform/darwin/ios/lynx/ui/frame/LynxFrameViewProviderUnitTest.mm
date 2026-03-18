// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxFrameView.h>
#import <Lynx/LynxFrameViewProvider.h>
#import <XCTest/XCTest.h>

@interface LynxFrameViewProviderUnitTest : XCTestCase
@end

@implementation LynxFrameViewProviderUnitTest

- (void)testLynxFrameViewProviderDefaultIsNil {
  LynxContext *context = [[LynxContext alloc] initWithContainerView:nil];
  XCTAssertNil(context.lynxFrameViewProvider);
}

- (void)testSetAndGetLynxFrameViewProvider {
  LynxContext *context = [[LynxContext alloc] initWithContainerView:nil];

  id<LynxFrameViewProvider> provider = OCMProtocolMock(@protocol(LynxFrameViewProvider));
  context.lynxFrameViewProvider = provider;

  XCTAssertEqual(provider, context.lynxFrameViewProvider);
}

- (void)testLynxFrameViewProviderReturnsView {
  LynxContext *context = [[LynxContext alloc] initWithContainerView:nil];

  LynxFrameView *customView = [[LynxFrameView alloc] init];
  id<LynxFrameViewProvider> provider = OCMProtocolMock(@protocol(LynxFrameViewProvider));
  OCMStub([provider getLynxFrameView:context]).andReturn(customView);

  context.lynxFrameViewProvider = provider;

  LynxFrameView *result = [provider getLynxFrameView:context];
  XCTAssertEqual(customView, result);
  XCTAssertNotNil(result);
}

- (void)testLynxFrameViewProviderReturnsNilFallsBackToDefault {
  LynxContext *context = [[LynxContext alloc] initWithContainerView:nil];

  id<LynxFrameViewProvider> provider = OCMProtocolMock(@protocol(LynxFrameViewProvider));
  OCMStub([provider getLynxFrameView:context]).andReturn(nil);

  context.lynxFrameViewProvider = provider;

  LynxFrameView *result = [provider getLynxFrameView:context];
  XCTAssertNil(result);
}

@end
