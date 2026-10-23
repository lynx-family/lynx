// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/common/bindings/event/message_event_test.h"

#include <memory>

#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "core/runtime/common/bindings/event/context_proxy.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_wrapper_utils.h"

namespace lynx {
namespace runtime {
namespace test {

TEST_F(MessageEventTest, TestMessageEventTest0) {
  auto event = std::make_unique<MessageEvent>(
      ContextProxy::Type::kCoreContext, ContextProxy::Type::kJSContext,
      std::make_unique<pub::ValueImplLepus>(lepus::Value()));

  EXPECT_EQ(event->GetTargetType(), ContextProxy::Type::kJSContext);
  EXPECT_EQ(event->GetOriginType(), ContextProxy::Type::kCoreContext);

  auto new_event = MessageEvent::ShallowCopy(*event);
  EXPECT_EQ(new_event->time_stamp(), event->time_stamp());
  EXPECT_EQ(new_event->GetTargetType(), event->GetTargetType());
  EXPECT_EQ(new_event->GetOriginType(), event->GetOriginType());

  MessageEvent& left_ref_event = *event;
  new_event = MessageEvent::ShallowCopy(left_ref_event);
  EXPECT_EQ(new_event->time_stamp(), left_ref_event.time_stamp());
  EXPECT_EQ(new_event->GetTargetType(), left_ref_event.GetTargetType());
  EXPECT_EQ(new_event->GetOriginType(), left_ref_event.GetOriginType());
}

TEST_F(MessageEventTest, DispatchIdSurvivesCopiesWithoutChangingPayload) {
  const auto original_level =
      base::logging::ParseLogLevel(base::logging::GetMinLogLevelName());
  base::logging::SetMinLogLevel(base::logging::detail::LOG_OBSERVE);
  auto args = lepus::CArray::Create();
  args->emplace_back("component");
  args->emplace_back("handler");
  args->emplace_back(lepus::Value());
  auto event = fml::MakeRefCounted<MessageEvent>(
      ContextProxy::Type::kCoreContext, ContextProxy::Type::kJSContext,
      std::make_unique<pub::ValueImplLepus>(lepus::Value(std::move(args))));
  EXPECT_EQ(event->DispatchId(), 0u);
  event->SetTraceFlowId(0);
  event->EnableDispatchDiagnostics();
  ASSERT_NE(event->DispatchId(), 0u);

  auto mutable_copy = MessageEvent::ShallowCopy(*event);
  auto const_copy = MessageEvent::ShallowCopy(
      static_cast<const MessageEvent&>(*mutable_copy));
  EXPECT_EQ(mutable_copy->DispatchId(), event->DispatchId());
  EXPECT_EQ(const_copy->DispatchId(), event->DispatchId());
  EXPECT_EQ(const_copy->TraceFlowId(), 0u);
  auto copied_args =
      pub::ValueUtils::ConvertValueToLepusValue(*const_copy->message());
  ASSERT_TRUE(copied_args.IsArray());
  ASSERT_EQ(copied_args.Array()->size(), 3u);
  EXPECT_EQ(copied_args.Array()->get(0).StdString(), "component");
  EXPECT_EQ(copied_args.Array()->get(1).StdString(), "handler");

  auto independent = fml::MakeRefCounted<MessageEvent>(
      ContextProxy::Type::kCoreContext, ContextProxy::Type::kJSContext,
      std::make_unique<pub::ValueImplLepus>(lepus::Value()));
  independent->EnableDispatchDiagnostics();
  EXPECT_NE(independent->DispatchId(), event->DispatchId());
  base::logging::SetMinLogLevel(base::logging::LOG_INFO);
  auto disabled_copy = MessageEvent::ShallowCopy(*event);
  auto disabled_const_copy =
      MessageEvent::ShallowCopy(static_cast<const MessageEvent&>(*event));
  base::logging::SetMinLogLevel(original_level);
  EXPECT_EQ(disabled_copy->DispatchId(), 0u);
  EXPECT_EQ(disabled_const_copy->DispatchId(), 0u);
}

}  // namespace test
}  // namespace runtime
}  // namespace lynx
