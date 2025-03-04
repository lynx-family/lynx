// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public

#include "core/runtime/bindings/jsi/event/context_proxy_in_js_test.h"

#include "core/runtime/bindings/jsi/event/js_event_listener_test.h"

namespace lynx {
namespace piper {
namespace test {

event::DispatchEventResult JSRuntimeTestMockDelegate::DispatchMessageEvent(
    runtime::MessageEvent event) {
  ss_ << "target: " << event.GetTargetString()
      << ", origin: " << event.GetOriginString()
      << ", message: " << event.message_;
  return event::DispatchEventResult::kNotCanceled;
}

void JSRuntimeTestMockDelegate::ClearResult() {
  ss_.str("");
  ss_.clear();
}

std::string JSRuntimeTestMockDelegate::DumpResult() { return ss_.str(); }

Value JSRuntimeTestMockJSApp::get(Runtime*, const PropNameID& name) {
  return piper::Value::undefined();
}

void JSRuntimeTestMockJSApp::set(Runtime*, const PropNameID& name,
                                 const Value& value) {
  return;
}

std::vector<PropNameID> JSRuntimeTestMockJSApp::getPropertyNames(Runtime& rt) {
  std::vector<PropNameID> vec;
  return vec;
}

void ContextProxyInJSTest::SetUp() {
  fml::MessageLoop::EnsureInitializedForCurrentThread();

  mock_js_app_ = std::make_shared<JSRuntimeTestMockJSApp>(runtime);

  auto nativeModule =
      eval("(function() { return {}; })()")->asObject(rt).value();
  app_ = App::Create(0, runtime, &delegate_, exception_handler_,
                     std::move(nativeModule), nullptr, "-1");

  app_->setJsAppObj(Object::createFromHostObject(*runtime, mock_js_app_));

  lynx_proxy_ = std::make_shared<piper::LynxProxy>(runtime, app_);
  Object lynx_obj = Object::createFromHostObject(rt, lynx_proxy_);
  function(R"--(
    function registerLynx(lynx) {
      globalThis.lynx = lynx;
    }
  )--")
      .call(rt, lynx_obj);
}

TEST_P(ContextProxyInJSTest, ContextProxyInJSPostMessageTest) {
  delegate_.ClearResult();
  function(R"--(
    function postTest0() {
      lynx.getCoreContext().postMessage('test message');
    }
  )--")
      .call(rt, piper::Value::undefined());

  EXPECT_EQ(delegate_.DumpResult(),
            "target: CoreContext, origin: JSContext, message: test message");

  delegate_.ClearResult();
  function(R"--(
    function postTest1() {
      return lynx.getCoreContext().postMessage();
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(), "");

  delegate_.ClearResult();
  function(R"--(
    function postTest1() {
      return lynx.getCoreContext().postMessage('1', '2', '3');
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(),
            "target: CoreContext, origin: JSContext, message: 1");
}

TEST_P(ContextProxyInJSTest, ContextProxyInJSDispatchEventTest) {
  delegate_.ClearResult();
  function(R"--(
    function test() {
      lynx.getCoreContext().dispatchEvent();
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(), "");

  delegate_.ClearResult();
  function(R"--(
    function test() {
      lynx.getCoreContext().dispatchEvent({});
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(), "");

  delegate_.ClearResult();
  function(R"--(
    function test() {
      lynx.getCoreContext().dispatchEvent({type: 'xxx', data: 'yyy'});
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(),
            "target: CoreContext, origin: JSContext, message: yyy");

  delegate_.ClearResult();
  function(R"--(
    function test() {
      lynx.getCoreContext().dispatchEvent({type: 'xxx'});
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(), "");

  delegate_.ClearResult();
  function(R"--(
    function test() {
      lynx.getCoreContext().dispatchEvent({data: 'xxx'});
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_EQ(delegate_.DumpResult(), "");
}

TEST_P(ContextProxyInJSTest, ContextProxyInJSAddEventListenerTest) {
  // Remove other listener before test.
  app_->context_proxy_vector_[static_cast<int32_t>(
                                  runtime::ContextProxy::Type::kCoreContext)]
      ->GetEventListenerMap()
      ->Clear();
  function(R"--(
    function test() {
      lynx.getCoreContext().addEventListener();
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_TRUE(app_
                  ->context_proxy_vector_[static_cast<int32_t>(
                      runtime::ContextProxy::Type::kCoreContext)]
                  ->GetEventListenerMap()
                  ->IsEmpty());

  function(R"--(
    function test() {
      lynx.getCoreContext().addEventListener(1, ()=>{});
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_TRUE(app_
                  ->context_proxy_vector_[static_cast<int32_t>(
                      runtime::ContextProxy::Type::kCoreContext)]
                  ->GetEventListenerMap()
                  ->IsEmpty());

  function(R"--(
    function test() {
      lynx.getCoreContext().addEventListener("1", "2");
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_TRUE(app_
                  ->context_proxy_vector_[static_cast<int32_t>(
                      runtime::ContextProxy::Type::kCoreContext)]
                  ->GetEventListenerMap()
                  ->IsEmpty());

  function(R"--(
    function test() {
      lynx.getCoreContext().addEventListener("1", ()=>{});
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_FALSE(app_
                   ->context_proxy_vector_[static_cast<int32_t>(
                       runtime::ContextProxy::Type::kCoreContext)]
                   ->GetEventListenerMap()
                   ->IsEmpty());
  EXPECT_EQ(app_
                ->context_proxy_vector_[static_cast<int32_t>(
                    runtime::ContextProxy::Type::kCoreContext)]
                ->GetEventListenerMap()
                ->Find("1")
                ->size(),
            1);
}

TEST_P(ContextProxyInJSTest, ContextProxyInJSRemoveEventListenerTest) {
  // Remove other listener before test.
  app_->context_proxy_vector_[static_cast<int32_t>(
                                  runtime::ContextProxy::Type::kCoreContext)]
      ->GetEventListenerMap()
      ->Clear();
  eval(R"--(
    globalThis.onEvent = (e)=>{};
    globalThis.onEvent_1 = (e)=>{};
    )--");

  function(R"--(
    function test() {
      lynx.getCoreContext().addEventListener("1", globalThis.onEvent);
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_FALSE(app_
                   ->context_proxy_vector_[static_cast<int32_t>(
                       runtime::ContextProxy::Type::kCoreContext)]
                   ->GetEventListenerMap()
                   ->IsEmpty());
  EXPECT_EQ(app_
                ->context_proxy_vector_[static_cast<int32_t>(
                    runtime::ContextProxy::Type::kCoreContext)]
                ->GetEventListenerMap()
                ->Find("1")
                ->size(),
            1);

  function(R"--(
    function test() {
      lynx.getCoreContext().removeEventListener(1, globalThis.onEvent);
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_FALSE(app_
                   ->context_proxy_vector_[static_cast<int32_t>(
                       runtime::ContextProxy::Type::kCoreContext)]
                   ->GetEventListenerMap()
                   ->IsEmpty());
  EXPECT_EQ(app_
                ->context_proxy_vector_[static_cast<int32_t>(
                    runtime::ContextProxy::Type::kCoreContext)]
                ->GetEventListenerMap()
                ->Find("1")
                ->size(),
            1);

  function(R"--(
    function test() {
      lynx.getCoreContext().removeEventListener("1", globalThis.onEvent1);
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_FALSE(app_
                   ->context_proxy_vector_[static_cast<int32_t>(
                       runtime::ContextProxy::Type::kCoreContext)]
                   ->GetEventListenerMap()
                   ->IsEmpty());
  EXPECT_EQ(app_
                ->context_proxy_vector_[static_cast<int32_t>(
                    runtime::ContextProxy::Type::kCoreContext)]
                ->GetEventListenerMap()
                ->Find("1")
                ->size(),
            1);

  function(R"--(
    function test() {
      lynx.getCoreContext().removeEventListener("1", globalThis.onEvent);
    }
  )--")
      .call(rt, piper::Value::undefined());
  EXPECT_TRUE(app_
                  ->context_proxy_vector_[static_cast<int32_t>(
                      runtime::ContextProxy::Type::kCoreContext)]
                  ->GetEventListenerMap()
                  ->IsEmpty());
  EXPECT_EQ(app_
                ->context_proxy_vector_[static_cast<int32_t>(
                    runtime::ContextProxy::Type::kCoreContext)]
                ->GetEventListenerMap()
                ->Find("1")
                ->size(),
            0);
}

TEST_P(ContextProxyInJSTest, ContextProxyInJSOnTriggerEventTest) {
  eval(R"--(
    globalThis.onEvent = (e)=>{};
    globalThis.onEvent_1 = (e)=>{};
    )--");

  function(R"--(
    function test() {
      lynx.getCoreContext().onTriggerEvent = globalThis.onEvent;
    }
  )--")
      .call(rt, piper::Value::undefined());

  auto compare_event_listener = std::make_unique<JSClosureEventListener>(
      runtime, app_, *(runtime->global().getProperty(rt, "onEvent")));
  auto compare_event_listener_1 = std::make_unique<JSClosureEventListener>(
      runtime, app_, *(runtime->global().getProperty(rt, "onEvent_1")));

  EXPECT_TRUE(app_
                  ->context_proxy_vector_[static_cast<int32_t>(
                      runtime::ContextProxy::Type::kCoreContext)]
                  ->GetListenerBeforePublishEvent()
                  ->Matches(compare_event_listener.get()));
  EXPECT_FALSE(app_
                   ->context_proxy_vector_[static_cast<int32_t>(
                       runtime::ContextProxy::Type::kCoreContext)]
                   ->GetListenerBeforePublishEvent()
                   ->Matches(compare_event_listener_1.get()));

  eval(R"--(
    globalThis.result = onEvent === lynx.getCoreContext().onTriggerEvent;
    )--");

  EXPECT_TRUE(runtime->global().getProperty(rt, "result")->isBool());
  EXPECT_TRUE(runtime->global().getProperty(rt, "result")->getBool());
}

INSTANTIATE_TEST_SUITE_P(
    Runtimes, ContextProxyInJSTest, ::testing::ValuesIn(runtimeGenerators()),
    [](const ::testing::TestParamInfo<ContextProxyInJSTest::ParamType>& info) {
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
