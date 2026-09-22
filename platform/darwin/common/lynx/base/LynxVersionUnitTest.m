// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxVersion.h>
#import <XCTest/XCTest.h>

@interface LynxVersion (UnitTest)

+ (NSString*)versionStringFromPodVersion:(NSString*)podVersion;

@end

@interface LynxVersionUnitTest : XCTestCase

@end

@implementation LynxVersionUnitTest

- (void)testLynxVersion {
  XCTAssertEqualObjects(LynxVersion.versionString, @"1.4.0");
}

- (void)testLynxVersionWithSixDigitAppIdPrefix {
  XCTAssertEqualObjects([LynxVersion versionStringFromPodVersion:@"999901_4.3.1-debug"],
                        @"4.3.1-debug");
}

- (void)testLynxVersionWithLegacyAppIdPrefix {
  XCTAssertEqualObjects([LynxVersion versionStringFromPodVersion:@"9999_4.3.1"], @"4.3.1");
}

- (void)testLynxVersionWithoutAppIdPrefix {
  XCTAssertEqualObjects([LynxVersion versionStringFromPodVersion:@"4.3.1"], @"4.3.1");
}

- (void)testLynxVersionWithNonNumericPrefix {
  XCTAssertEqualObjects([LynxVersion versionStringFromPodVersion:@"debug_4.3.1"], @"debug_4.3.1");
}

- (void)testLynxVersionWithEmptyVersionSuffix {
  XCTAssertEqualObjects([LynxVersion versionStringFromPodVersion:@"999901_"], @"999901_");
}

@end
