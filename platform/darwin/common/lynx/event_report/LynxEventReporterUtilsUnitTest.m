// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxEventReporterUtils.h"

@interface LynxEventReporterUtilsUnitTest : XCTestCase

@end

@implementation LynxEventReporterUtilsUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testRelativePathForURL {
  NSString *templateUrl = @"/lynx/test/template.js";
  NSString *path = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                        NSUserDomainMask, YES) firstObject];
  NSString *ret = [LynxEventReporterUtils
      relativePathForURL:[NSString stringWithFormat:@"file://%@%@", path, templateUrl]];
  XCTAssertEqualObjects(ret, templateUrl);
}

@end
