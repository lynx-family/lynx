// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxLogicExecutor.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxThreadSafeDictionary.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <Lynx/LynxViewBuilder+Internal.h>
#import <XCTest/XCTest.h>

#include <atomic>
#include <memory>

#include "core/base/threading/vsync_monitor.h"
#include "core/public/vsync_observer_interface.h"
#include "core/renderer/dom/element_manager.h"
#include "core/shell/runtime/bts/bts_runtime.h"

#import <Lynx/LynxTemplateRender+Internal.h>
#import "LynxTemplateRender+Protected.h"

using RuntimeTestBTSActor = std::shared_ptr<lynx::shell::LynxActor<lynx::shell::BTSRuntime>>;

@interface LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest;
- (LynxShadowNodeOwner *)shadowNodeOwnerForTest;
- (LynxViewBuilder *)builderForTest;
@end

@implementation LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest {
  return shell_.get();
}

- (LynxShadowNodeOwner *)shadowNodeOwnerForTest {
  return _shadowNodeOwner;
}

- (LynxViewBuilder *)builderForTest {
  return _builder;
}

@end

@interface RuntimeTestCustomVsyncMonitor : NSObject <LynxCustomVsyncMonitor>
@property(nonatomic, strong) NSMutableArray *callbacks;
@property(atomic, assign) NSUInteger requestCount;
- (LynxCustomVsyncCallback)takeCallback;
@end

@implementation RuntimeTestCustomVsyncMonitor
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

@interface RuntimeTestLogicExecutor : NSObject <LynxLogicExecutor>
@property(nonatomic, strong) NSDictionary *lastEvent;
@end

@implementation RuntimeTestLogicExecutor
- (void)onLynxEvent:(LynxView *)lynxView event:(NSDictionary *)event {
  self.lastEvent = event;
}
- (void)destroy {
}
@end

@interface LynxTemplateRenderRuntimeUnitTest : XCTestCase
@end

@implementation LynxTemplateRenderRuntimeUnitTest

- (NSArray<Class> *)rendererCreatorClasses {
  Class nativeCreatorClass = NSClassFromString(@"LynxUIRendererCreator");
  XCTAssertNotNil(nativeCreatorClass);
  if (!nativeCreatorClass) {
    return @[];
  }
  NSMutableArray<Class> *creatorClasses = [NSMutableArray arrayWithObject:nativeCreatorClass];
  Class clayCreatorClass = NSClassFromString(@"LynxUIRendererCreatorClay");
#if defined(ENABLE_CLAY)
  XCTAssertNotNil(clayCreatorClass);
#endif
  if (clayCreatorClass) {
    [creatorClasses addObject:clayCreatorClass];
  }
  return creatorClasses;
}

- (void)testViewCreatedRuntimeUsesConfiguredOrSystemVSyncSource {
  const BOOL customSourceCases[] = {NO, YES};
  for (Class creatorClass in [self rendererCreatorClasses]) {
    for (BOOL hasCustomSource : customSourceCases) {
      @autoreleasepool {
        RuntimeTestCustomVsyncMonitor *viewMonitor = [RuntimeTestCustomVsyncMonitor new];

        LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
          builder.disableMTSPoolWarmup = YES;
          builder.uiRendererCreator = [[creatorClass alloc] init];
          [builder setCustomVsyncMonitor:hasCustomSource ? viewMonitor : nil];
        }];
        XCTAssertNotNil(view);
        auto *shell = [view.templateRender shellForTest];
        XCTAssertNotEqual(shell, nullptr);
        if (!shell) {
          [view clearForDestroy];
          continue;
        }
        auto source = shell->GetVSyncMonitorPlatformImpl();
        XCTAssertEqual(source, [[view.templateRender builderForTest] vsyncMonitorPlatformImpl]);
        XCTAssertEqual(source != nullptr, hasCustomSource);
        [view clearForDestroy];
      }
    }
  }
}

- (void)testCopiedViewConfigurationSharesCustomVSyncSource {
  for (Class creatorClass in [self rendererCreatorClasses]) {
    @autoreleasepool {
      RuntimeTestCustomVsyncMonitor *monitor = [RuntimeTestCustomVsyncMonitor new];
      LynxView *parent = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
        builder.disableMTSPoolWarmup = YES;
        builder.enableJSRuntime = NO;
        builder.uiRendererCreator = [[creatorClass alloc] init];
        [builder setCustomVsyncMonitor:monitor];
      }];
      XCTAssertNotNil(parent);
      if (!parent) {
        continue;
      }
      auto source = [[parent.templateRender builderForTest] vsyncMonitorPlatformImpl];
      XCTAssertNotEqual(source, nullptr);

      LynxViewBuilderBlock copyConfiguration = [parent.templateRender getLynxViewBuilderBlock];
      XCTAssertNotNil(copyConfiguration);
      if (!copyConfiguration) {
        [parent clearForDestroy];
        continue;
      }
      LynxView *child = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
        copyConfiguration(builder);
        builder.disableMTSPoolWarmup = YES;
        builder.enableJSRuntime = NO;
        builder.uiRendererCreator = [[creatorClass alloc] init];
      }];
      XCTAssertNotNil(child);
      if (!child) {
        [parent clearForDestroy];
        continue;
      }
      XCTAssertEqual([[child.templateRender builderForTest] vsyncMonitorPlatformImpl], source);
      auto *parentShell = [parent.templateRender shellForTest];
      auto *childShell = [child.templateRender shellForTest];
      XCTAssertNotEqual(parentShell, nullptr);
      XCTAssertNotEqual(childShell, nullptr);
      if (parentShell && childShell) {
        XCTAssertEqual(parentShell->GetVSyncMonitorPlatformImpl(), source);
        XCTAssertEqual(childShell->GetVSyncMonitorPlatformImpl(), source);
      }

      [parent clearForDestroy];
      [child clearForDestroy];
    }
  }
}

- (void)assertAnimationFrameForActor:(const RuntimeTestBTSActor &)actor
                             monitor:(RuntimeTestCustomVsyncMonitor *)monitor
                          frameStart:(int64_t)frameStart
                         frameTarget:(int64_t)frameTarget {
  XCTestExpectation *registered = [self expectationWithDescription:@"BTS VSync registered"];
  XCTestExpectation *delivered = [self expectationWithDescription:@"BTS VSync delivered"];
  delivered.assertForOverFulfill = YES;
  auto runner = actor->GetRunner();
  actor->ActAsync([registered, delivered, runner, frameStart, frameTarget](auto &runtime) {
    auto observer = runtime->GetVSyncObserver();
    XCTAssertNotEqual(observer, nullptr);
    if (observer) {
      observer->RequestAnimationFrame(
          1, [delivered, runner, frameStart, frameTarget](int64_t start, int64_t target) {
            XCTAssertTrue(runner->RunsTasksOnCurrentThread());
            XCTAssertFalse(NSThread.isMainThread);
            XCTAssertEqual(start, frameStart);
            XCTAssertEqual(target, frameTarget);
            [delivered fulfill];
          });
    }
    [registered fulfill];
  });
  [self waitForExpectations:@[ registered ] timeout:5];
  LynxCustomVsyncCallback reply = [monitor takeCallback];
  XCTAssertNotNil(reply);
  if (reply) {
    // Deliver from the UI thread; the BTS callback must run on its JS runner.
    reply(frameStart, frameTarget);
    [self waitForExpectations:@[ delivered ] timeout:5];
  } else {
    [delivered fulfill];
  }
}

- (void)testViewCreatedRuntimeReceivesCustomVSyncOnJSThread {
  for (Class creatorClass in [self rendererCreatorClasses]) {
    @autoreleasepool {
      RuntimeTestCustomVsyncMonitor *monitor = [RuntimeTestCustomVsyncMonitor new];
      LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
        builder.disableMTSPoolWarmup = YES;
        builder.uiRendererCreator = [[creatorClass alloc] init];
        [builder setCustomVsyncMonitor:monitor];
      }];
      auto *shell = [view.templateRender shellForTest];
      XCTAssertNotEqual(shell, nullptr);
      if (!shell) {
        [view clearForDestroy];
        continue;
      }
      auto actor = shell->GetRuntimeActor();
      XCTAssertNotEqual(actor, nullptr);
      if (actor) {
        [self assertAnimationFrameForActor:actor
                                   monitor:monitor
                                frameStart:INT64_C(9007199254740993)
                               frameTarget:INT64_C(9007199263074326)];
      }
      [view clearForDestroy];
    }
  }
}

- (void)testDestroyWithPendingVSyncReleasesViewSourceAndHost {
  for (Class creatorClass in [self rendererCreatorClasses]) {
    __weak LynxView *weakView;
    __weak RuntimeTestCustomVsyncMonitor *weakMonitor;
    std::weak_ptr<lynx::base::VSyncMonitorPlatformImpl> weakSource;
    LynxCustomVsyncCallback staleReply;
    auto deliveries = std::make_shared<std::atomic<size_t>>(0);
    @autoreleasepool {
      RuntimeTestCustomVsyncMonitor *monitor = [RuntimeTestCustomVsyncMonitor new];
      weakMonitor = monitor;
      LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
        builder.disableMTSPoolWarmup = YES;
        builder.uiRendererCreator = [[creatorClass alloc] init];
        [builder setCustomVsyncMonitor:weakMonitor];
      }];
      weakView = view;
      auto *shell = [view.templateRender shellForTest];
      XCTAssertNotEqual(shell, nullptr);
      if (!shell) {
        [view clearForDestroy];
        continue;
      }
      weakSource = shell->GetVSyncMonitorPlatformImpl();
      auto runtimeActor = shell->GetRuntimeActor();
      auto engineActor = shell->GetEngineActor();
      XCTAssertNotEqual(runtimeActor, nullptr);
      XCTAssertNotEqual(engineActor, nullptr);
      if (!runtimeActor || !engineActor) {
        [view clearForDestroy];
        continue;
      }
      XCTestExpectation *runtimeRegistered =
          [self expectationWithDescription:@"pending BTS subscription"];
      XCTestExpectation *pageRegistered =
          [self expectationWithDescription:@"pending MTS and Core subscriptions"];
      runtimeActor->ActAsync([runtimeRegistered, deliveries](auto &runtime) {
        auto observer = runtime->GetVSyncObserver();
        XCTAssertNotEqual(observer, nullptr);
        if (observer) {
          observer->RequestAnimationFrame(1, [deliveries](int64_t, int64_t) { ++*deliveries; });
        }
        [runtimeRegistered fulfill];
      });
      engineActor->ActAsync([pageRegistered, deliveries](auto &engine) {
        auto *tasm = engine->GetTasm();
        XCTAssertNotEqual(tasm, nullptr);
        if (tasm) {
          auto pageMonitor = tasm->page_proxy()->element_manager()->vsync_monitor();
          XCTAssertNotEqual(pageMonitor, nullptr);
          if (pageMonitor) {
            pageMonitor->ScheduleVSyncSecondaryCallback(
                1, [deliveries](int64_t, int64_t) { ++*deliveries; });
          }
          tasm->GetDelegate().RequestVsync(1, [deliveries](int64_t, int64_t) { ++*deliveries; });
        }
        [pageRegistered fulfill];
      });
      [self waitForExpectations:@[ runtimeRegistered, pageRegistered ] timeout:5];
      staleReply = [monitor takeCallback];
      XCTAssertNotNil(staleReply);
      // Teardown must complete without requiring the host to deliver its signal.
      [view clearForDestroy];
      view = nil;
      monitor = nil;
      runtimeActor.reset();
      engineActor.reset();
    }
    // Native shell destruction is deferred to the main queue. Poll only weak
    // references after draining autoreleased configuration and callback objects.
    NSPredicate *released =
        [NSPredicate predicateWithBlock:^BOOL(id object, NSDictionary *bindings) {
          @autoreleasepool {
            return weakView == nil && weakMonitor == nil && weakSource.expired();
          }
        }];
    XCTestExpectation *allReleased = [self expectationForPredicate:released
                                               evaluatedWithObject:nil
                                                           handler:nil];
    [self waitForExpectations:@[ allReleased ] timeout:5];
    XCTAssertNil(weakView);
    XCTAssertNil(weakMonitor);
    XCTAssertTrue(weakSource.expired());
    if (staleReply) {
      staleReply(10, 20);
    }
    XCTAssertEqual(deliveries->load(), 0u);
  }
}

- (void)testMTSAndCoreAnimationShareCustomVSyncSource {
  constexpr int64_t frameStart = INT64_C(9007199254740993);
  constexpr int64_t frameTarget = INT64_C(9007199263074326);
  for (Class creatorClass in [self rendererCreatorClasses]) {
    @autoreleasepool {
      RuntimeTestCustomVsyncMonitor *monitor = [RuntimeTestCustomVsyncMonitor new];
      LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
        builder.disableMTSPoolWarmup = YES;
        builder.enableJSRuntime = NO;
        builder.uiRendererCreator = [[creatorClass alloc] init];
        [builder setCustomVsyncMonitor:monitor];
      }];
      auto *shell = [view.templateRender shellForTest];
      XCTAssertNotEqual(shell, nullptr);
      if (!shell) {
        [view clearForDestroy];
        continue;
      }
      auto actor = shell->GetEngineActor();
      XCTAssertNotEqual(actor, nullptr);
      if (!actor) {
        [view clearForDestroy];
        continue;
      }
      XCTestExpectation *registered =
          [self expectationWithDescription:@"page consumers registered"];
      XCTestExpectation *coreFrame = [self expectationWithDescription:@"Core animation VSync"];
      XCTestExpectation *mtsFrame = [self expectationWithDescription:@"MTS VSync"];
      coreFrame.assertForOverFulfill = YES;
      mtsFrame.assertForOverFulfill = YES;
      auto runner = actor->GetRunner();
      actor->ActAsync([registered, coreFrame, mtsFrame, runner](auto &engine) {
        auto *tasm = engine->GetTasm();
        XCTAssertNotEqual(tasm, nullptr);
        if (tasm) {
          auto pageMonitor = tasm->page_proxy()->element_manager()->vsync_monitor();
          XCTAssertNotEqual(pageMonitor, nullptr);
          if (pageMonitor) {
            XCTAssertTrue(pageMonitor->UsesCustomVSync());
            pageMonitor->ScheduleVSyncSecondaryCallback(
                1, [coreFrame, runner](int64_t start, int64_t target) {
                  XCTAssertTrue(runner->RunsTasksOnCurrentThread());
                  XCTAssertEqual(start, frameStart);
                  XCTAssertEqual(target, frameTarget);
                  [coreFrame fulfill];
                });
          }
          tasm->GetDelegate().RequestVsync(1, [mtsFrame, runner](int64_t start, int64_t target) {
            XCTAssertTrue(runner->RunsTasksOnCurrentThread());
            XCTAssertEqual(start, frameStart);
            XCTAssertEqual(target, frameTarget);
            [mtsFrame fulfill];
          });
        }
        [registered fulfill];
      });
      [self waitForExpectations:@[ registered ] timeout:5];
      XCTAssertEqual(monitor.requestCount, 1U);
      LynxCustomVsyncCallback reply = [monitor takeCallback];
      XCTAssertNotNil(reply);
      if (reply) {
        reply(frameStart, frameTarget);
        [self waitForExpectations:@[ coreFrame, mtsFrame ] timeout:5];
      } else {
        [coreFrame fulfill];
        [mtsFrame fulfill];
      }
      [view clearForDestroy];
    }
  }
}

- (void)testShadowNodeStorageRespectsRenderingMode {
  const struct {
    LynxEmbeddedMode mode;
    LynxThreadStrategyForRender strategy;
    BOOL needsThreadSafeStorage;
  } cases[] = {
      {LynxEmbeddedModeUnset, LynxThreadStrategyForRenderAllOnUI, NO},
      {LynxEmbeddedModeBase, LynxThreadStrategyForRenderAllOnUI, NO},
      {LynxEmbeddedModeUseTextService, LynxThreadStrategyForRenderAllOnUI, NO},
      {LynxEmbeddedModeLayoutInElement, LynxThreadStrategyForRenderAllOnUI, YES},
      {LynxEmbeddedModeFragmentLayerRender, LynxThreadStrategyForRenderAllOnUI, YES},
      {static_cast<LynxEmbeddedMode>(29), LynxThreadStrategyForRenderAllOnUI, YES},
      {LynxEmbeddedModeUnset, LynxThreadStrategyForRenderMostOnTASM, YES},
      {LynxEmbeddedModeUnset, LynxThreadStrategyForRenderPartOnLayout, YES},
      {LynxEmbeddedModeUnset, LynxThreadStrategyForRenderMultiThreads, YES},
  };
  for (const auto &testCase : cases) {
    LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
      builder.enableJSRuntime = NO;
      [builder setThreadStrategyForRender:testCase.strategy];
      [builder setEmbeddedMode:testCase.mode];
    }];
    LynxShadowNodeOwner *owner = [view.templateRender shadowNodeOwnerForTest];
    XCTAssertNotNil(owner);
    id nodeHolder = [owner valueForKey:@"nodeHolder"];
    XCTAssertEqual([nodeHolder isKindOfClass:LynxThreadSafeDictionary.class],
                   testCase.needsThreadSafeStorage, @"mode=%ld strategy=%ld", (long)testCase.mode,
                   (long)testCase.strategy);
  }
}

- (void)testLayoutInElementShadowNodesCanBeCreatedConcurrently {
  const LynxEmbeddedMode modes[] = {LynxEmbeddedModeLayoutInElement,
                                    LynxEmbeddedModeFragmentLayerRender, 29};
  for (auto mode : modes) {
    LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
      builder.enableJSRuntime = NO;
      [builder setThreadStrategyForRender:LynxThreadStrategyForRenderAllOnUI];
      [builder setEmbeddedMode:mode];
    }];
    LynxShadowNodeOwner *owner = [view.templateRender shadowNodeOwnerForTest];
    constexpr size_t nodeCount = 256;
    std::atomic<size_t> missingNodes{0};
    auto *missingNodesPtr = &missingNodes;
    dispatch_apply(nodeCount, dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0),
                   ^(size_t index) {
                     @autoreleasepool {
                       NSInteger sign = static_cast<NSInteger>(index + 1);
                       [owner createNodeWithSign:sign
                                           tagName:@"view"
                                             props:@{}
                                          eventSet:nil
                                     lepusEventSet:nil
                           isParentInlineContainer:true];
                       if ([owner nodeWithSign:sign] == nil) {
                         missingNodesPtr->fetch_add(1, std::memory_order_relaxed);
                       }
                     }
                   });
    XCTAssertEqual(missingNodes.load(), 0U);
    for (size_t index = 0; index < nodeCount; ++index) {
      NSInteger sign = static_cast<NSInteger>(index + 1);
      LynxShadowNode *node = [owner nodeWithSign:sign];
      XCTAssertNotNil(node);
      XCTAssertEqual(node.sign, sign);
      [owner destroyNode:sign];
      XCTAssertNil([owner nodeWithSign:sign]);
    }
    XCTAssertEqual([[owner valueForKey:@"nodeHolder"] count], 0U);
  }
}

- (void)testLogicExecutorPreservesGlobalEventsWithoutPerViewRuntime {
  RuntimeTestLogicExecutor *executor = [RuntimeTestLogicExecutor new];
  LynxViewGroup *group = [[LynxViewGroup alloc] initWithUrl:@"test://embedded"
                                             templateBundle:[LynxTemplateBundle new]];
  group.enableJSRuntime = YES;
  group.logicExecutor = executor;
  [group setEmbeddedMode:static_cast<LynxEmbeddedMode>(5)];
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.lynxViewGroup = group;
  }];

  auto *shell = [view.templateRender shellForTest];
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue([view getLynxContext].enableJSRuntime);
  XCTAssertTrue([view getLynxContext].hasLynxViewGroup);
  XCTAssertTrue(shell->IsRuntimeEnabled());
  XCTAssertTrue(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
  XCTAssertFalse(shell->GetTasm()->ShouldSendEventToMainThread());
  XCTAssertTrue(shell->GetTasm()->ShouldPostDataToJs(nullptr));
  auto config = std::make_shared<lynx::tasm::PageConfig>();
  shell->GetTasm()->SetPageConfig(config);
  for (auto mode :
       {lynx::tasm::AIR_MODE_OFF, lynx::tasm::AIR_MODE_FIBER, lynx::tasm::AIR_MODE_STRICT}) {
    config->SetLynxAirMode(mode);
    XCTAssertEqual(shell->GetTasm()->ShouldPostDataToJs(nullptr), mode == lynx::tasm::AIR_MODE_OFF);
    XCTAssertFalse(shell->GetTasm()->ShouldSendEventToMainThread());
  }
  [view sendGlobalEvent:@"runtime-test-event" withParams:@[ @"payload" ]];
  XCTAssertEqualObjects(executor.lastEvent[@"method"], kSendGlobalEvent);
  XCTAssertEqualObjects(executor.lastEvent[@"name"], @"runtime-test-event");
  XCTAssertEqualObjects(executor.lastEvent[@"params"], @[ @"payload" ]);
}

- (void)testEmbeddedModeWithoutLogicExecutorKeepsRuntime {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    [builder setEmbeddedMode:static_cast<LynxEmbeddedMode>(5)];
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertFalse([view getLynxContext].hasLynxViewGroup);
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
  auto config = std::make_shared<lynx::tasm::PageConfig>();
  shell->GetTasm()->SetPageConfig(config);
  for (auto mode :
       {lynx::tasm::AIR_MODE_OFF, lynx::tasm::AIR_MODE_FIBER, lynx::tasm::AIR_MODE_STRICT}) {
    config->SetLynxAirMode(mode);
    XCTAssertTrue(shell->GetTasm()->ShouldPostDataToJs(nullptr));
  }
}

- (void)testRegularViewKeepsRuntime {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertFalse([view getLynxContext].hasLynxViewGroup);
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
}

- (void)testDisabledRuntimeStaysDisabled {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = NO;
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertFalse(view.templateRender.enableJSRuntime);
  XCTAssertFalse(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
}

- (void)testAirStrictModeKeepsRuntimeDisabled {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    builder.enableAirStrictMode = YES;
  }];
  auto *shell = [view.templateRender shellForTest];
  XCTAssertFalse(view.templateRender.enableJSRuntime);
  XCTAssertFalse(shell->IsRuntimeEnabled());
  XCTAssertFalse(shell->GetTasm()->GetPageOptions().HasLogicExecutor());
}

@end
