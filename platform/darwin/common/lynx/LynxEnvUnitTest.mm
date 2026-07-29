// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <XCTest/XCTest.h>

#include "base/include/memory/memory_pressure_level.h"
#include "base/include/notification_center.h"

#include <atomic>
#include <memory>

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
      [&callCount](const std::string &tag, intptr_t data) {
        if (static_cast<lynx::base::MemoryPressureLevel>(data) ==
            lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL) {
          callCount++;
        }
      });

  [[LynxEnv sharedInstance] trimMemory:LynxMemoryPressureLevelCritical];

  XCTAssert(callCount == 1);
}

- (void)testInitStateIsCompletedAfterSharedInstanceCreated {
  LynxEnv *env = [LynxEnv sharedInstance];

  XCTAssertTrue([env isInitCompleted]);
}

- (void)testInitStateCanBeReadAcrossThreads {
  LynxEnv *env = [LynxEnv sharedInstance];
  dispatch_group_t group = dispatch_group_create();
  dispatch_queue_t queue = dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0);
  auto sawIncompleteState = std::make_shared<std::atomic_bool>(false);

  for (int i = 0; i < 8; i++) {
    dispatch_group_async(group, queue, ^{
      for (int j = 0; j < 1000; j++) {
        if (![env isInitCompleted]) {
          sawIncompleteState->store(true, std::memory_order_relaxed);
        }
      }
    });
  }

  long waitResult = dispatch_group_wait(group, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));

  XCTAssertEqual(waitResult, 0);
  XCTAssertFalse(sawIncompleteState->load(std::memory_order_relaxed));
}

- (void)testInitCompletionHandlerRunsOnMainThreadAfterInitCompleted {
  XCTestExpectation *completionExpectation =
      [self expectationWithDescription:@"LynxEnv initialization completion"];
  XCTestExpectation *mainQueueDrainedExpectation =
      [self expectationWithDescription:@"main queue drained"];
  dispatch_semaphore_t initReturned = dispatch_semaphore_create(0);
  LynxEnv *env = [LynxEnv alloc];
  auto completionCount = std::make_shared<std::atomic_int>(0);
  auto completionSawInitCompleted = std::make_shared<std::atomic_bool>(false);

  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    [env init:^{
      completionSawInitCompleted->store([env isInitCompleted], std::memory_order_relaxed);
      completionCount->fetch_add(1, std::memory_order_relaxed);
      long waitResult =
          dispatch_semaphore_wait(initReturned, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC));
      XCTAssertEqual(waitResult, 0);
      XCTAssertTrue(NSThread.isMainThread);
      [completionExpectation fulfill];
    }];
    dispatch_semaphore_signal(initReturned);
    dispatch_async(dispatch_get_main_queue(), ^{
      [mainQueueDrainedExpectation fulfill];
    });
  });

  [self waitForExpectations:@[ completionExpectation, mainQueueDrainedExpectation ] timeout:10];
  XCTAssertEqual(completionCount->load(std::memory_order_relaxed), 1);
  XCTAssertTrue(completionSawInitCompleted->load(std::memory_order_relaxed));
}

@end
