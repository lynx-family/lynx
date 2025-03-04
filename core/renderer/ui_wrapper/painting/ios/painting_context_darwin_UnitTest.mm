// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxTemplateRender.h"
#import "LynxUIOwner.h"

@interface painting_context_darwin_UnitTest : XCTestCase {
  std::unique_ptr<lynx::tasm::PaintingContextDarwin> paintingContext;
}

@end

@implementation painting_context_darwin_UnitTest

- (void)setUp {
  id templateRenderMock = OCMClassMock([LynxTemplateRender class]);
  LynxUIOwner* uiOwner = [[LynxUIOwner alloc] initWithContainerView:nil
                                                     templateRender:templateRenderMock
                                                  componentRegistry:nil
                                                      screenMetrics:nil];
  paintingContext = std::make_unique<lynx::tasm::PaintingContextDarwin>(uiOwner, false);
}

- (void)tearDown {
}

- (void)testScrollBy {
  // This is an example of a functional test case.
  // Use XCTAssert and related functions to verify your tests produce the correct results.
  auto runnable = ^{
    auto res = paintingContext->ScrollBy(1, 20, 20);
    XCTAssertEqual(res[2], 20);
  };
  runnable();
  dispatch_queue_t backgroundQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
  dispatch_async(backgroundQueue, ^{
    runnable();
  });
}

@end
