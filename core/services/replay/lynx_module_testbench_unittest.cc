// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#include "core/services/replay/lynx_module_testbench.h"

#include "core/runtime/js/jsi/jsi.h"
#undef private

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <mutex>

#include "base/include/closure.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/fml/memory/task_runner_checker.h"
#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "core/runtime/js/bindings/modules/module_delegate.h"
#include "core/services/replay/lynx_module_fixture_replay.h"
#include "core/services/replay/lynx_module_manager_testbench.h"
#include "testing/utils/make_js_runtime.h"
#if defined(OS_WIN)
#include <direct.h>

#include <random>
#else
#include <unistd.h>
#endif
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {

static fml::AutoResetWaitableEvent latch;

class MockDelegate : public runtime::js::ModuleDelegate {
 public:
  virtual int64_t RegisterJSCallbackFunction(
      runtime::js::Function func) override {
    return 1;
  }
  virtual void CallJSCallback(
      const std::shared_ptr<runtime::js::ModuleCallback>& callback,
      base::MoveOnlyClosure<bool> invoke_pre_func = nullptr,
      int64_t id_to_delete =
          runtime::js::ModuleCallback::kInvalidCallbackId) override {
    latch.Signal();
  }
  virtual void OnErrorOccurred(base::LynxError error) override {}
  virtual void OnMethodInvoked(const std::string& module_name,
                               const std::string& method_name,
                               int32_t code) override {}
  virtual void FlushJSBTiming(runtime::js::NativeModuleInfo timing) override {}
  virtual void RunOnJSThread(base::closure func) override {}
  virtual void RunOnPlatformThread(base::closure func) override {}
};

class FixtureModuleTest : public ::testing::Test {
 protected:
  void SetUp() override {
#if defined(OS_WIN)
    std::random_device random;
    for (int i = 0; i < 16; ++i) {
      directory = ::testing::TempDir() + "lynx_fixture_module_" +
                  std::to_string(random());
      if (_mkdir(directory.c_str()) == 0) return;
    }
    FAIL() << "Cannot create fixture directory";
#else
    directory = ::testing::TempDir() + "lynx_fixture_module_XXXXXX";
    ASSERT_NE(mkdtemp(directory.data()), nullptr);
#endif
  }
  void TearDown() override {
    std::remove((directory + "/fixture.js").c_str());
#if defined(OS_WIN)
    _rmdir(directory.c_str());
#else
    rmdir(directory.c_str());
#endif
  }
  std::string directory;
};

TEST_F(FixtureModuleTest, InitializesFromReplayDataModuleAndReleasesAssets) {
  using namespace runtime::js;
  class DataBinding : public LynxJSIModuleBinding {
   public:
    DataBinding() : LynxJSIModuleBinding(nullptr) {}
    Value get(Runtime* rt, const PropNameID& name) override {
      if (name.utf8(*rt) != "LynxRecorderReplayDataModule")
        return Value::null();
      return std::move(*rt->global().getProperty(*rt, "replayData"));
    }
  };
  std::ofstream(directory + "/fixture.js") << R"(
ctx.register("FixtureModule", "read", function() { return 42; });
)";
  for (bool fixture : {false, true}) {
    auto runtime = testing::utils::makeJSRuntime();
    runtime->global().setProperty(
        *runtime, "fixtureDirectory",
        String::createFromUtf8(*runtime, fixture ? directory : ""));
    ASSERT_TRUE(runtime
                    ->evaluateJavaScript(std::make_shared<StringBuffer>(R"(
var reads = 0, releases = 0;
var replayData = {
  getData: function(callback) {
    ++reads;
    callback(JSON.stringify({RecordData:"{}", JsbSettings:"{}", JsbIgnoredInfo:"{}"}));
  },
  releaseFixture: function() { ++releases; }
};
if (fixtureDirectory) replayData.getFixtureDirectory = function() { return fixtureDirectory; };
)"),
                                         "replay-data.js")
                    .has_value());
    auto manager = std::make_shared<ModuleManagerTestBench>();
    manager->initBindingPtr(manager, std::make_shared<MockDelegate>(),
                            std::make_shared<DataBinding>());
    bool json_loaded = false;
    manager->initRecordModuleData(runtime.get(), [&] { json_loaded = true; });
    EXPECT_EQ(json_loaded, !fixture);
    EXPECT_EQ(runtime->global().getProperty(*runtime, "reads")->getNumber(),
              fixture ? 0 : 1);
    if (fixture) {
      auto module = manager->bindingPtr->get(
          runtime.get(), PropNameID::forAscii(*runtime, "FixtureModule"));
      auto method =
          module.getObject(*runtime).getPropertyAsFunction(*runtime, "read");
      ASSERT_TRUE(method);
      auto result = method->call(*runtime);
      ASSERT_TRUE(result);
      ASSERT_TRUE(result->isNumber());
      EXPECT_EQ(result->getNumber(), 42);
      manager->Destroy();
      result = method->call(*runtime);
      ASSERT_TRUE(result);
      EXPECT_TRUE(result->isUndefined());
    }
    manager->Destroy();
    manager->Destroy();
    EXPECT_EQ(runtime->global().getProperty(*runtime, "releases")->getNumber(),
              fixture ? 1 : 0);
    EXPECT_TRUE(manager->bindingPtr
                    ->get(runtime.get(),
                          PropNameID::forAscii(*runtime, "FixtureModule"))
                    .isNull());
  }
}

TEST_F(FixtureModuleTest, DispatchReturnCallbacksAndDestroy) {
  using namespace runtime::js;
  class Delegate : public MockDelegate {
   public:
    int64_t RegisterJSCallbackFunction(Function func) override {
      auto id = static_cast<int64_t>(holders.size());
      holders.emplace(
          id, std::make_unique<ModuleCallbackFunctionHolder>(std::move(func)));
      return id;
    }
    void CallJSCallback(const std::shared_ptr<ModuleCallback>& callback,
                        base::MoveOnlyClosure<bool> = nullptr,
                        int64_t = ModuleCallback::kInvalidCallbackId) override {
      std::lock_guard<std::mutex> lock(mutex);
      callbacks.push_back(callback);
      ready.notify_all();
    }
    std::map<int64_t, std::unique_ptr<ModuleCallbackFunctionHolder>> holders;
    std::vector<std::shared_ptr<ModuleCallback>> callbacks;
    std::mutex mutex;
    std::condition_variable ready;
  };
  std::ofstream(directory + "/fixture.js") << R"(
    export default function(ctx) {
      ctx.register("FixtureModule", "read", function(args, callbacks) {
        if (typeof args[0] !== "function" || args[1].key !== "value") throw Error("args");
        if (args[2] !== "undefined" || args[3] !== "NaN" || args[1].missing !== "undefined") throw Error("special args");
        callbacks[0]({text:"NaN", __lynx_val__:5}, 0);
        callbacks[0]({text:"NaN", __lynx_val__:5}, 5);
        return {n:++this.n, __lynx_val__:"NaN"};
      });
      ctx.register("FixtureModule", "reject", function(args, callbacks) {
        callbacks[0]("valid");
        if (args[1]) return "a\u0000b";
        callbacks[0]("a\u0000b");
      });
    })";
  auto runtime = testing::utils::makeJSRuntime();
  auto context = std::make_shared<FixtureContext>();
  ASSERT_TRUE(context->Initialize(directory));
  auto delegate = std::make_shared<Delegate>();
  ModuleFixtureReplay module("FixtureModule", delegate, context);
  int calls = 0;
  auto callback = Function::createFromHostFunction(
      *runtime, PropNameID::forAscii(*runtime, "callback"), 1,
      [&calls](Runtime& rt, const Value&, const Value* args,
               size_t count) -> base::expected<Value, JSINativeException> {
        EXPECT_EQ(count, 1u);
        auto text = args[0].getObject(rt).getProperty(rt, "text");
        EXPECT_EQ(text->getString(rt).utf8(rt), "NaN");
        auto wrapped = args[0].getObject(rt).getProperty(rt, "__lynx_val__");
        EXPECT_EQ(wrapped->getNumber(), 5);
        ++calls;
        return Value::undefined();
      });
  Object object(*runtime);
  object.setProperty(*runtime, "key",
                     String::createFromUtf8(*runtime, "value"));
  object.setProperty(*runtime, "missing", Value::undefined());
  auto method =
      module.get(runtime.get(), PropNameID::forAscii(*runtime, "read"))
          .getObject(*runtime)
          .getFunction(*runtime);
  for (int n = 1; n <= 2; ++n) {
    auto result = method.call(*runtime, callback, object, Value::undefined(),
                              Value(std::numeric_limits<double>::quiet_NaN()));
    ASSERT_TRUE(result);
    ASSERT_TRUE(result->isObject());
    auto value = result->getObject(*runtime).getProperty(*runtime, "n");
    EXPECT_EQ(value->getNumber(), n);
    auto wrapped =
        result->getObject(*runtime).getProperty(*runtime, "__lynx_val__");
    EXPECT_EQ(wrapped->getString(*runtime).utf8(*runtime), "NaN");
  }
  {
    std::unique_lock<std::mutex> lock(delegate->mutex);
    ASSERT_TRUE(delegate->ready.wait_for(lock, std::chrono::seconds(3), [&] {
      return delegate->callbacks.size() == 4;
    }));
  }
  // Delivery owns its JSON after Dispatch returns, including repeated use of
  // one function.
  for (auto& wrapper : delegate->callbacks)
    wrapper->Invoke(runtime.get(),
                    delegate->holders[wrapper->CallbackId()].get());
  EXPECT_EQ(calls, 4);
  auto reject =
      module.get(runtime.get(), PropNameID::forAscii(*runtime, "reject"))
          .getObject(*runtime)
          .getFunction(*runtime);
  for (bool invalid_return : {false, true}) {
    auto result = reject.call(*runtime, callback, Value(invalid_return));
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->isUndefined());
    // Even the preceding valid callback must not be registered for delivery.
    EXPECT_EQ(delegate->holders.size(), 4u);
  }
  module.Destroy();
  context->Destroy();
  for (auto& wrapper : delegate->callbacks)
    wrapper->Invoke(runtime.get(),
                    delegate->holders[wrapper->CallbackId()].get());
  EXPECT_EQ(calls, 4);
  auto result = method.call(*runtime, callback, object, Value::undefined(),
                            Value(std::numeric_limits<double>::quiet_NaN()));
  ASSERT_TRUE(result);
  EXPECT_TRUE(result->isUndefined());
}

TEST(LynxModuleTestBench, StrictMode) {
  runtime::js::ModuleTestBench module("replayModule", nullptr);

  rapidjson::Document value;
  value.Parse("{}");
  module.jsb_settings_ = &value;
  ASSERT_TRUE(module.IsStrictMode());

  rapidjson::Document value2;
  value2.Parse("{\"strict\":true}");
  module.jsb_settings_ = &value2;
  ASSERT_TRUE(module.IsStrictMode());

  rapidjson::Document value3;
  value3.Parse("{\"strict\":false}");
  module.jsb_settings_ = &value3;
  ASSERT_FALSE(module.IsStrictMode());

  module.jsb_settings_ = nullptr;
  ASSERT_TRUE(module.IsStrictMode());
}

TEST(LynxModuleTestBench, InvokeJsbCallback) {
  std::shared_ptr<MockDelegate> mock_delegate(new MockDelegate);
  runtime::js::ModuleTestBench module("replayModule", mock_delegate);
  runtime::js::Function function(nullptr);
  module.InvokeJsbCallback(std::move(function),
                           rapidjson::Value(rapidjson::kNullType));
  ASSERT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(3)));
  module.InvokeJsbCallback(std::move(function),
                           rapidjson::Value(rapidjson::kNullType), 1 * 1000);
  ASSERT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(3)));
}

TEST(LynxModuleTestBench, IsJsbIgnoredParams) {
  runtime::js::ModuleTestBench module("replayModule", nullptr);
  module.jsb_ignored_info_ = nullptr;

  ASSERT_TRUE(module.IsJsbIgnoredParams("timestamp"));
  ASSERT_TRUE(module.IsJsbIgnoredParams("card_version"));
  ASSERT_TRUE(module.IsJsbIgnoredParams("containerID"));
  ASSERT_TRUE(module.IsJsbIgnoredParams("request_time"));
  ASSERT_TRUE(module.IsJsbIgnoredParams("header"));
  ASSERT_FALSE(module.IsJsbIgnoredParams("containerd"));

  rapidjson::Document ignored_info;
  ignored_info.Parse("[\"Test1\", \"Test2\"]");
  module.jsb_ignored_info_ = &ignored_info;
  ASSERT_TRUE(module.IsJsbIgnoredParams("Test1"));
  ASSERT_TRUE(module.IsJsbIgnoredParams("Test2"));
  ASSERT_FALSE(module.IsJsbIgnoredParams("Test3"));
}

TEST(LynxModuleTestBench, IsSameURL) {
  runtime::js::ModuleTestBench module("replayModule", nullptr);
  module.jsb_ignored_info_ = nullptr;

  ASSERT_TRUE(module.IsSameURL("http://www.lynx.com", "http://www.lynx.com"));
  ASSERT_TRUE(
      module.IsSameURL("http://www.lynx.com?p=1", "http://www.lynx.com?p=1"));
  ASSERT_FALSE(
      module.IsSameURL("http://www.lynx.com?p=1", "http://www.lynx.com?p=2"));
  ASSERT_TRUE(module.IsSameURL("http://www.lynx.com?timestamp=1234567",
                               "http://www.lynx.com?timestamp=7654321"));
  ASSERT_FALSE(module.IsSameURL("http://www.lynx.com?name=lynx1",
                                "http://www.lynx.com?name=lynx2"));

  rapidjson::Document ignored_info;
  ignored_info.Parse("[\"Test1\"]");
  module.jsb_ignored_info_ = &ignored_info;
  ASSERT_TRUE(module.IsSameURL("http://www.lynx.com?Test1=1",
                               "http://www.lynx.com?Test1=2"));
  ASSERT_TRUE(module.IsSameURL("http://www.lynx.com?Test1=1&timestamp=12345",
                               "http://www.lynx.com?Test1=2&timestamp=54321"));
}

TEST(LynxModuleTestBench, AppletBridgeWeakMatchIgnoresCallbackIdInJsonString) {
  auto runtime = testing::utils::makeJSRuntime();
  runtime::js::ModuleTestBench module("AppletBridgeModule", nullptr);
  runtime::js::LynxModule::MethodMetadata method(1, "postMessage");

  std::string runtime_arg_json =
      R"({"callbackId":999,"payload":"{\"callbackId\":999,\"amount\":1}"})";
  auto runtime_arg = runtime::js::Value::createFromJsonUtf8(
      *runtime, reinterpret_cast<const uint8_t*>(runtime_arg_json.c_str()),
      runtime_arg_json.size());
  ASSERT_TRUE(runtime_arg.has_value());
  runtime::js::Value args[] = {std::move(*runtime_arg)};

  rapidjson::Document recorded;
  recorded.Parse(
      R"({"Params":{"argc":1,"args":[{"callbackId":1,"payload":"{\"callbackId\":1,\"amount\":1}"}]}})");

  EXPECT_TRUE(module.IsAppletBridgeProtocolWeakMatch(method, runtime.get(),
                                                     args, 1, recorded));
}

TEST(LynxModuleTestBench, AppletBridgeWeakMatchFuzzesNumberInJsonString) {
  auto runtime = testing::utils::makeJSRuntime();
  runtime::js::ModuleTestBench module("AppletBridgeModule", nullptr);
  runtime::js::LynxModule::MethodMetadata method(1, "postMessage");

  std::string runtime_arg_json =
      R"({"callbackId":999,"payload":"{\"amount\":999}"})";
  auto runtime_arg = runtime::js::Value::createFromJsonUtf8(
      *runtime, reinterpret_cast<const uint8_t*>(runtime_arg_json.c_str()),
      runtime_arg_json.size());
  ASSERT_TRUE(runtime_arg.has_value());
  runtime::js::Value args[] = {std::move(*runtime_arg)};

  rapidjson::Document recorded;
  recorded.Parse(
      R"({"Params":{"argc":1,"args":[{"callbackId":1,"payload":"{\"amount\":1}"}]}})");

  EXPECT_TRUE(module.IsAppletBridgeProtocolWeakMatch(method, runtime.get(),
                                                     args, 1, recorded));
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
