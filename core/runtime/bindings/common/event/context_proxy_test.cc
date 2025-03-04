// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/common/event/context_proxy_test.h"

#include <memory>
#include <utility>

#include "core/event/event_listener_test.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/common/event/message_event.h"

namespace lynx {
namespace runtime {
namespace test {

MockContextProxyDelegate::MockContextProxyDelegate() {
  proxy_map_in_js_[ContextProxy::Type::kCoreContext] =
      std::make_unique<ContextProxy>(*this, ContextProxy::Type::kJSContext,
                                     ContextProxy::Type::kCoreContext);
  proxy_map_in_js_[ContextProxy::Type::kJSContext] =
      std::make_unique<ContextProxy>(*this, ContextProxy::Type::kJSContext,
                                     ContextProxy::Type::kJSContext);

  proxy_map_in_lepus_[ContextProxy::Type::kCoreContext] =
      std::make_unique<ContextProxy>(*this, ContextProxy::Type::kCoreContext,
                                     ContextProxy::Type::kCoreContext);
  proxy_map_in_lepus_[ContextProxy::Type::kJSContext] =
      std::make_unique<ContextProxy>(*this, ContextProxy::Type::kCoreContext,
                                     ContextProxy::Type::kJSContext);
}

event::DispatchEventResult MockContextProxyDelegate::DispatchMessageEvent(
    MessageEvent event) {
  event_vec_.emplace_back(MessageEvent::ShallowCopy(event));
  if (event.GetTargetType() == ContextProxy::Type::kJSContext) {
    auto target = proxy_map_in_js_[event.GetOriginType()].get();
    target->DispatchEvent(event);
  } else if (event.GetTargetType() == ContextProxy::Type::kCoreContext) {
    auto target = proxy_map_in_lepus_[event.GetOriginType()].get();
    target->DispatchEvent(event);
  } else {
    return event::DispatchEventResult::kCanceledBeforeDispatch;
  }
  return event::DispatchEventResult::kNotCanceled;
}

TEST_F(ContextProxyTest, TestContextProxyTest0) {
  auto context_proxy = std::make_unique<ContextProxy>(
      delegate_, ContextProxy::Type::kCoreContext,
      ContextProxy::Type::kJSContext);

  EXPECT_TRUE(delegate_.event_vec_.empty());
  EXPECT_EQ(context_proxy->GetOriginType(), ContextProxy::Type::kCoreContext);
  EXPECT_EQ(context_proxy->GetTargetType(), ContextProxy::Type::kJSContext);

  context_proxy->SetListenerBeforePublishEvent(
      std::make_unique<event::test::MockEventListener>(
          event::EventListener::Type::kJSClosureEventListener, "all"));
  context_proxy->AddEventListener(
      "1", std::make_unique<event::test::MockEventListener>(
               event::EventListener::Type::kJSClosureEventListener, "1"));
  context_proxy->AddEventListener(
      "message", std::make_unique<event::test::MockEventListener>(
                     event::EventListener::Type::kJSClosureEventListener, "1"));

  context_proxy->PostMessage(lepus::Value());
  EXPECT_EQ(delegate_.event_vec_.size(), 1);
  EXPECT_TRUE(delegate_.event_vec_[0].message().IsEmpty());
}

TEST_F(ContextProxyTest, TestContextProxyTest1) {
  auto js_js = delegate_.proxy_map_in_js_[ContextProxy::Type::kJSContext].get();
  auto js_js_all = std::make_unique<event::test::MockEventListener>(
      event::EventListener::Type::kJSClosureEventListener, "all");
  auto js_js_all_ptr = js_js_all.get();
  js_js->SetListenerBeforePublishEvent(std::move(js_js_all));

  auto js_lepus =
      delegate_.proxy_map_in_js_[ContextProxy::Type::kCoreContext].get();
  auto js_lepus_all = std::make_unique<event::test::MockEventListener>(
      event::EventListener::Type::kJSClosureEventListener, "all");
  auto js_lepus_all_ptr = js_lepus_all.get();
  js_lepus->SetListenerBeforePublishEvent(std::move(js_lepus_all));

  auto lepus_js =
      delegate_.proxy_map_in_lepus_[ContextProxy::Type::kJSContext].get();
  auto lepus_js_all = std::make_unique<event::test::MockEventListener>(
      event::EventListener::Type::kJSClosureEventListener, "all");
  auto lepus_js_all_ptr = lepus_js_all.get();
  lepus_js->SetListenerBeforePublishEvent(std::move(lepus_js_all));

  auto lepus_lepus =
      delegate_.proxy_map_in_lepus_[ContextProxy::Type::kCoreContext].get();
  auto lepus_lepus_all = std::make_unique<event::test::MockEventListener>(
      event::EventListener::Type::kJSClosureEventListener, "all");
  auto lepus_lepus_all_ptr = lepus_lepus_all.get();
  lepus_lepus->SetListenerBeforePublishEvent(std::move(lepus_lepus_all));

  // js post to js
  EXPECT_EQ(js_js_all_ptr->GetCount(), 0);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 0);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 0);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);
  js_js->PostMessage(lepus::Value());
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 0);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 0);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);

  // js post to lepus
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 0);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 0);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);
  js_lepus->PostMessage(lepus::Value());
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 0);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);

  // lepus post to lepus
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 0);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);
  lepus_lepus->PostMessage(lepus::Value());
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 1);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);

  // lepus post to js
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 1);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 0);
  lepus_js->PostMessage(lepus::Value());
  EXPECT_EQ(js_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_js_all_ptr->GetCount(), 1);
  EXPECT_EQ(lepus_lepus_all_ptr->GetCount(), 1);
  EXPECT_EQ(js_lepus_all_ptr->GetCount(), 1);
}

TEST_F(ContextProxyTest, TestContextProxyStaticMethod) {
  EXPECT_EQ(
      ContextProxy::ConvertContextTypeToString(ContextProxy::Type::kJSContext),
      "JSContext");
  EXPECT_EQ(
      ContextProxy::ConvertContextTypeToString(ContextProxy::Type::kUIContext),
      "UIContext");
  EXPECT_EQ(ContextProxy::ConvertContextTypeToString(
                ContextProxy::Type::kCoreContext),
            "CoreContext");
  EXPECT_EQ(
      ContextProxy::ConvertContextTypeToString(ContextProxy::Type::kDevTool),
      "Devtool");
  EXPECT_EQ(
      ContextProxy::ConvertContextTypeToString(ContextProxy::Type::kUnknown),
      "Unknown");

  EXPECT_EQ(ContextProxy::ConvertStringToContextType("JSContext"),
            ContextProxy::Type::kJSContext);
  EXPECT_EQ(ContextProxy::ConvertStringToContextType("UIContext"),
            ContextProxy::Type::kUIContext);
  EXPECT_EQ(ContextProxy::ConvertStringToContextType("CoreContext"),
            ContextProxy::Type::kCoreContext);
  EXPECT_EQ(ContextProxy::ConvertStringToContextType("Devtool"),
            ContextProxy::Type::kDevTool);
  EXPECT_EQ(ContextProxy::ConvertStringToContextType("xxx"),
            ContextProxy::Type::kUnknown);
}

}  // namespace test
}  // namespace runtime
}  // namespace lynx
