// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/common/event/message_event_test.h"

#include <memory>

#include "core/runtime/bindings/common/event/context_proxy.h"

namespace lynx {
namespace runtime {
namespace test {

TEST_F(MessageEventTest, TestMessageEventTest0) {
  auto event = std::make_unique<MessageEvent>(ContextProxy::Type::kCoreContext,
                                              ContextProxy::Type::kJSContext,
                                              lepus::Value());

  EXPECT_EQ(event->GetTargetType(), ContextProxy::Type::kJSContext);
  EXPECT_EQ(event->GetOriginType(), ContextProxy::Type::kCoreContext);

  auto new_event = MessageEvent::ShallowCopy(*event);
  EXPECT_EQ(new_event.time_stamp(), event->time_stamp());
  EXPECT_EQ(new_event.GetTargetType(), event->GetTargetType());
  EXPECT_EQ(new_event.GetOriginType(), event->GetOriginType());

  MessageEvent& left_ref_event = *event;
  new_event = MessageEvent::ShallowCopy(left_ref_event);
  EXPECT_EQ(new_event.time_stamp(), left_ref_event.time_stamp());
  EXPECT_EQ(new_event.GetTargetType(), left_ref_event.GetTargetType());
  EXPECT_EQ(new_event.GetOriginType(), left_ref_event.GetOriginType());
}

}  // namespace test
}  // namespace runtime
}  // namespace lynx
