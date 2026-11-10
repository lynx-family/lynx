// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxViewBuilder+Internal.h>
#import <XCTest/XCTest.h>

#import "LynxCustomVsyncMonitorBridge.h"

#include <memory>

@interface LynxTestCustomVsyncMonitor : NSObject <LynxCustomVsyncMonitor>
@property(nonatomic, strong) NSMutableArray *callbacks;
@property(atomic, assign) NSUInteger requestCount;
- (LynxCustomVsyncCallback)takeCallback;
@end

@implementation LynxTestCustomVsyncMonitor

- (instancetype)init {
  if (self = [super init]) {
    _callbacks = [NSMutableArray array];
  }
  return self;
}

- (void)requestVsync:(LynxCustomVsyncCallback)callback {
  @synchronized(self) {
    self.requestCount += 1;
    [self.callbacks addObject:[callback copy]];
  }
}

- (LynxCustomVsyncCallback)takeCallback {
  @synchronized(self) {
    if (self.callbacks.count == 0) {
      return nil;
    }
    LynxCustomVsyncCallback callback = self.callbacks.firstObject;
    [self.callbacks removeObjectAtIndex:0];
    return callback;
  }
}

@end

@interface LynxCustomVsyncMonitorUnitTest : XCTestCase
@end

@implementation LynxCustomVsyncMonitorUnitTest

- (void)testNilMonitorPreservesSystemSource {
  XCTAssertEqual(lynx::platform::CreateVSyncMonitorPlatformImpl(nil), nullptr);
  LynxViewBuilder *builder = [LynxViewBuilder new];
  XCTAssertEqual([builder vsyncMonitorPlatformImpl], nullptr);
  LynxTestCustomVsyncMonitor *monitor = [LynxTestCustomVsyncMonitor new];
  [builder setCustomVsyncMonitor:monitor];
  XCTAssertNotEqual([builder vsyncMonitorPlatformImpl], nullptr);
  [builder setCustomVsyncMonitor:nil];
  XCTAssertEqual([builder vsyncMonitorPlatformImpl], nullptr);
  XCTAssertEqual(monitor.requestCount, 0U);
}

- (void)testBuilderUsesFinalMonitorWithoutRequestingTicks {
  const BOOL configureInitialMonitor[] = {NO, YES};
  for (BOOL configureInitial : configureInitialMonitor) {
    LynxViewBuilder *builder = [LynxViewBuilder new];
    LynxTestCustomVsyncMonitor *initialMonitor = [LynxTestCustomVsyncMonitor new];
    LynxTestCustomVsyncMonitor *finalMonitor = [LynxTestCustomVsyncMonitor new];
    [builder setCustomVsyncMonitor:configureInitial ? initialMonitor : nil];
    [builder setCustomVsyncMonitor:finalMonitor];
    auto source = [builder vsyncMonitorPlatformImpl];
    XCTAssertNotEqual(source, nullptr);
    if (!source) {
      continue;
    }
    XCTAssertEqual([builder vsyncMonitorPlatformImpl], source);
    XCTAssertEqual(initialMonitor.requestCount, 0U);
    XCTAssertEqual(finalMonitor.requestCount, 0U);

    int delivered = 0;
    source->RequestVSync([&](int64_t start, int64_t target) {
      XCTAssertEqual(start, 10);
      XCTAssertEqual(target, 20);
      delivered += 1;
    });
    [builder vsyncMonitorPlatformImpl]->RequestVSync(
        [&](int64_t start, int64_t target) { delivered += 1; });
    XCTAssertEqual(initialMonitor.requestCount, 0U);
    XCTAssertEqual(finalMonitor.requestCount, 1U);
    LynxCustomVsyncCallback reply = [finalMonitor takeCallback];
    XCTAssertNotNil(reply);
    if (reply) {
      reply(10, 20);
    }
    XCTAssertEqual(delivered, 2);
    XCTAssertEqual([builder vsyncMonitorPlatformImpl], source);
  }
}

- (void)testBridgeCoalescesRequestsAndPreservesNanoseconds {
  LynxTestCustomVsyncMonitor *monitor = [LynxTestCustomVsyncMonitor new];
  auto source = lynx::platform::CreateVSyncMonitorPlatformImpl(monitor);
  constexpr int64_t frameStart = INT64_C(9007199254740993);
  constexpr int64_t frameTarget = frameStart + INT64_C(8333333);
  int delivered = 0;
  source->RequestVSync([&, value = std::make_unique<int>(1)](int64_t start, int64_t target) {
    XCTAssertEqual(start, frameStart);
    XCTAssertEqual(target, frameTarget);
    delivered += *value;
  });
  source->RequestVSync([&](int64_t start, int64_t target) {
    XCTAssertEqual(start, frameStart);
    XCTAssertEqual(target, frameTarget);
    delivered += 1;
  });
  XCTAssertEqual(monitor.requestCount, 1U);

  LynxCustomVsyncCallback reply = [monitor takeCallback];
  XCTAssertNotNil(reply);
  if (!reply) {
    return;
  }
  reply(frameStart, frameTarget);
  reply(frameStart, frameTarget);
  XCTAssertEqual(delivered, 2);

  source->RequestVSync([&](int64_t start, int64_t target) { delivered += 1; });
  XCTAssertEqual(monitor.requestCount, 2U);
  reply = [monitor takeCallback];
  XCTAssertNotNil(reply);
  if (reply) {
    reply(frameStart + 1, frameTarget + 1);
  }
  XCTAssertEqual(delivered, 3);
}

- (void)testBridgeAcceptsReplyFromAnotherThread {
  LynxTestCustomVsyncMonitor *monitor = [LynxTestCustomVsyncMonitor new];
  auto source = lynx::platform::CreateVSyncMonitorPlatformImpl(monitor);
  XCTestExpectation *delivered = [self expectationWithDescription:@"background VSync reply"];
  delivered.assertForOverFulfill = YES;
  source->RequestVSync([delivered](int64_t start, int64_t target) {
    XCTAssertFalse(NSThread.isMainThread);
    XCTAssertEqual(start, 10);
    XCTAssertEqual(target, 20);
    [delivered fulfill];
  });
  LynxCustomVsyncCallback reply = [monitor takeCallback];
  XCTAssertNotNil(reply);
  if (reply) {
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
      reply(10, 20);
    });
  } else {
    [delivered fulfill];
  }
  [self waitForExpectationsWithTimeout:5 handler:nil];
}

- (void)testPendingReplyDoesNotRetainBuilderSourceOrHost {
  __weak LynxViewBuilder *weakBuilder;
  __weak LynxViewBuilder *weakChildBuilder;
  __weak LynxTestCustomVsyncMonitor *weakMonitor;
  std::weak_ptr<lynx::base::VSyncMonitorPlatformImpl> weakSource;
  LynxCustomVsyncCallback reply;
  int delivered = 0;
  @autoreleasepool {
    LynxViewBuilder *builder = [LynxViewBuilder new];
    weakBuilder = builder;
    LynxTestCustomVsyncMonitor *monitor = [LynxTestCustomVsyncMonitor new];
    weakMonitor = monitor;
    [builder setCustomVsyncMonitor:monitor];
    monitor = nil;
    auto source = [builder vsyncMonitorPlatformImpl];
    weakSource = source;
    LynxViewBuilder *childBuilder = [LynxViewBuilder new];
    weakChildBuilder = childBuilder;
    [childBuilder setVSyncMonitorPlatformImpl:source];
    XCTAssertNotNil(weakMonitor);
    source->RequestVSync([&](int64_t start, int64_t target) { delivered += 1; });
    reply = [weakMonitor takeCallback];
    source.reset();
    XCTAssertFalse(weakSource.expired());
    builder = nil;
    XCTAssertNil(weakBuilder);
    XCTAssertFalse(weakSource.expired());
    childBuilder = nil;
  }
  XCTAssertNil(weakBuilder);
  XCTAssertNil(weakChildBuilder);
  XCTAssertTrue(weakSource.expired());
  XCTAssertNil(weakMonitor);
  XCTAssertNotNil(reply);
  if (reply) {
    reply(10, 20);
  }
  XCTAssertEqual(delivered, 0);
}

- (void)testCopiedSourceSurvivesChangesToOriginalBuilder {
  enum class BuilderChange { kClear, kReplace, kRelease };
  const BuilderChange changes[] = {BuilderChange::kClear, BuilderChange::kReplace,
                                   BuilderChange::kRelease};
  for (BuilderChange change : changes) {
    @autoreleasepool {
      LynxTestCustomVsyncMonitor *monitor = [LynxTestCustomVsyncMonitor new];
      LynxViewBuilder *builder = [LynxViewBuilder new];
      [builder setCustomVsyncMonitor:monitor];
      LynxViewBuilder *childBuilder = [LynxViewBuilder new];
      auto source = [builder vsyncMonitorPlatformImpl];
      XCTAssertNotEqual(source, nullptr);
      [childBuilder setVSyncMonitorPlatformImpl:source];
      XCTAssertEqual([childBuilder vsyncMonitorPlatformImpl], source);
      if (!source || [childBuilder vsyncMonitorPlatformImpl] != source) {
        continue;
      }
      std::weak_ptr<lynx::base::VSyncMonitorPlatformImpl> sharedSource = source;
      source.reset();

      int delivered = 0;
      [builder vsyncMonitorPlatformImpl]->RequestVSync(
          [&](int64_t start, int64_t target) { delivered += 1; });
      [childBuilder vsyncMonitorPlatformImpl]->RequestVSync(
          [&](int64_t start, int64_t target) { delivered += 1; });
      XCTAssertEqual(monitor.requestCount, 1U);
      LynxCustomVsyncCallback reply = [monitor takeCallback];
      XCTAssertNotNil(reply);
      if (reply) {
        reply(10, 20);
      }
      XCTAssertEqual(delivered, 2);

      switch (change) {
        case BuilderChange::kClear:
          [builder setCustomVsyncMonitor:nil];
          XCTAssertEqual([builder vsyncMonitorPlatformImpl], nullptr);
          break;
        case BuilderChange::kReplace:
          [builder setCustomVsyncMonitor:[LynxTestCustomVsyncMonitor new]];
          XCTAssertNotEqual([builder vsyncMonitorPlatformImpl], nullptr);
          XCTAssertNotEqual([builder vsyncMonitorPlatformImpl], sharedSource.lock());
          break;
        case BuilderChange::kRelease:
          builder = nil;
          break;
      }
      XCTAssertFalse(sharedSource.expired());
      auto childSource = [childBuilder vsyncMonitorPlatformImpl];
      XCTAssertNotEqual(childSource, nullptr);
      XCTAssertEqual(childSource, sharedSource.lock());
      if (!childSource) {
        continue;
      }
      childSource->RequestVSync([&](int64_t start, int64_t target) { delivered += 1; });
      XCTAssertEqual(monitor.requestCount, 2U);
      reply = [monitor takeCallback];
      XCTAssertNotNil(reply);
      if (reply) {
        reply(30, 40);
      }
      XCTAssertEqual(delivered, 3);
    }
  }
}

@end
