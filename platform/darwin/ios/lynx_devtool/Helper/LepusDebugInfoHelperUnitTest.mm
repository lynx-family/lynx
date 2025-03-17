// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <LynxDevtool/LepusDebugInfoHelper.h>
#import <XCTest/XCTest.h>

@interface LepusDebugInfoHelperUnitTest : XCTestCase
@end

@implementation LepusDebugInfoHelperUnitTest {
  LepusDebugInfoHelper* _debugInfoHelper;
}

- (void)setUp {
  _debugInfoHelper = [[LepusDebugInfoHelper alloc] init];
}

- (void)testGetDebugInfo {
  std::string invalid_url = "https://error-url/debug-info.json";

  std::string result = [_debugInfoHelper getDebugInfo:invalid_url];
  XCTAssertTrue(result.empty());
  XCTAssertEqual([[_debugInfoHelper debugInfoUrl] UTF8String], invalid_url);
}

@end
