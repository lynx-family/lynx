// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryUsageQuery.h>
#import <XCTest/XCTest.h>

@interface LynxMemoryUsageQueryUnitTest : XCTestCase

@end

@implementation LynxMemoryUsageQueryUnitTest

- (void)testQueryAcceptsNilCallback {
  XCTAssertNoThrow([[LynxMemoryUsageQuery sharedInstance] queryLynxGlobalMemoryUsageAsync:nil]);
}

- (void)testQueryAcceptsNilCallbackWithInjectedTimeout {
  XCTAssertNoThrow([[LynxMemoryUsageQuery sharedInstance] queryLynxGlobalMemoryUsageAsync:nil
                                                                                timeoutMs:20]);
}

@end
