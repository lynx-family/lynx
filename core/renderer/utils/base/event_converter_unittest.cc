// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/base/event_converter.h"

#include <string>

#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

TEST(EventConverterTest, PreservesMetadataAndOpaqueStrings) {
  const std::string payloads[] = {"", "{\"type\":\"refresh\"}",
                                  "\"quoted\"\nemoji: \xf0\x9f\x98\x80",
                                  std::string("nul\0tail", 8)};
  for (const auto& payload : payloads) {
    auto factory = pub::PubValueFactoryDefault();
    runtime::MessageEvent event(
        "ProtocolTest", 1792562665000, runtime::ContextProxy::Type::kJSContext,
        runtime::ContextProxy::Type::kDevTool, factory.CreateString(payload));
    auto bridge_event = EventConverter::ConvertMessageEventToVMEvent(event);
    EXPECT_EQ(bridge_event.type, "ProtocolTest");
    EXPECT_EQ(bridge_event.timestamp, 1792562665000);
    EXPECT_EQ(bridge_event.origin, "JSContext");
    EXPECT_EQ(bridge_event.target, "Devtool");
    EXPECT_EQ(bridge_event.data, payload);
    auto copy = EventConverter::ConvertVMEventToMessageEvent(bridge_event);
    EXPECT_TRUE(copy->IsSendingToDevTool());
    EXPECT_EQ(copy->message()->str(), payload);
  }
}

TEST(EventConverterTest, RoutesDevtoolMessagesToEachVM) {
  for (const auto* target : {"JSContext", "CoreContext", "UIContext"}) {
    devtool::VMEvent input{"ProtocolTest", "Devtool", target,
                           "{\"type\":\"refresh\"}", 1};
    auto event = EventConverter::ConvertVMEventToMessageEvent(input);
    EXPECT_GT(event->time_stamp(), input.timestamp);
    EXPECT_EQ(event->type(), "ProtocolTest");
    EXPECT_EQ(event->GetOriginType(), runtime::ContextProxy::Type::kDevTool);
    EXPECT_EQ(event->GetTargetString(), target);
    EXPECT_EQ(event->IsSendingToJSThread(), std::string(target) == "JSContext");
    EXPECT_EQ(event->IsSendingToCoreThread(),
              std::string(target) == "CoreContext");
    EXPECT_EQ(event->IsSendingToUIThread(), std::string(target) == "UIContext");
    EXPECT_EQ(event->message()->str(), input.data);
  }
}

TEST(EventConverterTest, EmptyPayloadAndUnknownContexts) {
  devtool::VMEvent input{"ProtocolTest", "Devtool", "unknown"};
  auto event = EventConverter::ConvertVMEventToMessageEvent(input);
  EXPECT_FALSE(event->IsSendingToJSThread());
  EXPECT_FALSE(event->IsSendingToCoreThread());
  EXPECT_FALSE(event->IsSendingToUIThread());
  EXPECT_EQ(event->message()->str(), "");
  auto empty = EventConverter::ConvertVMEventToMessageEvent({});
  EXPECT_EQ(empty->type(), "");
  EXPECT_EQ(empty->GetOriginType(), runtime::ContextProxy::Type::kUnknown);
  EXPECT_EQ(empty->GetTargetString(), "Unknown");
  EXPECT_EQ(empty->message()->str(), "");

  runtime::MessageEvent empty_event(
      "ProtocolTest", runtime::ContextProxy::Type::kJSContext,
      runtime::ContextProxy::Type::kDevTool, nullptr);
  EXPECT_EQ(EventConverter::ConvertMessageEventToVMEvent(empty_event).data, "");
}

}  // namespace tasm
}  // namespace lynx
