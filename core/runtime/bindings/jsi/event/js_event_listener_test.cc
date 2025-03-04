// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/runtime/bindings/jsi/event/js_event_listener_test.h"

#include <string>
#include <utility>

#include "base/include/fml/message_loop.h"
#include "core/event/event_listener_test.h"
#include "core/runtime/bindings/common/event/message_event.h"
#include "core/runtime/bindings/jsi/event/js_event_listener.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

void JSClosureEventListenerTest::SetUp() {
  base::UIThread::Init();
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  auto nativeModule =
      eval("(function() { return {}; })()")->asObject(rt).value();
  app_ = App::Create(0, runtime, &delegate_, exception_handler_,
                     std::move(nativeModule), nullptr, "-1");
}

TEST_P(JSClosureEventListenerTest, JSClosureEventListenerMatches) {
  auto js_function_1 = function(R"--(
function onEvent(e) {

  return;
}
)--");

  auto js_function_2 = function(R"--(
function onEvent(e) {

  return;
}
)--");

  auto listener_1_1 = std::make_unique<JSClosureEventListener>(
      runtime, app_, piper::Value(rt, js_function_1));
  auto listener_1_2 = std::make_unique<JSClosureEventListener>(
      runtime, app_, piper::Value(rt, js_function_1));

  EXPECT_TRUE(listener_1_1->Matches(listener_1_2.get()));
  EXPECT_TRUE(listener_1_1->Matches(listener_1_1.get()));
  EXPECT_TRUE(listener_1_2->Matches(listener_1_2.get()));

  auto listener_2_1 = std::make_unique<JSClosureEventListener>(
      nullptr, nullptr, piper::Value(rt, js_function_2));
  EXPECT_FALSE(listener_1_1->Matches(listener_2_1.get()));

  auto listener_2_2 = std::make_unique<JSClosureEventListener>(
      runtime, app_, piper::Value(rt, js_function_2));
  EXPECT_FALSE(listener_1_1->Matches(listener_2_2.get()));

  event::test::MockEventListener mock_listener(
      event::EventListener::Type::kLepusClosureEventListener, "1");
  EXPECT_FALSE(listener_1_1->Matches(&mock_listener));

  std::shared_ptr<Runtime> another_runtime(
      factory(exception_handler_).release());
  auto& another_rt = *another_runtime;
  auto another_js_function_str = std::string(R"--(
  function onEvent(e) {
    return;
  }
  )--");
  auto another_js_function =
      another_runtime->global()
          .getPropertyAsFunction(another_rt, "eval")
          ->call(another_rt, ("(" + another_js_function_str + ")").c_str())
          ->getObject(another_rt)
          .getFunction(another_rt);
  auto another_js_function_value =
      piper::Value(another_rt, another_js_function);
  auto another_js_listener = std::make_unique<JSClosureEventListener>(
      another_runtime, app_, another_js_function_value);
  EXPECT_FALSE(listener_1_1->Matches(another_js_listener.get()));

  auto copy_runtime = runtime;
  auto listener_1_1_from_copy_runtime =
      std::make_unique<JSClosureEventListener>(runtime, app_,
                                               piper::Value(rt, js_function_1));
  EXPECT_TRUE(listener_1_1->Matches(listener_1_1_from_copy_runtime.get()));
}

TEST_P(JSClosureEventListenerTest, JSClosureEventListenerInvoke) {
  auto buffer = std::make_shared<StringBuffer>(R"--(
    globalThis.count = 0;
)--");
  auto ret = runtime->evaluateJavaScript(buffer, "test");
  EXPECT_TRUE(ret.has_value());

  auto count = runtime->global().getProperty(rt, "count");
  EXPECT_TRUE(count->isNumber());
  EXPECT_EQ(count->asNumber(rt), 0);

  auto js_function = function(R"--(
function onEvent(e) {
  globalThis.count = globalThis.count + 1;
  return;
}
)--");

  auto listener = std::make_unique<JSClosureEventListener>(
      runtime, app_, piper::Value(rt, js_function));
  listener->Invoke(nullptr);
  count = runtime->global().getProperty(rt, "count");
  EXPECT_EQ(count->asNumber(rt), 1);

  listener = std::make_unique<JSClosureEventListener>(
      nullptr, nullptr, piper::Value(rt, js_function));
  listener->Invoke(nullptr);
  count = runtime->global().getProperty(rt, "count");
  EXPECT_EQ(count->asNumber(rt), 1);

  listener = std::make_unique<JSClosureEventListener>(runtime, app_, *count);
  listener->Invoke(nullptr);
  count = runtime->global().getProperty(rt, "count");
  EXPECT_EQ(count->asNumber(rt), 1);
}

TEST_P(JSClosureEventListenerTest, JSClosureEventListenerGetClosure) {
  auto js_function = function(R"--(
function onEvent(e) {

  return;
}
)--");

  auto listener_1 = std::make_unique<JSClosureEventListener>(
      runtime, app_, piper::Value(rt, js_function));
  auto listener_2 = std::make_unique<JSClosureEventListener>(
      nullptr, nullptr, piper::Value(rt, js_function));

  EXPECT_TRUE(piper::Value::strictEquals(rt, piper::Value(rt, js_function),
                                         listener_1->GetClosure()));
  EXPECT_TRUE(listener_2->GetClosure().isUndefined());
}

TEST_P(JSClosureEventListenerTest,
       JSClosureEventListenerConvertEventToPiperValue) {
  auto message_event = std::make_unique<runtime::MessageEvent>(
      runtime::ContextProxy::Type::kCoreContext,
      runtime::ContextProxy::Type::kJSContext, lepus::Value("1"));

  auto js_function = function(R"--(
function onEvent(e) {

  return;
}
)--");
  auto listener_1 = std::make_unique<JSClosureEventListener>(
      runtime, app_, piper::Value(rt, js_function));

  auto res_1 = listener_1->ConvertEventToPiperValue(message_event.get());
  EXPECT_TRUE(res_1.isObject());
  EXPECT_EQ(res_1.getObject(rt).getProperty(rt, "type")->getString(rt).utf8(rt),
            "message");
  EXPECT_EQ(res_1.getObject(rt).getProperty(rt, "data")->getString(rt).utf8(rt),
            "1");
  EXPECT_EQ(
      res_1.getObject(rt).getProperty(rt, "origin")->getString(rt).utf8(rt),
      "CoreContext");

  auto listener_2 = std::make_unique<JSClosureEventListener>(
      nullptr, nullptr, piper::Value(rt, js_function));
  auto res_2 = listener_2->ConvertEventToPiperValue(message_event.get());
  EXPECT_TRUE(res_2.isUndefined());

  message_event->event_type_ = event::Event::EventType::kTouchEvent;
  auto res_3 = listener_1->ConvertEventToPiperValue(message_event.get());
  EXPECT_TRUE(res_3.isObject());
}

INSTANTIATE_TEST_SUITE_P(Runtimes, JSClosureEventListenerTest,
                         ::testing::ValuesIn(runtimeGenerators()),
                         [](const ::testing::TestParamInfo<
                             JSClosureEventListenerTest::ParamType>& info) {
                           auto rt = info.param(nullptr);
                           switch (rt->type()) {
                             case JSRuntimeType::v8:
                               return "v8";
                             case JSRuntimeType::jsc:
                               return "jsc";
                             case JSRuntimeType::quickjs:
                               return "quickjs";
                           }
                         });

}  // namespace test
}  // namespace piper
}  // namespace lynx
