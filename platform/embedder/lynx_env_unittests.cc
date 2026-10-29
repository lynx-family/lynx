// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <optional>
#include <utility>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/value/table.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/tasm/config.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "platform/embedder/public/lynx_env.h"
#include "platform/embedder/public/lynx_value.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace {

struct HighMemoryResult {
  int calls{0};
  std::optional<lynx::pub::LynxValue> event;
};

void OnHighMemory(void* user_data, const lynx_value* event) {
  EXPECT_TRUE(lynx::base::UIThread::GetRunner()->RunsTasksOnCurrentThread());
  auto& result = *static_cast<HighMemoryResult*>(user_data);
  ++result.calls;
  result.event.emplace(*event);
}

class LynxEnvHighMemoryTest : public ::testing::Test {
 protected:
  void SetUp() override { lynx::base::UIThread::Init(); }
  void TearDown() override {
    lynx_env_set_high_memory_callback(nullptr, nullptr);
    DrainUI();
  }

  static void Emit() {
    lynx::fml::AutoResetWaitableEvent barrier;
    lynx::tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()
        ->PostTask([&barrier] {
          auto event = lynx::lepus::Dictionary::Create();
          event->SetValue("type", "large_vm");
          event->SetValue("key", "embedder");
          event->SetValue("bytes", uint64_t{1024 * 1024});
          lynx::tasm::performance::MemoryMonitor::ReportHighMemoryEvent(
              lynx::lepus::Value(std::move(event)));
          barrier.Signal();
        });
    barrier.Wait();
  }

  static void DrainUI() {
    lynx::fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  }
};

TEST_F(LynxEnvHighMemoryTest, DeliversDictionaryOnUIAndSupportsRetainingValue) {
  HighMemoryResult result;
  lynx_env_set_high_memory_callback(OnHighMemory, &result);
  Emit();
  EXPECT_EQ(result.calls, 0);
  DrainUI();
  ASSERT_EQ(result.calls, 1);
  ASSERT_TRUE(result.event);
  EXPECT_EQ(result.event->GetProperty("bytes").Number(), 1024 * 1024);
  EXPECT_EQ(result.event->GetProperty("key").StdString(), "embedder");
}

TEST_F(LynxEnvHighMemoryTest, RegistrationAndClearApplyToQueuedEvents) {
  HighMemoryResult first, second;
  auto& env = lynx::pub::LynxEnv::GetInstance();
  env.SetHighMemoryCallback(OnHighMemory, &first);
  Emit();
  env.SetHighMemoryCallback(OnHighMemory, &second);
  DrainUI();
  EXPECT_EQ(first.calls, 1);
  EXPECT_EQ(second.calls, 1);
  Emit();
  env.SetHighMemoryCallback(nullptr, nullptr);
  DrainUI();
  EXPECT_EQ(first.calls, 1);
  EXPECT_EQ(second.calls, 1);
}

TEST_F(LynxEnvHighMemoryTest, ClearDuringCallbackPreservesCurrentDelivery) {
  HighMemoryResult first, second;
  lynx_env_set_high_memory_callback(
      [](void* user_data, const lynx_value* event) {
        OnHighMemory(user_data, event);
        lynx_env_set_high_memory_callback(nullptr, nullptr);
      },
      &first);
  lynx_env_set_high_memory_callback(OnHighMemory, &second);
  Emit();
  Emit();
  DrainUI();
  EXPECT_EQ(first.calls, 1);
  EXPECT_EQ(second.calls, 1);
}

TEST_F(LynxEnvHighMemoryTest, CallbacksRunInRegistrationOrder) {
  std::vector<int> order;
  std::pair<std::vector<int>*, int> first{&order, 1}, second{&order, 2};
  auto callback = [](void* user_data, const lynx_value*) {
    auto& entry = *static_cast<std::pair<std::vector<int>*, int>*>(user_data);
    entry.first->push_back(entry.second);
  };
  lynx_env_set_high_memory_callback(callback, &first);
  lynx_env_set_high_memory_callback(callback, &second);
  Emit();
  DrainUI();
  EXPECT_EQ(order, (std::vector<int>{1, 2}));
}

TEST_F(LynxEnvHighMemoryTest, RegistrationDuringCallbackAffectsNextDelivery) {
  HighMemoryResult first, second;
  std::pair<HighMemoryResult*, HighMemoryResult*> results{&first, &second};
  lynx_env_set_high_memory_callback(
      [](void* user_data, const lynx_value* event) {
        auto& results =
            *static_cast<std::pair<HighMemoryResult*, HighMemoryResult*>*>(
                user_data);
        OnHighMemory(results.first, event);
        if (results.first->calls == 1) {
          lynx_env_set_high_memory_callback(OnHighMemory, results.second);
        }
      },
      &results);
  Emit();
  DrainUI();
  EXPECT_EQ(first.calls, 1);
  EXPECT_EQ(second.calls, 0);
  Emit();
  DrainUI();
  EXPECT_EQ(first.calls, 2);
  EXPECT_EQ(second.calls, 1);
}

}  // namespace

#if ENABLE_INSPECTOR
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#include "third_party/debug_router/src/debug_router/common/debug_router.h"

TEST(LynxEnv, DevtoolDefaultAppInfo) {
  lynx::embedder::DevToolEnvEmbedder::GetInstance();
  EXPECT_EQ(debugrouter::common::DebugRouter::GetInstance().GetAppInfoByKey(
                "sdkVersion"),
            lynx::tasm::Config::GetCurrentLynxVersion());
}

TEST(LynxEnv, Devtool) {
  lynx_env_enable_devtool(0);
  EXPECT_EQ(lynx_env_is_devtool_enabled(), 0);
  lynx_env_enable_devtool(1);
  EXPECT_EQ(lynx_env_is_devtool_enabled(), 1);
  EXPECT_EQ(debugrouter::common::DebugRouter::GetInstance().GetAppInfoByKey(
                "sdkVersion"),
            lynx::tasm::Config::GetCurrentLynxVersion());
}

TEST(LynxEnv, DevtoolCpp) {
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.SetDevtoolEnabled(false);
  EXPECT_EQ(lynx_env.IsDevtoolEnabled(), false);
  lynx_env.SetDevtoolEnabled(true);
  EXPECT_EQ(lynx_env.IsDevtoolEnabled(), true);
}

TEST(LynxEnv, Logbox) {
  lynx_env_enable_logbox(0);
  EXPECT_EQ(lynx_env_is_logbox_enabled(), 0);
  lynx_env_enable_logbox(1);
  EXPECT_EQ(lynx_env_is_logbox_enabled(), 1);
}

TEST(LynxEnv, LogboxCpp) {
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.SetLogboxEnabled(false);
  EXPECT_EQ(lynx_env.IsLogboxEnabled(), false);
  lynx_env.SetLogboxEnabled(true);
  EXPECT_EQ(lynx_env.IsLogboxEnabled(), true);
}
#endif

TEST(LynxEnv, Version) {
  EXPECT_STREQ(lynx_env_get_sdk_version(),
               lynx::tasm::Config::GetCurrentLynxVersion().c_str());
  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  EXPECT_STREQ(lynx_env.GetVersion(),
               lynx::tasm::Config::GetCurrentLynxVersion().c_str());
}

TEST(LynxEnv, RegisterNativeModule) {
  lynx_env_register_native_module(
      "test",
      [](napi_env, napi_value exports, const char* module_name, void* opaque) {
        return exports;
      },
      nullptr);

  auto& lynx_env = lynx::pub::LynxEnv::GetInstance();
  lynx_env.RegisterNativeModule(
      "test",
      [](napi_env, napi_value exports, const char* module_name, void* opaque) {
        return exports;
      },
      nullptr);
}
