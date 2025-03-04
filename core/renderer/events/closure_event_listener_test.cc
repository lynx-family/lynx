// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/events/closure_event_listener.h"

#include <memory>

#include "base/include/value/base_string.h"
#include "core/event/event_listener.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace event {
namespace test {

TEST(ClosureEventListenerTest, Invoke) {
  bool called = false;
  ClosureEventListener listener([&called](lepus::Value args) {
    EXPECT_TRUE(args.IsString());
    EXPECT_TRUE(args.String().IsEqual("foo"));
    called = true;
  });
  runtime::MessageEvent event(runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value("foo"));
  listener.Invoke(&event);
  EXPECT_TRUE(called);
}

TEST(ClosureEventListenerTest, Matches) {
  ClosureEventListener listener([](lepus::Value args) {});
  runtime::MessageEvent event(runtime::ContextProxy::Type::kCoreContext,
                              runtime::ContextProxy::Type::kJSContext,
                              lepus::Value());
  EXPECT_TRUE(listener.Matches(&listener));
  ClosureEventListener listener2([](lepus::Value args) {});
  EXPECT_FALSE(listener.Matches(&listener2));
}

}  // namespace test
}  // namespace event
}  // namespace lynx
