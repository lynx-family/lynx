// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxEventReporter.h"
#import "LynxFeatureCounter.h"
#include "core/services/feature_count/global_feature_counter.h"

@interface LynxFeatureCounterUnitTest : XCTestCase

@end

@implementation LynxFeatureCounterUnitTest

- (void)setUp {
  lynx::tasm::LynxEnv::GetInstance()
      .external_env_map_[lynx::tasm::LynxEnv::Key::ENABLE_FEATURE_COUNTER] = "true";
  lynx::tasm::report::GlobalFeatureCounter::Instance().enable_ = true;
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testCount {
  int32_t instanceId = 1000;
  [LynxFeatureCounter count:LynxFeatureObjcImplicitAnimation instanceId:instanceId];
  id mock = OCMClassMock([LynxEventReporter class]);
  id propsChecker = [OCMArg checkWithBlock:^BOOL(NSDictionary *props) {
    const char *name = lynx::tasm::report::LynxFeatureToString(
        static_cast<lynx::tasm::report::LynxFeature>(LynxFeatureObjcImplicitAnimation));
    NSString *propName = [NSString stringWithUTF8String:name];
    XCTAssertTrue([[props objectForKey:propName] boolValue]);
    return YES;
  }];
  OCMExpect([mock onEvent:@"lynxsdk_feature_count_event" instanceId:instanceId props:propsChecker]);
  lynx::tasm::report::GlobalFeatureCounter::ClearAndReport(instanceId);
  sleep(1);
  OCMVerifyAll(mock);
  [mock stopMocking];
}

@end
