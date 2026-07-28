// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>

#import "LynxLogContext+Internal.h"

@interface LynxLogContextUnitTest : XCTestCase

@end

@implementation LynxLogContextUnitTest

- (void)testFormatsCanonicalTuple {
  LynxLogContext *context = [[LynxLogContext alloc] initWithViewId:-1
                                                          engineId:0
                                                         runtimeId:2147483647];

  XCTAssertEqual(context.viewId, -1);
  XCTAssertEqual(context.engineId, 0);
  XCTAssertEqual(context.runtimeId, 2147483647);
  XCTAssertEqualObjects(context.description, @"[-1,0,2147483647]");
}

@end
