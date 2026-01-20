// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContainerView.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <malloc/malloc.h>
#include <objc/runtime.h>

@interface LynxContainerViewUnitTest : XCTestCase
@end

@implementation LynxContainerViewUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testRendererMethods {
  LynxContainerView* containerView = [[LynxContainerView alloc] init];

  LynxRenderer* renderer = [containerView createRendererWithSign:1];

  [containerView setRenderer:renderer];

  XCTAssertEqual([containerView getRenderer], renderer);
}

- (void)testGetView {
  LynxContainerView* containerView = [[LynxContainerView alloc] init];
  XCTAssertEqual([containerView getView], containerView);
}

@end
