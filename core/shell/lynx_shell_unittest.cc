// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

// FIXME(heshan):will delete after refact manufactor
// use for hook
#define private public
#define protected public

#include "core/shell/lynx_shell.h"

#include <thread>

#include "base/include/debug/lynx_error.h"
#include "base/include/value/base_value.h"
#include "core/public/pub_value.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/performance/memory_monitor/memory_record.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/testing/mock_native_facade.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class LynxShellTest : public ::testing::Test {
 protected:
  LynxShellTest() = default;
  ~LynxShellTest() override = default;

  void SetUp() override {
    base::UIThread::Init();
    lynx::tasm::performance::MemoryMonitor::SetForceEnable(false);
    auto& env = lynx::tasm::LynxEnv::GetInstance();
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, false);
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, false);
    facade_ = new MockNativeFacade;

    // FIXME(heshan): tricky, here must ensure manufactor not create thread
    // POSIX thread exit may cause crash...
    lynx::shell::ShellOption option;
    shell_.reset(
        lynx::shell::LynxShellBuilder()
            .SetNativeFacade(std::unique_ptr<NativeFacade>(facade_))
            .SetLayoutContextPlatformImpl(nullptr)
            .SetStrategy(base::ThreadStrategyForRendering::ALL_ON_UI)
            .SetShellOption(option)
            .SetPropBundleCreator(
                std::make_shared<lynx::tasm::PropBundleCreatorDefault>())
            .build());

    // hook runners, use for test
    shell_->runners_ =
        MockRunnerManufactor(base::ThreadStrategyForRendering::MULTI_THREADS);
    shell_->facade_actor_->runner_ = shell_->runners_.GetUITaskRunner();
    shell_->engine_actor_->runner_ = shell_->runners_.GetTASMTaskRunner();
    shell_->layout_actor_->runner_ = shell_->runners_.GetLayoutTaskRunner();
    shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
        nullptr, shell_->runners_.GetJSTaskRunner());
    tasm_mediator_ = shell_->tasm_mediator_;
    arwe_ = facade_->arwe;
  }

  void TearDown() override {
    shell_->Destroy();
    arwe_->Wait();
    // check destroy life cycle

    ASSERT_TRUE(shell_->facade_actor_->impl_ == nullptr);

    // check call after destroy, no crash is ok
    shell_->OnEnterForeground();

    shell_ = nullptr;
    lynx::tasm::performance::MemoryMonitor::SetForceEnable(false);
    auto& env = lynx::tasm::LynxEnv::GetInstance();
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, false);
    env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, false);
  }

  MockNativeFacade* facade_;
  std::unique_ptr<LynxShell> shell_;
  TasmMediator* tasm_mediator_ = nullptr;
  std::shared_ptr<fml::AutoResetWaitableEvent> arwe_;
};

// for async operation, use arwe wait for result
TEST_F(LynxShellTest, OnTemplateLoaded) {
  std::string url = "url";
  tasm_mediator_->OnTemplateLoaded(url);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<std::string>((*facade_)["url"]), url);
}

TEST_F(LynxShellTest, OnSSRHydrateFinished) {
  std::string url = "url";
  tasm_mediator_->OnSSRHydrateFinished(url);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<std::string>((*facade_)["url"]), url);
}

TEST_F(LynxShellTest, OnErrorOccurred) {
  int32_t error_code = 777;
  std::string msg = "shell test error msg";
  tasm_mediator_->OnErrorOccurred(
      base::LynxError(error_code, msg, "", base::LynxErrorLevel::Error));
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<int32_t>((*facade_)["error_code"]), error_code);
  ASSERT_EQ(std::any_cast<std::string>((*facade_)["msg"]), msg);
}

TEST_F(LynxShellTest, OnConfigUpdated) {
  lepus::Value data = lepus::Value("test data");
  tasm_mediator_->OnConfigUpdated(data);
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
  ASSERT_EQ(std::any_cast<lepus::Value>((*facade_)["data"]), data);
}

TEST_F(LynxShellTest, OnUpdateDataWithoutChange) {
  tasm_mediator_->OnUpdateDataWithoutChange();
  arwe_->Wait();
  ASSERT_TRUE(*facade_);
}

TEST_F(LynxShellTest, GetAllPerformanceEntries) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, true);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, true);

  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    auto entry = controller->GetValueFactory()->CreateMap();
    entry->PushStringToMap("entryType", "metric");
    entry->PushStringToMap("name", "testMetric");
    entry->PushDoubleToMap("startTime", 1.5);
    controller->OnPerformanceEvent(std::move(entry));
  });

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_EQ(entries.Array()->size(), 1u);

  lepus::Value first_entry = entries.Array()->get(0);
  ASSERT_TRUE(first_entry.IsTable());
  ASSERT_EQ(first_entry.Table()->GetValue("entryType").String().str(),
            "metric");
  ASSERT_EQ(first_entry.Table()->GetValue("name").String().str(), "testMetric");
  ASSERT_DOUBLE_EQ(first_entry.Table()->GetValue("startTime").Number(), 1.5);
  ASSERT_EQ(first_entry.Table()->GetValue("instanceId").Int32(),
            shell_->perf_controller_actor_->GetInstanceId());
}

TEST_F(LynxShellTest, GetAllPerformanceEntriesReturnsEmptyWhenDevToolDisabled) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, false);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, false);

  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    auto entry = controller->GetValueFactory()->CreateMap();
    entry->PushStringToMap("entryType", "metric");
    entry->PushStringToMap("name", "testMetric");
    entry->PushDoubleToMap("startTime", 1.5);
    controller->OnPerformanceEvent(std::move(entry));
  });

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_EQ(entries.Array()->size(), 0u);
}

TEST_F(LynxShellTest, MemoryMonitorTeardownAfterAllocation) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, true);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, true);

  lynx::tasm::performance::MemoryMonitor::SetForceEnable(true);
  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    controller->GetMemoryMonitor().AllocateMemory(
        lynx::tasm::performance::MemoryRecord("test", 1024));
  });

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_GT(entries.Array()->size(), 0u);
}

TEST_F(LynxShellTest, ResetTimingBeforeReloadClearsPerformanceEntries) {
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDebugEnabled, true);
  env.SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxDevToolEnable, true);

  shell_->perf_controller_actor_->ActSync([](auto& controller) {
    auto entry = controller->GetValueFactory()->CreateMap();
    entry->PushStringToMap("entryType", "metric");
    entry->PushStringToMap("name", "beforeReload");
    controller->OnPerformanceEvent(std::move(entry));
  });

  shell_->ResetTimingBeforeReload();
  shell_->perf_controller_actor_->ActSync([](auto& controller) {});

  lepus::Value entries = shell_->GetAllPerformanceEntries();
  ASSERT_TRUE(entries.IsArray());
  ASSERT_EQ(entries.Array()->size(), 0u);
}

}  // namespace shell
}  // namespace lynx
