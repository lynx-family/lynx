// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/common/bindings/modules/lynx_native_module_manager.h"
#if ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/recorder_constants.h"
#include "core/services/recorder/testbench_base_recorder.h"
#endif
#include "core/shell/lynx_shell.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/testing/mock_native_facade.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class MockLazyBundleLoader : public LazyBundleLoader {
 public:
  MockLazyBundleLoader() = default;
  ~MockLazyBundleLoader() = default;
  void RequireTemplate(RadonLazyComponent* lazy_bundle, const std::string& url,
                       int instance_id) override {}

};  // MockLazyBundleLoader
}  // namespace tasm
namespace shell {

class LynxShellBuilderTest : public ::testing::Test {
 protected:
  LynxShellBuilderTest() = default;
  ~LynxShellBuilderTest() override = default;

  void SetUp() override {
    lynx_env_config_ =
        std::make_unique<lynx::tasm::LynxEnvConfig>(60, 90, 1.f, 1.f);
    loader_ = std::make_shared<lynx::tasm::MockLazyBundleLoader>();

    option_ = std::make_unique<lynx::shell::ShellOption>();

    shell_builder_ = std::make_unique<LynxShellBuilder>();
  }

  void TearDown() override { shell_ = nullptr; }

  intptr_t facade_;
  intptr_t painting_context_;
  std::unique_ptr<lynx::tasm::LynxEnvConfig> lynx_env_config_ = nullptr;
  std::shared_ptr<lynx::tasm::MockLazyBundleLoader> loader_ = nullptr;
  bool enable_pre_update_data_ = true;
  bool enable_layout_only_ = false;
  std::string locale_ = "LynxShellBuilderTotalTest";
  lynx::base::ThreadStrategyForRendering strategy_ =
      lynx::base::ThreadStrategyForRendering::ALL_ON_UI;
  std::unique_ptr<lynx::shell::ShellOption> option_ = nullptr;

  std::unique_ptr<LynxShell> shell_;
  std::unique_ptr<LynxShellBuilder> shell_builder_ = nullptr;

};  // LynxShellBuilderTest

TEST_F(LynxShellBuilderTest, LynxShellBuilderTotalTest) {
  auto facade = std::make_unique<MockNativeFacade>();
  facade_ = reinterpret_cast<intptr_t>(facade.get());

  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetEnableElementManagerVsyncMonitor(true)
                   .SetLazyBundleLoader(loader_)
                   .SetEnablePreUpdateData(enable_pre_update_data_)
                   .SetEnableLayoutOnly(enable_layout_only_)
                   .SetTasmLocale(locale_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .build());

  shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, shell_->runners_.GetUITaskRunner());

  // SetNativeFacade() test
  EXPECT_EQ(reinterpret_cast<intptr_t>(shell_->facade_actor_->Impl()), facade_);

  auto out_lynx_engine = shell_->engine_actor_->Impl();
  // SetPaintingContextCreator() test
  EXPECT_EQ(reinterpret_cast<intptr_t>(out_lynx_engine->GetTasm()
                                           ->page_proxy()
                                           ->element_manager()
                                           ->catalyzer()
                                           ->painting_context()
                                           ->platform_impl_.get()),
            painting_context_);
  // SetLynxEnvConfig() test
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->GetLynxEnvConfig()
                .ScreenWidth(),
            lynx_env_config_->ScreenWidth());
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->GetLynxEnvConfig()
                .ScreenHeight(),
            lynx_env_config_->ScreenHeight());
  // SetLazyBundleLoader() test
  EXPECT_EQ(out_lynx_engine->GetTasm()->component_loader_.get(), loader_.get());
  // SetEnablePreUpdateData() test
  EXPECT_EQ(out_lynx_engine->GetTasm()->enable_pre_update_data_,
            enable_pre_update_data_);
  // SetEnableLayoutOnly() test
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->GetEnableLayoutOnly(),
            enable_layout_only_);
  // SetTasmLocale() test
  EXPECT_EQ(out_lynx_engine->GetTasm()->locale_, locale_);

  // SetLayoutContextPlatformImpl() test
  auto out_layout_context = shell_->layout_actor_->Impl();
  EXPECT_EQ(out_layout_context->page_options_.GetInstanceID(),
            shell_->instance_id_);
  EXPECT_EQ(out_layout_context->platform_impl_.get(), nullptr);

  // SetStrategy() test
  EXPECT_EQ(shell_->ThreadStrategy(), strategy_);

  EXPECT_EQ(shell_->layout_result_manager_, nullptr);
  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_, nullptr);

  // SystemInfo.pixelRatio test
  auto& element_manager = shell_->GetTasm()->page_proxy()->element_manager();
  ASSERT_EQ(shell_->GetTasm()->GetDevicePixelRatio(), 1.0f);
  ASSERT_EQ(element_manager->GetLynxEnvConfig().DevicePixelRatio(), 1.0f);

  shell_->UpdateScreenMetrics(800, 600, 1.75f);

  fml::AutoResetWaitableEvent arwe;
  shell_->RunOnTasmThread([&arwe]() { arwe.Signal(); });
  arwe.Wait();

  ASSERT_EQ(shell_->GetTasm()->GetDevicePixelRatio(), 1.75f);
  ASSERT_EQ(element_manager->GetLynxEnvConfig().DevicePixelRatio(), 1.75f);
}

#if ENABLE_TESTBENCH_RECORDER
class LynxShellBuilderRecorderTest
    : public LynxShellBuilderTest,
      public ::testing::WithParamInterface<bool> {
 protected:
  void TearDown() override {
    LynxShellBuilderTest::TearDown();
    auto& recorder = tasm::recorder::TestBenchBaseRecorder::GetInstance();
    recorder.thread_.GetTaskRunner()->PostSyncTask([]() {});
    recorder.is_recording_ = false;
    recorder.Clear();
  }
};

TEST_P(LynxShellBuilderRecorderTest,
       InitializesRecorderBeforeOptionalBTSRuntime) {
  auto& recorder = tasm::recorder::TestBenchBaseRecorder::GetInstance();
  recorder.Clear();
  recorder.StartRecord();

  option_->enable_js_ = GetParam();
  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::make_unique<MockNativeFacade>())
                   .SetNativeModuleManager(
                       std::make_unique<pub::LynxNativeModuleManager>())
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .build());

  const int64_t record_id = reinterpret_cast<int64_t>(shell_.get());
  EXPECT_EQ(shell_->GetTasm()->GetRecordID(), record_id);
  EXPECT_EQ(shell_->layout_actor_->Impl()->record_id_, record_id);
  ASSERT_NE(shell_->GetTasm()->lepus_module_manager_, nullptr);
  EXPECT_EQ(shell_->GetTasm()->lepus_module_manager_->record_id_, record_id);

  if (GetParam()) {
    base::UIThread::Init();
    auto bts_module_manager = std::make_shared<pub::LynxNativeModuleManager>();
    shell_->InitRuntime(
        "", nullptr, bts_module_manager,
        [](const std::shared_ptr<LynxActor<BTSRuntime>>&) {},
        std::vector<std::string>(), LynxRuntimeFlags::PENDING_JS_TASK, "");
    EXPECT_EQ(bts_module_manager->record_id_, record_id);
    shell_->runtime_actor_->ActSync([record_id](auto& runtime) {
      ASSERT_NE(runtime, nullptr);
      EXPECT_EQ(runtime->record_id_, record_id);
    });
  }

  recorder.thread_.GetTaskRunner()->PostSyncTask([]() {});
  auto& actions =
      recorder.lynx_view_table_[record_id][tasm::recorder::kActionList];
  ASSERT_TRUE(actions.IsArray());
  ASSERT_EQ(actions.Size(), 1u);
  EXPECT_STREQ(actions[0][tasm::recorder::kFunctionName].GetString(),
               tasm::recorder::kFuncSetThreadStrategy);
  const auto& params = actions[0][tasm::recorder::kParams];
  EXPECT_EQ(params[tasm::recorder::kParamThreadStrategy].GetInt(),
            static_cast<int32_t>(strategy_));
  EXPECT_EQ(params[tasm::recorder::kParamEnableJSRuntime].GetBool(),
            GetParam());
}

INSTANTIATE_TEST_SUITE_P(WithAndWithoutBTSRuntime, LynxShellBuilderRecorderTest,
                         ::testing::Bool());
#endif

TEST_F(LynxShellBuilderTest, DisableJSRuntimeUsesUIRunnerForJS) {
  auto facade = std::make_unique<MockNativeFacade>();
  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };
  option_->enable_js_ = false;

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .build());

  EXPECT_EQ(shell_->runners_.GetJSTaskRunner()->GetLoop(),
            shell_->runners_.GetUITaskRunner()->GetLoop());
}

TEST_F(LynxShellBuilderTest,
       LynxShellBuilderDisableForceLayoutOnBackgroundThreadTest) {
  auto facade = std::make_unique<MockNativeFacade>();
  facade_ = reinterpret_cast<intptr_t>(facade.get());

  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetEnableElementManagerVsyncMonitor(true)
                   .SetLazyBundleLoader(loader_)
                   .SetEnablePreUpdateData(enable_pre_update_data_)
                   .SetEnableLayoutOnly(enable_layout_only_)
                   .SetTasmLocale(locale_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetForceLayoutOnBackgroundThread(false)
                   .build());

  shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, shell_->runners_.GetUITaskRunner());

  EXPECT_EQ(shell_->layout_result_manager_, nullptr);
  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_, nullptr);
}

TEST_F(LynxShellBuilderTest,
       LynxShellBuilderEnableForceLayoutOnBackgroundThreadTest) {
  auto facade = std::make_unique<MockNativeFacade>();
  facade_ = reinterpret_cast<intptr_t>(facade.get());

  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };

  shell_.reset((*shell_builder_)
                   .SetNativeFacade(std::move(facade))
                   .SetPaintingContextCreator(painting_context_creator)
                   .SetLynxEnvConfig(*lynx_env_config_)
                   .SetEnableElementManagerVsyncMonitor(true)
                   .SetLazyBundleLoader(loader_)
                   .SetEnablePreUpdateData(enable_pre_update_data_)
                   .SetEnableLayoutOnly(enable_layout_only_)
                   .SetTasmLocale(locale_)
                   .SetLayoutContextPlatformImpl(nullptr)
                   .SetStrategy(strategy_)
                   .SetShellOption(*option_)
                   .SetForceLayoutOnBackgroundThread(true)
                   .build());

  shell_->runtime_actor_ = std::make_shared<LynxActor<BTSRuntime>>(
      nullptr, shell_->runners_.GetUITaskRunner());

  EXPECT_NE(shell_->layout_result_manager_, nullptr);
  EXPECT_NE(shell_->layout_mediator_->layout_result_manager_, nullptr);

  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_,
            shell_->layout_result_manager_);
  EXPECT_EQ(shell_->layout_mediator_->layout_result_manager_,
            shell_->layout_mediator_->operation_queue_);
}

}  // namespace shell
}  // namespace lynx
