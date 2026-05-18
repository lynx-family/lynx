// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxUIFrame.h>
#import <XCTest/XCTest.h>

#include "base/include/value/table.h"

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

@end
