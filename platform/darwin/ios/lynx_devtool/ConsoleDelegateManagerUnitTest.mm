// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#import <LynxDevtool/ConsoleDelegateManager.h>
#import <LynxDevtool/DevToolPlatformDarwinDelegate.h>
#import <LynxDevtool/LynxInspectorConsoleDelegate.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#include "devtool/lynx_devtool/js_debug/js/inspector_java_script_debugger_impl.h"

@interface LynxInspectorConsoleDelegateMock : NSObject <LynxInspectorConsoleDelegate>

@property(nonatomic, readwrite) NSString *message;

- (void)onConsoleMessage:(NSString *)msg;

@end

@implementation LynxInspectorConsoleDelegateMock

- (void)onConsoleMessage:(NSString *)msg {
  _message = msg;
}

@end

@interface ConsoleDelegateManagerUnitTest : XCTestCase

@end

@implementation ConsoleDelegateManagerUnitTest {
  LynxInspectorConsoleDelegateMock *_consoleDelegate;
  DevToolPlatformDarwinDelegate *_platform;
  std::shared_ptr<lynx::devtool::InspectorJavaScriptDebuggerImpl> debugger_;
  NSString *_objectDetail;
  NSString *_objectStringify;
}

- (void)setUp {
  _consoleDelegate = [[LynxInspectorConsoleDelegateMock alloc] init];

  _platform =
      [[DevToolPlatformDarwinDelegate alloc] initWithLynxView:OCMClassMock([LynxView class])
                                                  withUIOwner:OCMClassMock([LynxUIOwner class])];
  [_platform setLynxInspectorConsoleDelegate:_consoleDelegate];

  debugger_ = std::make_shared<lynx::devtool::InspectorJavaScriptDebuggerImpl>(nullptr);
  [_platform getNativePtr]->js_debugger_wp_ = debugger_;
}

- (void)testOnConsoleMessage {
  std::string message = "{\"type\":\"log\",\"args\":[{\"value\":\"test object: "
                        "\",\"type\":\"string\"},{\"type\":\"object\",\"objectId\":"
                        "\"530412529136\",\"className\":\"Object\",\"description\":\"Object\"}]}";
  [_platform onConsoleMessage:message];
  NSString *result = [_consoleDelegate message];
  XCTAssertEqual(message, [result UTF8String]);
}

- (void)testOnConsoleObject {
  NSString *objectId = @"530412529136";
  [_platform getConsoleObject:objectId
                needStringify:NO
                resultHandler:^(NSString *detail) {
                  self->_objectDetail = detail;
                }];
  [_platform getConsoleObject:objectId
                needStringify:YES
                resultHandler:^(NSString *detail) {
                  self->_objectStringify = detail;
                }];

  std::string detail =
      "[{\"name\":\"a\",\"value\":{\"description\":\"1\",\"value\":1,\"type\":\"number\"}},{"
      "\"name\":\"b\",\"value\":{\"value\":\"test\",\"type\":\"string\"}},{\"name\":\"c\","
      "\"value\":{\"value\":true,\"type\":\"boolean\"}},{\"name\":\"__proto__\",\"value\":{"
      "\"type\":\"object\",\"objectId\":\"530412818672\",\"className\":\"Object\",\"description\":"
      "\"Object\"}}]";
  std::string stringify = "{\n\t\"a\": 1,\n\t\"b\": \"test\",\n\t\"c\": true\n}";
  [_platform onConsoleObject:detail callbackId:-1];
  XCTAssertEqual(detail, [_objectDetail UTF8String]);
  [_platform onConsoleObject:stringify callbackId:-2];
  XCTAssertEqual(stringify, [_objectStringify UTF8String]);
}

@end
