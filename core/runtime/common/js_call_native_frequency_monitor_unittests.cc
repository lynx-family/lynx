// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/js_call_native_frequency_monitor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace runtime {
namespace test {
namespace {

TEST(JsCallNativeFrequencyMonitorTest, ReportsAtDefaultThresholdPerMethod) {
  auto& env = tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv("lynx_debug_enabled", true);
  env.SetBoolLocalEnv("enable_devtool", true);

  JsCallNativeFrequencyMonitor monitor(800);

  for (int i = 0; i < 799; ++i) {
    EXPECT_FALSE(
        monitor.Record(10, "callLepusMethod", "Foo", "stack").has_value());
  }

  auto err_opt = monitor.Record(10, "callLepusMethod", "Foo", "stack");
  ASSERT_TRUE(err_opt.has_value());
  EXPECT_EQ(err_opt->error_level_, base::LynxErrorLevel::Warn);
  EXPECT_NE(err_opt->error_message_.find("callLepusMethod"), std::string::npos);
  EXPECT_EQ(err_opt->custom_info_["lynx_context_call_api_name"],
            "callLepusMethod");
  EXPECT_EQ(err_opt->custom_info_["lynx_context_method_name"], "Foo");
  EXPECT_EQ(err_opt->custom_info_["lynx_context_count_in_window"], "800");
}

TEST(JsCallNativeFrequencyMonitorTest, CountsPerMethodNameSeparately) {
  auto& env = tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv("lynx_debug_enabled", true);
  env.SetBoolLocalEnv("enable_devtool", true);

  JsCallNativeFrequencyMonitor monitor(800);

  for (int i = 0; i < 300; ++i) {
    EXPECT_FALSE(monitor.Record(10, "callLepusMethod", "A", "").has_value());
  }
  for (int i = 0; i < 799; ++i) {
    EXPECT_FALSE(monitor.Record(10, "callLepusMethod", "B", "").has_value());
  }

  EXPECT_TRUE(monitor.Record(10, "callLepusMethod", "B", "").has_value());
  EXPECT_FALSE(monitor.Record(10, "callLepusMethod", "A", "").has_value());
}

TEST(JsCallNativeFrequencyMonitorTest, WindowResetDropsCounts) {
  auto& env = tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv("lynx_debug_enabled", true);
  env.SetBoolLocalEnv("enable_devtool", true);

  JsCallNativeFrequencyMonitor monitor(800);

  for (int i = 0; i < 100; ++i) {
    EXPECT_FALSE(monitor.Record(1, "callLepusMethod", "Foo", "").has_value());
  }
  EXPECT_FALSE(monitor.Record(6001, "callLepusMethod", "Foo", "").has_value());
}

}  // namespace
}  // namespace test
}  // namespace runtime
}  // namespace lynx
