// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxDisplayListApplier+Internal.h>
#import <Lynx/LynxRenderer+Internal.h>
#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererHost.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import <malloc/malloc.h>
#include <objc/runtime.h>
#include "core/renderer/dom/fragment/display_list.h"

@interface LynxRenderer (Testing)
- (void)ensureLynxDisplayListApplier;
@end

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
  id host = OCMProtocolMock(@protocol(LynxRendererHost));
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:host andSign:1];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);
  OCMStub([mockApplier initWithView:host]).andReturn(mockApplier);

  lynx::tasm::DisplayList list;
  [[mockApplier expect] applyDisplayList:&list];

  [renderer updateDisplayList:&list];

  XCTAssertEqual([renderer getDisplayList], &list);
  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testEnsureLynxDisplayListApplier {
  id host = OCMProtocolMock(@protocol(LynxRendererHost));
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:host andSign:1];

  id mockApplier = OCMClassMock([LynxDisplayListApplier class]);
  OCMStub([mockApplier alloc]).andReturn(mockApplier);

  // Verify initWithView: is called with the host
  [[[mockApplier expect] andReturn:mockApplier] initWithView:host];

  [renderer ensureLynxDisplayListApplier];

  [mockApplier verify];
  [mockApplier stopMocking];
}

- (void)testGetSign {
  LynxRenderer* renderer = [[LynxRenderer alloc] initWithRenderHost:nil andSign:1];
  XCTAssertEqual([renderer getSign], 1);
}

@end
