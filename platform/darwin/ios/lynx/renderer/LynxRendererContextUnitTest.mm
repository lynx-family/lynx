// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxTextRenderManager.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

@interface LynxRendererContextUnitTest : XCTestCase
@end

@implementation LynxRendererContextUnitTest

- (void)testProperties {
  LynxRendererContext *context = [[LynxRendererContext alloc] init];

  // Test bodyView property
  id mockBodyView = OCMProtocolMock(@protocol(LUIBodyView));
  context.bodyView = mockBodyView;
  XCTAssertEqual(context.bodyView, mockBodyView);

  // Test textRenderManager property
  id mockTextRenderManager = OCMClassMock([LynxTextRenderManager class]);
  context.textRenderManager = mockTextRenderManager;
  XCTAssertEqual(context.textRenderManager, mockTextRenderManager);
}

@end
