// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <malloc/malloc.h>
#include <objc/runtime.h>

@interface LynxRendererUnitTest : XCTestCase
@end

@implementation LynxRendererUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testUpdateDisplayList {
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:nil andSign:1];
  lynx::tasm::DisplayList list;
  [renderer updateDisplayList:&list];
  XCTAssertEqual([renderer getDisplayList], &list);
}

- (void)testGetSign {
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:nil andSign:1];
  XCTAssertEqual([renderer getSign], 1);
}

@end
