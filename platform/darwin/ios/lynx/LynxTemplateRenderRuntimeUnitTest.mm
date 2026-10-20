// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxLogicExecutor.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxThreadSafeDictionary.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <XCTest/XCTest.h>

#include <atomic>

#import "LynxTemplateRender+Protected.h"

@interface LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest;
- (LynxShadowNodeOwner *)shadowNodeOwnerForTest;
@end

@implementation LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest {
  return shell_.get();
}

- (LynxShadowNodeOwner *)shadowNodeOwnerForTest {
  return _shadowNodeOwner;
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
