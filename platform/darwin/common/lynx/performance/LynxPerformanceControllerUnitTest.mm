// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <Foundation/Foundation.h>
#import <Lynx/LynxPerformanceEntryConverter.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#include <memory>
#import "LynxPerformanceController+Native.h"
#include "base/include/closure.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/darwin/performance_controller_darwin.h"

using namespace lynx::tasm::performance;

class MockPerformanceController : public PerformanceController {
 public:
  using AsyncCompletion = lynx::base::MoveOnlyClosure<void, lynx::lepus::Value>;
  void OnPerformanceEvent(const std::unique_ptr<lynx::pub::Value> entry) override {
    lynx::lepus::Value lepus_entry = lynx::pub::ValueUtils::ConvertValueToLepusValue(*entry);
    async_completion_(std::move(lepus_entry));
  }
  AsyncCompletion async_completion_;
};

@interface LynxPerformanceControllerUnitTest : XCTestCase {
  std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>> _actor;
}
@property(nonatomic, strong) id<LynxPerformanceObserverProtocol> mockObserver;
@property(nonatomic, strong) LynxPerformanceController* controller;

@end

@implementation LynxPerformanceControllerUnitTest

- (void)setUp {
  [super setUp];
  lynx::tasm::performance::MemoryMonitor::SetForceEnable(true);
  self.mockObserver = OCMProtocolMock(@protocol(LynxPerformanceObserverProtocol));
  self.controller = [[LynxPerformanceController alloc] initWithObserver:self.mockObserver];
  std::unique_ptr<PerformanceController> controller = std::make_unique<MockPerformanceController>();
  _actor = std::make_shared<lynx::shell::LynxActor<PerformanceController>>(
      std::move(controller), lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner());
  [self.controller setNativeActor:_actor];
}

- (void)tearDown {
  self.mockObserver = nil;
  self.controller = nil;
  _actor.reset();
  [super tearDown];
}

#pragma mark - Initialization Tests

- (void)testInitWithObserver_SetsObserverCorrectly {
  XCTAssertEqual(self.controller.observer, self.mockObserver);
}

#pragma mark - Memory Monitor Tests

- (void)verifyAllocateMemoryWithCategory:(NSString*)category
                                  sizeKb:(float)sizeKb
                             description:(NSString*)description {
  XCTestExpectation* expectation = [self expectationWithDescription:description];

  MockPerformanceController* controller =
      reinterpret_cast<MockPerformanceController*>(_actor->Impl());
  NSDictionary* entry_dict;
  controller->async_completion_ = [&expectation, &entry_dict](lynx::lepus::Value r) mutable {
    entry_dict = lynx::tasm::convertLepusValueToNSObject(r);
    [expectation fulfill];  // trigger
  };
  // invoke
  [self.controller allocateMemoryWithCategory:category sizeKb:sizeKb detail:nil];

  // wait 1 second
  [self waitForExpectations:@[ expectation ] timeout:1.0];

  // verify
  float totalSizeKb = [[entry_dict objectForKey:@"sizeKb"] floatValue];
  XCTAssertEqual(totalSizeKb, sizeKb);
  NSDictionary* categoryInfo = [[entry_dict objectForKey:@"detail"] objectForKey:category];
  XCTAssertEqualObjects([categoryInfo objectForKey:@"category"], category);
  XCTAssertEqual([[categoryInfo objectForKey:@"sizeKb"] floatValue], sizeKb);
}

- (void)testAllocateMemory {
  [self verifyAllocateMemoryWithCategory:@"TestCategory"
                                  sizeKb:100.0f
                             description:@"testAllocateMemory"];
}

- (void)testDeallocate {
  NSString* category = @"TestCategory";
  float sizeKb = 100.0f;
  [self verifyAllocateMemoryWithCategory:category sizeKb:sizeKb description:@"testAllocateMemory"];

  XCTestExpectation* expectation = [self expectationWithDescription:@"Wait for async call"];

  MockPerformanceController* controller =
      reinterpret_cast<MockPerformanceController*>(_actor->Impl());
  NSDictionary* entry_dict;
  controller->async_completion_ = [&expectation, &entry_dict](lynx::lepus::Value r) mutable {
    entry_dict = lynx::tasm::convertLepusValueToNSObject(r);
    [expectation fulfill];  // trigger
  };

  float sizeKb20 = 20.0f;
  [self.controller deallocateMemoryWithCategory:category sizeKb:sizeKb20 detail:nil];
  [self waitForExpectations:@[ expectation ] timeout:1.0];

  // verify
  float totalSizeKb = [[entry_dict objectForKey:@"sizeKb"] floatValue];
  XCTAssertEqual(totalSizeKb, (sizeKb - sizeKb20));
  NSDictionary* categoryInfo = [[entry_dict objectForKey:@"detail"] objectForKey:category];
  XCTAssertEqualObjects([categoryInfo objectForKey:@"category"], category);
  XCTAssertEqual([[categoryInfo objectForKey:@"sizeKb"] floatValue], (sizeKb - sizeKb20));
}

- (void)testUpdateMemoryUsage {
  XCTestExpectation* expectation = [self expectationWithDescription:@"Wait for async call"];

  MockPerformanceController* controller =
      reinterpret_cast<MockPerformanceController*>(_actor->Impl());
  NSDictionary* entry_dict;
  controller->async_completion_ = [&expectation, &entry_dict](lynx::lepus::Value r) mutable {
    entry_dict = lynx::tasm::convertLepusValueToNSObject(r);
    [expectation fulfill];  // trigger
  };
  NSString* category = @"TestCategory";
  float sizeKb = 100.0f;
  // invoke
  [self.controller updateMemoryUsageWithCategory:category sizeKb:sizeKb detail:nil];

  // wait 1 second
  [self waitForExpectations:@[ expectation ] timeout:1.0];

  // verify
  float totalSizeKb = [[entry_dict objectForKey:@"sizeKb"] floatValue];
  XCTAssertEqual(totalSizeKb, sizeKb);
  NSDictionary* categoryInfo = [[entry_dict objectForKey:@"detail"] objectForKey:category];
  XCTAssertEqualObjects([categoryInfo objectForKey:@"category"], category);
  XCTAssertEqual([[categoryInfo objectForKey:@"sizeKb"] floatValue], sizeKb);
}

#pragma mark - Event Forwarding Tests

- (void)testOnPerformanceEvent_ForwardsToObserver {
  LynxPerformanceEntry* entry = [[LynxPerformanceEntry alloc] init];
  [self.controller onPerformanceEvent:entry];

  OCMVerify([self.mockObserver onPerformanceEvent:entry]);
}

@end
