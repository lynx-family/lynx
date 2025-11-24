// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/jsi/js_task_adapter.h"

#include <memory>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/jsi/jsi_unittest.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace piper {
namespace test {

using std::chrono_literals::operator""ms;  // for ms literal

class JsTaskTest : public JSITestBase {
 protected:
  void SetUp() override {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    base::UIThread::Init();

    // Lazily create adapter to ensure fml is initialized
    adapter =
        std::make_shared<JsTaskAdapter>(runtime, "-1", tasm::PageOptions());
  }
  std::shared_ptr<JsTaskAdapter> adapter;
};

TEST_P(JsTaskTest, SetTimeoutTaskTest) {
  auto func = function("function () { globalThis.foo = 'bar' }");
  std::variant<std::unique_ptr<piper::Function>, double> result =
      std::make_unique<piper::Function>(std::move(func));
  adapter->SetTimeout(std::move(result), 100, 0);
  EXPECT_CALL(*exception_handler_, onJSIException).Times(0);

  EXPECT_TRUE(eval("globalThis.foo")->isUndefined());

  std::this_thread::sleep_for(200ms);

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(eval("globalThis.foo")->getString(rt).utf8(rt), "bar");
}

TEST_P(JsTaskTest, SetIntervalTaskTest) {
  auto func =
      function("function () { globalThis.foo = (globalThis.foo || 0) + 1 }");
  std::variant<std::unique_ptr<piper::Function>, double> result =
      std::make_unique<piper::Function>(std::move(func));
  adapter->SetInterval(std::move(result), 100, 0);
  EXPECT_CALL(*exception_handler_, onJSIException).Times(0);

  EXPECT_TRUE(eval("globalThis.foo")->isUndefined());

  std::this_thread::sleep_for(100ms);
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  EXPECT_EQ(eval("globalThis.foo")->getNumber(), 1);

  std::this_thread::sleep_for(100ms);
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  EXPECT_EQ(eval("globalThis.foo")->getNumber(), 2);
}

TEST_P(JsTaskTest, ThrownErrorTaskTest) {
  auto thrown_func = function("function() { throw new Error('foo') }");

  std::variant<std::unique_ptr<piper::Function>, double> result =
      std::make_unique<piper::Function>(std::move(thrown_func));
  adapter->SetTimeout(std::move(result), 100, 0);

  std::this_thread::sleep_for(200ms);
  // Should throw error with message 'foo'
  EXPECT_CALL(*exception_handler_, onJSIException(HasMessage("foo"))).Times(1);

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
}

TEST_P(JsTaskTest, QueueMicrotaskTaskTest) {
  auto func = function("function () { globalThis.microtask = 'microtask' }");
  std::variant<std::unique_ptr<piper::Function>, double> result =
      std::make_unique<piper::Function>(std::move(func));
  adapter->QueueMicrotask(std::move(result), 0);
  EXPECT_CALL(*exception_handler_, onJSIException).Times(0);

  EXPECT_TRUE(eval("globalThis.microtask")->isUndefined());

  std::this_thread::sleep_for(200ms);

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(eval("globalThis.microtask")->getString(rt).utf8(rt), "microtask");
}

TEST_P(JsTaskTest, MicrotaskTimeoutTaskOrderTest) {
  auto timeout_func =
      function("function () { globalThis.lastTask = 'timeout'; }");
  auto microtask_func =
      function("function () { globalThis.lastTask = 'microtask'; }");
  std::variant<std::unique_ptr<piper::Function>, double> microtask_result =
      std::make_unique<piper::Function>(std::move(microtask_func));
  adapter->QueueMicrotask(std::move(microtask_result), 0);
  std::variant<std::unique_ptr<piper::Function>, double> timeout_result =
      std::make_unique<piper::Function>(std::move(timeout_func));
  adapter->SetTimeout(std::move(timeout_result), 0, 0);
  EXPECT_CALL(*exception_handler_, onJSIException).Times(0);

  EXPECT_TRUE(eval("globalThis.lastTask")->isUndefined());

  std::this_thread::sleep_for(200ms);

  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(eval("globalThis.lastTask")->getString(rt).utf8(rt), "timeout");
}

INSTANTIATE_TEST_SUITE_P(
    Runtimes, JsTaskTest, ::testing::ValuesIn(runtimeGenerators()),
    [](const ::testing::TestParamInfo<JsTaskTest::ParamType>& info) {
      auto rt = info.param(nullptr);
      switch (rt->type()) {
        case JSRuntimeType::v8:
          return "v8";
        case JSRuntimeType::jsc:
          return "jsc";
        case JSRuntimeType::quickjs:
          return "quickjs";
        case JSRuntimeType::jsvm:
          return "jsvm";
      }
    });
}  // namespace test
}  // namespace piper
}  // namespace lynx
