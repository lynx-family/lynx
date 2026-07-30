// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxUIRendererProtocol.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxScreenCastHelper.h>
#import <OCMock/OCMock.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/lynx_devtool/agent/input/input_event_target.h"

@interface DevToolPlatformDarwinDelegate (ScreenshotModeTesting)
- (NSString *)effectiveScreenshotMode;
@end

@interface DevToolInsertTextField : UITextField
@end

@implementation DevToolInsertTextField

- (BOOL)isFirstResponder {
  return YES;
}

@end

@interface DevToolPlatformDarwinDelegateUnitTest : XCTestCase
@end

@implementation DevToolPlatformDarwinDelegateUnitTest

- (void)testInsertTextUsesFirstResponderTextInput {
  UIView *lynxView = [[UIView alloc] initWithFrame:CGRectZero];
  UIView *container = [[UIView alloc] initWithFrame:CGRectZero];
  DevToolInsertTextField *textField = [[DevToolInsertTextField alloc] initWithFrame:CGRectZero];
  textField.text = @"ac";
  textField.selectedTextRange = [textField
      textRangeFromPosition:[textField positionFromPosition:textField.beginningOfDocument offset:1]
                 toPosition:[textField positionFromPosition:textField.beginningOfDocument
                                                     offset:1]];
  [container addSubview:textField];
  [lynxView addSubview:container];

  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:(LynxView *)lynxView];

  [platform insertText:@"b"];

  XCTAssertEqualObjects(textField.text, @"abc");
}

- (void)testInputEventTargetSupportsSingleTouchPointer {
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];

  std::shared_ptr<lynx::input::InputEventTarget> target =
      [platform getNativePtr]->GetInputEventTarget();

  XCTAssertNotEqual(target, nullptr);
  lynx::input::PointerCapabilities capabilities = target->GetPointerCapabilities();
  XCTAssertEqual(capabilities.default_source_type, lynx::input::PointerSourceType::kTouch);
  XCTAssertTrue(capabilities.supports_touch);
  XCTAssertFalse(capabilities.supports_mouse);
}

- (void)testInputEventTargetRejectsUnsupportedPointerEvents {
  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:nil];
  std::shared_ptr<lynx::input::InputEventTarget> target =
      [platform getNativePtr]->GetInputEventTarget();
  lynx::input::PointerEvent event;
  event.source_type = lynx::input::PointerSourceType::kMouse;
  event.type = lynx::input::PointerEventType::kDown;
  event.changed_pointer_id = 1;
  lynx::input::Pointer firstPointer;
  firstPointer.id = 1;
  firstPointer.x = 10.f;
  firstPointer.y = 20.f;
  event.pointers.push_back(firstPointer);

  XCTAssertFalse(target->InjectPointerEvent(event));

  event.source_type = lynx::input::PointerSourceType::kTouch;
  lynx::input::Pointer secondPointer;
  secondPointer.id = 2;
  secondPointer.x = 30.f;
  secondPointer.y = 40.f;
  event.pointers.push_back(secondPointer);
  XCTAssertFalse(target->InjectPointerEvent(event));
}

- (void)testStartCastingPublishesEffectiveLynxViewMode {
  id lynxView = OCMClassMock([LynxView class]);
  id templateRender = OCMClassMock([LynxTemplateRender class]);
  id renderer = OCMProtocolMock(@protocol(LynxUIRendererProtocol));
  OCMStub([lynxView templateRender]).andReturn(templateRender);
  OCMStub([templateRender lynxUIRenderer]).andReturn(renderer);
  OCMStub([renderer isFullScreenShotSupported]).andReturn(NO);

  DevToolPlatformDarwinDelegate *platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:lynxView];
  id castHelper = OCMClassMock([LynxScreenCastHelper class]);
  [platform setValue:castHelper forKey:@"castHelper"];
  NSString *format = @"jpeg";
  OCMExpect([castHelper startCasting:80
                               width:720
                              height:1280
                                mode:ScreenshotModeLynxView
                              format:format]);

  lynx::devtool::DevToolStatus::GetInstance().SetStatus(
      lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
  [platform startCasting:80 width:720 height:1280 mode:ScreenshotModeFullScreen format:format];

  XCTAssertEqualObjects([platform effectiveScreenshotMode], ScreenshotModeLynxView);
  std::string effectiveMode = lynx::devtool::DevToolStatus::GetInstance().GetStatus(
      lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode);
  XCTAssertEqualObjects([NSString stringWithUTF8String:effectiveMode.c_str()],
                        ScreenshotModeLynxView);
  OCMVerifyAll(castHelper);

  lynx::devtool::DevToolStatus::GetInstance().SetStatus(
      lynx::devtool::DevToolStatus::kDevToolStatusKeyScreenShotMode,
      lynx::devtool::DevToolStatus::SCREENSHOT_MODE_FULLSCREEN);
}

@end
