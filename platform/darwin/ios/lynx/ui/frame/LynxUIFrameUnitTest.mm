// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <Lynx/LynxFrameView.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIFrame.h>
#import <Lynx/LynxUIOwner.h>
#import <Lynx/LynxView+Internal.h>
#import <XCTest/XCTest.h>

#include "base/include/value/table.h"

@interface LynxFrameView (Testing)
- (BOOL)ensureRenderCreated;
@end

@interface LynxUIFrame (Testing)
- (void)attachPageUICallback;
@end

@interface LynxUIFrameUnitTest : XCTestCase
@end

@implementation LynxUIFrameUnitTest

- (LynxTemplateData *)pendingInitDataForUIFrame:(LynxUIFrame *)uiFrame {
  return [uiFrame.view valueForKey:@"initData"];
}

- (void)testDataDictionaryStoresPendingTemplateData {
  LynxUIFrame *uiFrame = [[LynxUIFrame alloc] initWithView:nil];

  [LynxPropsProcessor updateProp:@{@"value" : @2} withKey:@"data" forUI:uiFrame];

  LynxTemplateData *pendingData = [self pendingInitDataForUIFrame:uiFrame];
  XCTAssertNotNil(pendingData);
  XCTAssertEqual(2, [[pendingData dictionary][@"value"] intValue]);
}

- (void)testDataPointerStoresPendingTemplateData {
  LynxUIFrame *uiFrame = [[LynxUIFrame alloc] initWithView:nil];
  auto table = lynx::lepus::Dictionary::Create();
  table->SetValue("value", lynx::lepus::Value(3));
  auto *value = new lynx::lepus::Value(table);
  auto ptr = reinterpret_cast<NSInteger>(value);
  XCTAssertNotEqual(ptr, 0);

  [LynxPropsProcessor updateProp:@(ptr) withKey:@"data" forUI:uiFrame];

  LynxTemplateData *pendingData = [self pendingInitDataForUIFrame:uiFrame];
  XCTAssertNotNil(pendingData);
  XCTAssertEqual(3, [[pendingData dictionary][@"value"] intValue]);
}

- (void)testNestedFrameInheritsDirectHostThreadStrategy {
  LynxView *rootView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder *builder) {
    [builder setThreadStrategyForRender:LynxThreadStrategyForRenderAllOnUI];
  }];
  LynxUIFrame *parent = [[LynxUIFrame alloc] initWithView:nil];
  parent.context = rootView.templateRender.uiOwner.uiContext;
  [LynxPropsProcessor updateProp:@1 withKey:@"thread-strategy" forUI:parent];
  XCTAssertTrue([parent.view ensureRenderCreated]);
  XCTAssertEqual(LynxThreadStrategyForRenderMostOnTASM, [parent.view getThreadStrategyForRender]);

  LynxTemplateRender *parentRender = [parent.view valueForKey:@"render"];
  LynxUIFrame *child = [[LynxUIFrame alloc] initWithView:nil];
  child.context = parentRender.uiOwner.uiContext;
  XCTAssertTrue([child.view ensureRenderCreated]);
  XCTAssertEqual(LynxThreadStrategyForRenderMostOnTASM, [child.view getThreadStrategyForRender]);
}

- (void)testDestroyFrameReleasesChildPageFromParent {
  LynxView *rootView = [[LynxView alloc] init];
  LynxRootUI *rootUI = [[LynxRootUI alloc] initWithLynxView:rootView];
  LynxUIContext *context = rootView.templateRender.uiOwner.uiContext;
  context.rootUI = rootUI;
  __weak LynxRootUI *weakChildPage;
  @autoreleasepool {
    LynxUIFrame *frame = [[LynxUIFrame alloc] initWithView:nil];
    frame.context = context;
    [frame attachPageUICallback];
    attachLynxPageUI attach = [frame.view valueForKey:@"attachLynxPageUICallback"];
    LynxRootUI *childPage = [[LynxRootUI alloc] initWithLynxView:frame.view];
    weakChildPage = childPage;
    attach(childPage);
    XCTAssertEqual(rootUI.childrenLynxPageUI.count, 1u);
  }
  XCTAssertEqual(rootUI.childrenLynxPageUI.count, 0u);
  XCTAssertNil(weakChildPage);
}

@end
