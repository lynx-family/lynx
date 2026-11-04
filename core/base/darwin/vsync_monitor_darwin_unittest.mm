// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <QuartzCore/CADisplayLink.h>
#import <XCTest/XCTest.h>
#import <objc/runtime.h>

#include <functional>
#include <memory>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/thread.h"
#include "core/base/darwin/vsync_monitor_darwin.h"

// Drive native frames explicitly so assertions do not depend on simulator
// display timing. The factory still records where the production source creates
// and registers its display link.
@interface LynxTestVSyncDisplayLink : NSObject
@property(nonatomic, strong) id target;
@property(nonatomic) SEL selector;
@property(nonatomic) CFTimeInterval timestamp;
@property(nonatomic) CFTimeInterval targetTimestamp;
@property(nonatomic) CFTimeInterval duration;
@property(nonatomic) BOOL paused;
@property(nonatomic) CAFrameRateRange preferredFrameRateRange;
@property(nonatomic) NSInteger preferredFramesPerSecond;
@property(nonatomic, strong) NSRunLoop *registeredRunLoop;
@property(nonatomic, copy) NSString *registeredMode;
@property(nonatomic) BOOL createdOnMain;
@property(nonatomic) BOOL registeredOnMain;
@property(nonatomic) BOOL requestedOnMain;
@property(nonatomic) BOOL refreshRateSetOnMain;
@property(nonatomic) BOOL invalidatedOnMain;
@property(nonatomic) BOOL invalidated;
@property(nonatomic) NSUInteger requestCount;
- (void)addToRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode;
- (void)invalidate;
- (void)fire;
- (void)fireTarget:(id)target;
@end

@implementation LynxTestVSyncDisplayLink
- (void)addToRunLoop:(NSRunLoop *)runLoop forMode:(NSString *)mode {
  self.registeredRunLoop = runLoop;
  self.registeredMode = mode;
  self.registeredOnMain = [NSThread isMainThread];
}
- (void)setPaused:(BOOL)paused {
  _paused = paused;
  if (!paused) {
    self.requestedOnMain = [NSThread isMainThread];
    self.requestCount += 1;
  }
}
- (void)setPreferredFrameRateRange:(CAFrameRateRange)range {
  _preferredFrameRateRange = range;
  self.refreshRateSetOnMain = [NSThread isMainThread];
}
- (void)setPreferredFramesPerSecond:(NSInteger)rate {
  _preferredFramesPerSecond = rate;
  self.refreshRateSetOnMain = [NSThread isMainThread];
}
- (void)invalidate {
  self.invalidated = YES;
  self.invalidatedOnMain = [NSThread isMainThread];
  self.target = nil;
}
- (void)fire {
  if (!self.paused && !self.invalidated) {
    [self fireTarget:self.target];
  }
}
- (void)fireTarget:(id)target {
  NSInvocation *invocation = [NSInvocation
      invocationWithMethodSignature:[target methodSignatureForSelector:self.selector]];
  invocation.target = target;
  invocation.selector = self.selector;
  id sender = self;
  [invocation setArgument:&sender atIndex:2];
  [invocation invoke];
}
@end

static NSMutableArray<LynxTestVSyncDisplayLink *> *testDisplayLinks;

@interface CADisplayLink (LynxVSyncMonitorTest)
+ (CADisplayLink *)lynxTestDisplayLinkWithTarget:(id)target selector:(SEL)selector;
@end

@implementation CADisplayLink (LynxVSyncMonitorTest)
+ (CADisplayLink *)lynxTestDisplayLinkWithTarget:(id)target selector:(SEL)selector {
  if (![NSStringFromClass([target class]) isEqualToString:@"LynxVSyncPulse"]) {
    return [self lynxTestDisplayLinkWithTarget:target selector:selector];
  }
  LynxTestVSyncDisplayLink *link = [LynxTestVSyncDisplayLink new];
  link.target = target;
  link.selector = selector;
  link.timestamp = 1024.0;
  link.targetTimestamp = 1024.125;
  link.duration = 0.125;
  link.createdOnMain = [NSThread isMainThread];
  @synchronized(testDisplayLinks) {
    [testDisplayLinks addObject:link];
  }
  return (CADisplayLink *)link;
}
@end

@interface VSyncMonitorDarwinTest : XCTestCase
@end

@implementation VSyncMonitorDarwinTest {
  std::unique_ptr<lynx::fml::Thread> tasmThread_;
  std::shared_ptr<lynx::base::VSyncMonitorIOS> monitor_;
}

- (void)setUp {
  [super setUp];
  XCTAssertTrue([NSThread isMainThread]);
  testDisplayLinks = [NSMutableArray new];
  method_exchangeImplementations(
      class_getClassMethod(CADisplayLink.class, @selector(displayLinkWithTarget:selector:)),
      class_getClassMethod(CADisplayLink.class, @selector(lynxTestDisplayLinkWithTarget:
                                                                               selector:)));
  tasmThread_ = std::make_unique<lynx::fml::Thread>("VSyncMonitorDarwinTest");
}

- (void)tearDown {
  [self runOnTASM:[self] { monitor_.reset(); }];
  [self drainUI];
  tasmThread_.reset();
  method_exchangeImplementations(
      class_getClassMethod(CADisplayLink.class, @selector(displayLinkWithTarget:selector:)),
      class_getClassMethod(CADisplayLink.class, @selector(lynxTestDisplayLinkWithTarget:
                                                                               selector:)));
  testDisplayLinks = nil;
  [super tearDown];
}

- (void)runOnTASM:(std::function<void()>)task {
  dispatch_semaphore_t done = dispatch_semaphore_create(0);
  tasmThread_->GetTaskRunner()->PostTask([task = std::move(task), done] {
    task();
    dispatch_semaphore_signal(done);
  });
  XCTAssertEqual(dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC)),
                 0);
}

- (void)drainUI {
  XCTestExpectation *done = [self expectationWithDescription:@"main source tasks drained"];
  dispatch_async(dispatch_get_main_queue(), ^{
    [done fulfill];
  });
  [self waitForExpectations:@[ done ] timeout:2];
}

- (void)testUISourceKeepsNativeOperationsOnMainAndDeliversOriginalTimeOnTASM {
  XCTestExpectation *frame = [self expectationWithDescription:@"frame delivered on TASM"];
  [self runOnTASM:[self, frame] {
    monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(false, true, false);
    monitor_->BindTaskRunner(tasmThread_->GetTaskRunner());
    // A high refresh rate request before Init must not create a TASM source.
    monitor_->SetHighRefreshRate();
    monitor_->Init();
    monitor_->Init();
    monitor_->ScheduleVSyncSecondaryCallback(1, [self, frame](int64_t start, int64_t target) {
      XCTAssertTrue(tasmThread_->GetTaskRunner()->RunsTasksOnCurrentThread());
      XCTAssertFalse([NSThread isMainThread]);
      XCTAssertEqual(start, 1024000000000LL);
      XCTAssertEqual(target, 1024125000000LL);
      [frame fulfill];
    });
    monitor_->ScheduleVSyncSecondaryCallback(1, [](int64_t, int64_t) {
      XCTFail(@"Duplicate subscriber must not replace the pending frame");
    });
  }];
  [self drainUI];
  XCTAssertEqual(testDisplayLinks.count, 1u);
  LynxTestVSyncDisplayLink *link = testDisplayLinks.firstObject;
  XCTAssertTrue(link.createdOnMain);
  XCTAssertTrue(link.registeredOnMain);
  XCTAssertEqual(link.registeredRunLoop, NSRunLoop.mainRunLoop);
  XCTAssertEqualObjects(link.registeredMode, NSRunLoopCommonModes);
  XCTAssertTrue(link.requestedOnMain);
  XCTAssertTrue(link.refreshRateSetOnMain);
  XCTAssertEqual(link.requestCount, 1u);
  [link fire];
  [self waitForExpectations:@[ frame ] timeout:2];
  XCTAssertTrue(link.paused);
}

- (void)testUnusedUISourceDoesNotCreateDisplayLinkOnDestruction {
  [self runOnTASM:[self] {
    monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(false, true, false);
    monitor_.reset();
  }];
  [self drainUI];
  XCTAssertEqual(testDisplayLinks.count, 0u);
}

- (void)testQueuedSourceInitializationAndRequestAreCleanedUpAfterDestruction {
  [self runOnTASM:[self] {
    monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(false, true, false);
    monitor_->BindTaskRunner(tasmThread_->GetTaskRunner());
    monitor_->Init();
    monitor_->ScheduleVSyncSecondaryCallback(
        1, [](int64_t, int64_t) { XCTFail(@"A destroyed monitor must not deliver a frame"); });
    monitor_.reset();
  }];
  [self drainUI];
  XCTAssertEqual(testDisplayLinks.count, 1u);
  LynxTestVSyncDisplayLink *link = testDisplayLinks.firstObject;
  XCTAssertTrue(link.createdOnMain);
  XCTAssertTrue(link.invalidated);
  XCTAssertTrue(link.invalidatedOnMain);
  XCTAssertNil(link.target);
}

- (void)testNativeCallbackCannotUseDestroyedMonitor {
  [self runOnTASM:[self] {
    monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(true, true, false);
    monitor_->BindTaskRunner(tasmThread_->GetTaskRunner());
    monitor_->ScheduleVSyncSecondaryCallback(1, [](int64_t, int64_t) {
      XCTFail(@"A late native callback must not enter a destroyed monitor");
    });
  }];
  [self drainUI];
  LynxTestVSyncDisplayLink *link = testDisplayLinks.firstObject;
  id retainedTarget = link.target;
  [self runOnTASM:[self] { monitor_.reset(); }];
  [self drainUI];
  XCTAssertTrue(link.invalidated);
  [link fireTarget:retainedTarget];
}

- (void)testConstructorInitializationCanDeliverAfterSharedOwnershipIsEstablished {
  XCTestExpectation *frame = [self expectationWithDescription:@"constructor initialized source"];
  [self runOnTASM:[self, frame] {
    monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(true, true, false);
    monitor_->BindTaskRunner(tasmThread_->GetTaskRunner());
    monitor_->ScheduleVSyncSecondaryCallback(1, [frame](int64_t, int64_t) { [frame fulfill]; });
  }];
  [self drainUI];
  [testDisplayLinks.firstObject fire];
  [self waitForExpectations:@[ frame ] timeout:2];
}

- (void)testLegacySourceStaysOnItsCreatingRunLoop {
  [self runOnTASM:[self] {
    monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(true, false, false);
    monitor_->BindTaskRunner(tasmThread_->GetTaskRunner());
    monitor_->Init();
    monitor_->ScheduleVSyncSecondaryCallback(1, [](int64_t, int64_t) {});
  }];
  XCTAssertEqual(testDisplayLinks.count, 1u);
  LynxTestVSyncDisplayLink *link = testDisplayLinks.firstObject;
  XCTAssertFalse(link.createdOnMain);
  XCTAssertFalse(link.registeredOnMain);
  XCTAssertNotEqual(link.registeredRunLoop, NSRunLoop.mainRunLoop);
  XCTAssertFalse(link.requestedOnMain);
}

- (void)testMergedUIRunnerCanReleaseMonitorDuringCallback {
  lynx::fml::MessageLoop::EnsureInitializedForCurrentThread();
  monitor_ = std::make_shared<lynx::base::VSyncMonitorIOS>(false, true, false);
  monitor_->BindTaskRunner(lynx::fml::MessageLoop::GetCurrent().GetTaskRunner());
  monitor_->Init();
  BOOL delivered = NO;
  monitor_->ScheduleVSyncSecondaryCallback(1, [self, &delivered](int64_t, int64_t) {
    XCTAssertTrue([NSThread isMainThread]);
    delivered = YES;
    monitor_.reset();
  });
  LynxTestVSyncDisplayLink *link = testDisplayLinks.firstObject;
  [link fire];
  XCTAssertTrue(delivered);
  XCTAssertTrue(link.invalidated);
  XCTAssertTrue(link.invalidatedOnMain);
}

@end
