// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <XCTest/XCTest.h>
#include "core/base/memory/memory_pressure_callback.h"

@interface LynxEnvUnitTest : XCTestCase

@end

@implementation LynxEnvUnitTest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testEnableCreateViewAsync {
  XCTAssert([[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableCreateUIAsync
                                             defaultValue:NO] == NO);

  [[LynxEnv sharedInstance] updateExternalEnvCacheForKey:@"enable_create_ui_async" withValue:@"1"];

  XCTAssert([[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableCreateUIAsync
                                             defaultValue:NO] == YES);
}

- (void)testEnableAnimationSyncTimeOpt {
  XCTAssert([[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableAnimationSyncTimeOpt
                                             defaultValue:NO] == NO);

  [[LynxEnv sharedInstance] updateExternalEnvCacheForKey:@"enable_animation_sync_time_opt"
                                               withValue:@"1"];

  XCTAssert([[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableAnimationSyncTimeOpt
                                             defaultValue:NO] == YES);
}

- (void)testTrimMemory {
  int callCount = 0;
  lynx::base::MemoryPressureCallback listener([&callCount](lynx::base::MemoryPressureLevel level) {
    if (level == lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL) {
      callCount++;
    }
  });

  [[LynxEnv sharedInstance] trimMemory:LynxMemoryPressureLevelCritical];

  XCTAssert(callCount == 1);
}

@end
