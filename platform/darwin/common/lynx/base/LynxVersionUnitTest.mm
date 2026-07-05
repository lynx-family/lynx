// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxVersion.h>
#import <XCTest/XCTest.h>

#include "core/renderer/tasm/config.h"

@interface LynxVersionUnitTest : XCTestCase

@end

@implementation LynxVersionUnitTest

- (void)testLynxVersion {
  NSString *currentVersion =
      [NSString stringWithUTF8String:lynx::tasm::Config::GetCurrentLynxVersion().c_str()];
  XCTAssertEqualObjects(LynxVersion.versionString, currentVersion);
}

@end
