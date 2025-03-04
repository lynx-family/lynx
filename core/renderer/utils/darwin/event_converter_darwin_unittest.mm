// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>

#include "core/renderer/utils/darwin/event_converter_darwin.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"
#include "core/runtime/vm/lepus/lepus_value.h"

using namespace lynx;

@interface EventConverterDarwinUnitTest : XCTestCase {
}

@end

@implementation EventConverterDarwinUnitTest

- (void)setUp {
}

- (void)tearDown {
}

- (void)testConvertFunctions {
  runtime::MessageEvent event("xxx", 1, runtime::ContextProxy::Type::kJSContext,
                              runtime::ContextProxy::Type::kDevTool, lepus::Value("zzz"));

  auto dict = tasm::darwin::EventConverterDarwin::ConverMessageEventToNSDictionary(event);
  auto result = tasm::darwin::EventConverterDarwin::ConvertNSDictionaryToMessageEvent(dict);

  XCTAssertEqual(result.type(), std::string("xxx"));
  XCTAssertEqual(event.type(), result.type());
  XCTAssertEqual(result.time_stamp(), 1);
  XCTAssertEqual(event.time_stamp(), result.time_stamp());
  XCTAssertEqual(result.GetOriginType(), runtime::ContextProxy::Type::kJSContext);
  XCTAssertEqual(event.GetOriginType(), result.GetOriginType());
  XCTAssertEqual(result.GetOriginString(), std::string(runtime::kJSContext));
  XCTAssertEqual(event.GetOriginString(), result.GetOriginString());
  XCTAssertEqual(result.GetTargetType(), runtime::ContextProxy::Type::kDevTool);
  XCTAssertEqual(event.GetTargetType(), result.GetTargetType());
  XCTAssertEqual(result.GetTargetString(), std::string(runtime::kDevTool));
  XCTAssertEqual(event.GetTargetString(), result.GetTargetString());
  XCTAssertEqual(result.message(), lepus::Value("zzz"));
  XCTAssertEqual(event.message(), result.message());
}

- (void)testConvertFunctions0 {
  runtime::MessageEvent event("", 1, runtime::ContextProxy::Type::kJSContext,
                              runtime::ContextProxy::Type::kDevTool, lepus::Value());

  auto dict = tasm::darwin::EventConverterDarwin::ConverMessageEventToNSDictionary(event);
  auto result = tasm::darwin::EventConverterDarwin::ConvertNSDictionaryToMessageEvent(dict);

  XCTAssertEqual(result.type(), std::string(""));
  XCTAssertEqual(event.type(), result.type());
  XCTAssertEqual(result.time_stamp(), 1);
  XCTAssertEqual(event.time_stamp(), result.time_stamp());
  XCTAssertEqual(result.GetOriginType(), runtime::ContextProxy::Type::kJSContext);
  XCTAssertEqual(event.GetOriginType(), result.GetOriginType());
  XCTAssertEqual(result.GetOriginString(), std::string(runtime::kJSContext));
  XCTAssertEqual(event.GetOriginString(), result.GetOriginString());
  XCTAssertEqual(result.GetTargetType(), runtime::ContextProxy::Type::kDevTool);
  XCTAssertEqual(event.GetTargetType(), result.GetTargetType());
  XCTAssertEqual(result.GetTargetString(), std::string(runtime::kDevTool));
  XCTAssertEqual(event.GetTargetString(), result.GetTargetString());
  auto empty = lepus::Value();
  XCTAssertEqual(result.message(), empty);
  XCTAssertEqual(event.message(), result.message());
}

@end
