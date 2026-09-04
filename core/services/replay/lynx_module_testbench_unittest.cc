// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#include "core/services/replay/lynx_module_testbench.h"

#include "core/runtime/js/jsi/jsi.h"
#undef private

#include "base/include/closure.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/fml/memory/task_runner_checker.h"
#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "core/runtime/js/bindings/modules/module_delegate.h"
#include "testing/utils/make_js_runtime.h"
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
