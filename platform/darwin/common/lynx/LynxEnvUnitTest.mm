// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <XCTest/XCTest.h>

#include "base/include/memory/memory_pressure_level.h"
#include "base/include/notification_center.h"

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
  lynx::base::NotificationCallback listener(
      lynx::base::MEMORY_PRESSURE_NOTIFICATION,
      [&callCount](const std::string& tag, intptr_t data) {
        if (static_cast<lynx::base::MemoryPressureLevel>(data) ==
            lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL) {
          callCount++;
        }
      });

  [[LynxEnv sharedInstance] trimMemory:LynxMemoryPressureLevelCritical];

  XCTAssert(callCount == 1);
}

@end
