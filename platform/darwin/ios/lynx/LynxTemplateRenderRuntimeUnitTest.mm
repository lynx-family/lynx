// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxContext.h>
#import <Lynx/LynxLogicExecutor.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxView.h>
#import <XCTest/XCTest.h>

#import "LynxTemplateRender+Protected.h"

@interface LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest;
@end

@implementation LynxTemplateRender (RuntimeUnitTest)
- (lynx::shell::LynxShell *)shellForTest {
  return shell_.get();
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
