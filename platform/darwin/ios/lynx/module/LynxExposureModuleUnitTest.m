// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxExposureModuleUnitTest.h"
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxExposureModule.h"
#import "LynxUIExposureUnitTest.h"

@interface LynxExposureModuleUnitTest : XCTestCase {
  LynxExposureModule* module;
  LynxUIExposure* exposure;
}
@end

@implementation LynxExposureModuleUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
  module = [[LynxExposureModule alloc] init];
  module = OCMPartialMock(module);
  exposure = OCMClassMock([LynxUIExposure class]);
  OCMStub([module exposure]).andReturn(exposure);
  OCMStub([module runOnUIThreadSafely:([OCMArg invokeBlockWithArgs:module, nil])]);
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
  module = nil;
  exposure = nil;
}

- (void)testStopExposure {
  OCMExpect([exposure stopExposure:[OCMArg any]]);
  [module stopExposure:@{@"sendEvent" : @NO}];
  [module stopExposure:@{@"sendEvent" : @YES}];
  OCMVerifyAll(exposure);
}

- (void)testResumeExposure {
  OCMExpect([exposure resumeExposure]);
  [module resumeExposure];
  OCMVerifyAll(exposure);
}

@end
