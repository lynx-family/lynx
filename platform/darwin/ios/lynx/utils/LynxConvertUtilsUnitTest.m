// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XCTest/XCTest.h>
#import "LynxConvertUtils.h"

@interface LynxConvertUtilsUnitTest : XCTestCase

@end

@implementation LynxConvertUtilsUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testConvertToJsonData {
  NSDictionary *dictionary = @{
    @"helloString" : @"Hello, World!",
    @"magicNumber" : @42,
    @"bool" : @(YES),
  };
  XCTAssertEqualObjects([LynxConvertUtils convertToJsonData:dictionary],
                        @"{\"bool\":true,\"helloString\":\"Hello, World!\",\"magicNumber\":42}");
}

@end
