// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxPerformanceEntryConverter.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#import "LynxPerformanceController+Native.h"

#include <memory>
#include "base/include/closure.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/darwin/performance_controller_darwin.h"

using namespace lynx::shell;
using namespace lynx::tasm;

class MockPerformanceSender : public performance::PerformanceEventSender {
 public:
  explicit MockPerformanceSender()
      : performance::PerformanceEventSender(std::make_shared<lynx::pub::PubValueFactoryDefault>()) {
  }
  ~MockPerformanceSender() override = default;
  void OnPerformanceEvent(std::unique_ptr<lynx::pub::Value> entry,
                          performance::EventType type) override{};
};

@interface MockObserver : NSObject <LynxPerformanceObserverProtocol>
@property(nonatomic, strong) LynxPerformanceEntry *lastEntry;
@property(nonatomic, strong) XCTestExpectation *expectation;
@end

@implementation MockObserver
- (void)onPerformanceEvent:(nonnull LynxPerformanceEntry *)entry {
  self.lastEntry = entry;
  [self.expectation fulfill];  // Fulfill
  self.expectation = nil;
}
@end

@interface LynxPerformanceControllerTests : XCTestCase {
  std::shared_ptr<PerformanceControllerActor> _actor;
}
@property(nonatomic, strong) LynxPerformanceController *controller;
@property(nonatomic, strong) id mockObserver;

@end

@implementation LynxPerformanceControllerTests

- (void)setUp {
  [super setUp];
  performance::MemoryMonitor::SetForceEnable(true);
  self.mockObserver = OCMProtocolMock(@protocol(LynxPerformanceObserverProtocol));
  self.controller = [[LynxPerformanceController alloc] initWithObserver:self.mockObserver];
  auto perfControllerDarwin =
      std::make_unique<performance::PerformanceControllerDarwin>(self.controller);
  auto perfController = std::make_unique<performance::PerformanceController>(
      std::make_unique<MockPerformanceSender>(), nullptr, 0);
  perfController->SetPlatformImpl(std::move(perfControllerDarwin));
  _actor = std::make_shared<PerformanceControllerActor>(
      std::move(perfController), performance::PerformanceController::GetTaskRunner());
  [self.controller setNativeActor:_actor];
}

- (void)asyncVerify:(void (^)(void))func
              check:(void (^)(LynxPerformanceEntry *entry))checkCallback {
  XCTestExpectation *expectation =
      [self expectationWithDescription:@"Performance event should be received via OCMock"];
  __block LynxPerformanceEntry *receivedEntry = nil;
  OCMExpect([self.mockObserver onPerformanceEvent:[OCMArg checkWithBlock:^BOOL(id obj) {
                                 XCTAssertTrue([obj isKindOfClass:[LynxPerformanceEntry class]],
                                               @"Argument should be a LynxPerformanceEntry");
                                 receivedEntry = (LynxPerformanceEntry *)obj;
                                 [expectation fulfill];
                                 return YES;
                               }]]);

  if (func) {
    func();
  }
  [self waitForExpectationsWithTimeout:5.0
                               handler:^(NSError *error) {
                                 if (error) {
                                   XCTFail(@"Expectation failed with error: %@", error);
                                 }
                               }];
  OCMVerifyAll(self.mockObserver);
  XCTAssertNotNil(receivedEntry, @"Received entry should not be nil");
  if (checkCallback) {
    checkCallback(receivedEntry);
  }
}

- (void)testInitialization {
  XCTAssertNotNil(self.controller);
  XCTAssertEqualObjects(self.controller.observer, self.mockObserver);
}

- (void)testMemoryMonitorProtocolMethods {
  NSString *entryName = @"memory";
  NSString *entryType = @"memory";
  NSString *category = @"test";
  float sizeBytes = 1024;

  // check allocateMemory
  [self
      asyncVerify:^{
        [self.controller allocateMemory:^LynxMemoryRecord * {
          return [[LynxMemoryRecord alloc] initWithCategory:category
                                                  sizeBytes:sizeBytes
                                                     detail:nil];
        }];
      }
      check:^(LynxPerformanceEntry *entry) {
        XCTAssertEqualObjects(entry.name, entryName);
        XCTAssertEqualObjects(entry.name, entryType);
      }];
  // check deallocateMemory
  [self
      asyncVerify:^{
        [self.controller deallocateMemory:^LynxMemoryRecord * {
          return [[LynxMemoryRecord alloc] initWithCategory:category
                                                  sizeBytes:sizeBytes
                                                     detail:nil];
        }];
      }
      check:^(LynxPerformanceEntry *entry) {
        XCTAssertEqualObjects(entry.name, entryName);
        XCTAssertEqualObjects(entry.name, entryType);
      }];
  // check updateMemoryUsage
  [self
      asyncVerify:^{
        [self.controller updateMemoryUsage:^LynxMemoryRecord * {
          return [[LynxMemoryRecord alloc] initWithCategory:category
                                                  sizeBytes:sizeBytes
                                                     detail:nil];
        }];
      }
      check:^(LynxPerformanceEntry *entry) {
        XCTAssertEqualObjects(entry.name, entryName);
        XCTAssertEqualObjects(entry.name, entryType);
      }];
}

- (void)testMemoryMonitorTeardownAfterAllocation {
  NSString *category = @"test";
  float sizeBytes = 1024;

  [self
      asyncVerify:^{
        [self.controller allocateMemory:^LynxMemoryRecord * {
          return [[LynxMemoryRecord alloc] initWithCategory:category
                                                  sizeBytes:sizeBytes
                                                     detail:nil];
        }];
      }
      check:^(LynxPerformanceEntry *entry) {
        XCTAssertEqualObjects(entry.name, @"memory");
        XCTAssertEqualObjects(entry.entryType, @"memory");
      }];

  _actor.reset();
}

- (void)testResetTimingBeforeReloadClearsPerformanceEntries {
  [self
      asyncVerify:^{
        [self.controller onPerformanceEvent:[[LynxPerformanceEntry alloc] init]];
      }
      check:^(LynxPerformanceEntry *entry) {
        XCTAssertNotNil(entry);
      }];

  [self.controller resetTimingBeforeReload];
  _actor->ActSync([](const std::unique_ptr<performance::PerformanceController> &) {});

  size_t entriesCount =
      _actor->ActSync([](const std::unique_ptr<performance::PerformanceController> &controller) {
        auto entries = controller->GetAllPerformanceEntries();
        return static_cast<size_t>(entries->Length());
      });
  XCTAssertEqual(entriesCount, 0u);
}

- (void)testTimingCollectorProtocolMethods {
  NSString *testKey = @"testMark";
  XCTAssertNoThrow([self.controller markTiming:testKey pipelineID:nil]);

  uint64_t testTimestamp = 123456789;
  XCTAssertNoThrow([self.controller setTiming:testTimestamp key:testKey pipelineID:nil]);

  NSString *pipelineId = @"testPipelineId";
  NSString *pipelineOrigin = @"testPipelineOrigin";
  uint64_t startTimestamp = 987654321;
  XCTAssertNoThrow([self.controller onPipelineStart:pipelineId
                                     pipelineOrigin:pipelineOrigin
                                          timestamp:startTimestamp]);

  XCTAssertNoThrow([self.controller resetTimingBeforeReload]);
}

- (void)testPerformanceObserverProtocol {
  LynxPerformanceEntry *testEntry = [[LynxPerformanceEntry alloc] init];
  [self
      asyncVerify:^{
        [self.controller onPerformanceEvent:testEntry];
      }
      check:^(LynxPerformanceEntry *entry) {
        XCTAssertEqualObjects(testEntry, entry);
      }];
}

@end
