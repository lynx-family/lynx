// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#import <Lynx/LynxDevToolPool.h>
#import <Lynx/LynxDevtool.h>

#define private public
#include "core/renderer/utils/devtool_lifecycle.h"

@interface LynxDevToolPoolUnitTest : XCTestCase
@property(nonatomic, strong) LynxDevToolPool *pool;
@end

@implementation LynxDevToolPoolUnitTest

- (void)setUp {
  [super setUp];
  lynx::tasm::DevToolLifecycle::GetInstance().OnAttached();
  self.pool = [[LynxDevToolPool alloc] initWithURL:@"test-url" debuggable:YES];
}

- (void)tearDown {
  self.pool = nil;
  lynx::tasm::DevToolLifecycle::GetInstance().ResetForTesting();
  [super tearDown];
}

- (void)testCreate {
  LynxDevToolPool *pool = [[LynxDevToolPool alloc] initWithURL:@"" debuggable:NO];
  XCTAssertFalse([[pool valueForKey:@"_debuggable"] boolValue]);
  XCTAssertEqual(@"anonymous bundle", [pool valueForKey:@"_url"]);
  XCTAssertNotEqual(nullptr, [pool valueForKey:@"devtool_pool_"]);

  XCTAssertTrue([[self.pool valueForKey:@"_debuggable"] boolValue]);
  XCTAssertEqual(@"test-url", [self.pool valueForKey:@"_url"]);
  XCTAssertNotEqual(nullptr, [self.pool valueForKey:@"devtool_pool_"]);
}

- (void)testCreateAndPopDevTool {
  [self.pool createDevTool];
  NSMutableArray *devtools = [self.pool valueForKey:@"_devtools"];
  XCTAssertEqual([devtools count], 0, @"Devtool list should be empty when devtool is disabled.");

  [self.pool popDevTool];
  XCTAssertEqual([devtools count], 0,
                 @"Devtool list should remain empty when popping an empty list.");

  lynx::tasm::DevToolLifecycle::GetInstance().OnEnabled();
  [self.pool createDevTool];
  XCTAssertEqual([devtools count], 1, @"Devtool list should have one item after creation.");

  [self.pool popDevTool];
  XCTAssertEqual([devtools count], 0, @"Devtool list should be empty after pop.");

  lynx::tasm::DevToolLifecycle::GetInstance().OnDisabled();
}

- (void)testOnMTSRuntimeCreated {
  // Test with empty devtools list, should not crash
  [self.pool onMTSRuntimeCreated];

  // 1. Create a mock and set up expectations
  id devtoolMock = OCMClassMock([LynxDevtool class]);
  // Stub the method to allow the call without throwing an exception.
  [[[devtoolMock stub] ignoringNonObjectArgs] onMTSRuntimeCreated:0];
  OCMStub([devtoolMock attachDebugBridge:[OCMArg any]]);

  // 2. Inject the mock into the pool
  NSMutableArray *devtools = [self.pool valueForKey:@"_devtools"];
  [devtools addObject:devtoolMock];

  // 3. Call the method under test
  [self.pool onMTSRuntimeCreated];

  // 4. Verify the interactions and results
  [[[devtoolMock verify] ignoringNonObjectArgs] onMTSRuntimeCreated:0];
  OCMVerify([devtoolMock attachDebugBridge:@"test-url - context0"]);
}

@end
