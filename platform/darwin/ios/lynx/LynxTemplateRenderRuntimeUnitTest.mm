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
- (BOOL)shellRuntimeEnabledForTest;
- (BOOL)shouldPostLepusConsoleForTest;
- (BOOL)shouldSendEventToMainThreadForTest;
- (BOOL)shouldPostDataToJSForTest;
@end

@implementation LynxTemplateRender (RuntimeUnitTest)
- (BOOL)shellRuntimeEnabledForTest {
  return shell_->IsRuntimeEnabled();
}
- (BOOL)shouldPostLepusConsoleForTest {
  return shell_->ShouldPostLepusConsole();
}
- (BOOL)shouldSendEventToMainThreadForTest {
  return shell_->GetTasm()->ShouldSendEventToMainThread();
}
- (BOOL)shouldPostDataToJSForTest {
  return shell_->GetTasm()->ShouldPostDataToJs(nullptr);
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

  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue([view getLynxContext].enableJSRuntime);
  XCTAssertTrue([view.templateRender shellRuntimeEnabledForTest]);
  XCTAssertTrue([view.templateRender shouldPostLepusConsoleForTest]);
  XCTAssertFalse([view.templateRender shouldSendEventToMainThreadForTest]);
  XCTAssertTrue([view.templateRender shouldPostDataToJSForTest]);
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
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue([view.templateRender shellRuntimeEnabledForTest]);
  XCTAssertFalse([view.templateRender shouldPostLepusConsoleForTest]);
}

- (void)testRegularViewKeepsRuntime {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
  }];
  XCTAssertTrue(view.templateRender.enableJSRuntime);
  XCTAssertTrue([view.templateRender shellRuntimeEnabledForTest]);
  XCTAssertFalse([view.templateRender shouldPostLepusConsoleForTest]);
}

- (void)testDisabledRuntimeStaysDisabled {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = NO;
  }];
  XCTAssertFalse(view.templateRender.enableJSRuntime);
  XCTAssertFalse([view.templateRender shellRuntimeEnabledForTest]);
  XCTAssertTrue([view.templateRender shouldPostLepusConsoleForTest]);
}

- (void)testAirStrictModeKeepsRuntimeDisabled {
  LynxView *view = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    builder.enableJSRuntime = YES;
    builder.enableAirStrictMode = YES;
  }];
  XCTAssertFalse(view.templateRender.enableJSRuntime);
  XCTAssertFalse([view.templateRender shellRuntimeEnabledForTest]);
  XCTAssertTrue([view.templateRender shouldPostLepusConsoleForTest]);
}

@end
