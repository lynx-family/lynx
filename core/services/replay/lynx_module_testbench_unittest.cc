// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#include "core/services/replay/lynx_module_testbench.h"

#include "core/runtime/jsi/jsi.h"
#undef private

#include "base/include/closure.h"
#include "base/include/debug/lynx_error.h"
#include "base/include/fml/memory/task_runner_checker.h"
#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "core/runtime/bindings/jsi/modules/module_delegate.h"
#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {

static fml::AutoResetWaitableEvent latch;

class MockDelegate : public piper::ModuleDelegate {
 public:
  virtual int64_t RegisterJSCallbackFunction(piper::Function func) override {
    return 1;
  }
  virtual void CallJSCallback(
      const std::shared_ptr<piper::ModuleCallback>& callback,
      int64_t id_to_delete =
          piper::ModuleCallback::kInvalidCallbackId) override {
    latch.Signal();
  }
  virtual void OnErrorOccurred(base::LynxError error) override {}
  virtual void OnMethodInvoked(const std::string& module_name,
                               const std::string& method_name,
                               int32_t code) override {}
  virtual void FlushJSBTiming(piper::NativeModuleInfo timing) override {}
  virtual void RunOnJSThread(base::closure func) override {}
  virtual void RunOnPlatformThread(base::closure func) override {}
};

TEST(LynxModuleTestBench, StrictMode) {
  piper::ModuleTestBench module("replayModule", nullptr);

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
  piper::ModuleTestBench module("replayModule", mock_delegate);
  piper::Function function(nullptr);
  module.InvokeJsbCallback(std::move(function),
                           rapidjson::Value(rapidjson::kNullType));
  ASSERT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(3)));
  module.InvokeJsbCallback(std::move(function),
                           rapidjson::Value(rapidjson::kNullType), 1 * 1000);
  ASSERT_FALSE(latch.WaitWithTimeout(fml::TimeDelta::FromSeconds(3)));
}

TEST(LynxModuleTestBench, IsJsbIgnoredParams) {
  piper::ModuleTestBench module("replayModule", nullptr);
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
  piper::ModuleTestBench module("replayModule", nullptr);
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

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
