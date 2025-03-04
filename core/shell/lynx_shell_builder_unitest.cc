// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
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
    auto facade = std::make_unique<MockNativeFacade>();
    facade_ = reinterpret_cast<intptr_t>(facade.get());
    auto painting_context =
        std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
    painting_context_ = reinterpret_cast<intptr_t>(painting_context.get());
    auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
      return std::move(painting_context);
    };
    lynx_env_config_ =
        std::make_unique<lynx::tasm::LynxEnvConfig>(60, 90, 1.f, 1.f);
    loader_ = std::make_shared<lynx::tasm::MockLazyBundleLoader>();

    option_ = std::make_unique<lynx::shell::ShellOption>();

    shell_builder_ = std::make_unique<LynxShellBuilder>();
    shell_.reset((*shell_builder_)
                     .SetNativeFacade(std::move(facade))
                     .SetPaintingContextCreator(painting_context_creator)
                     .SetLynxEnvConfig(*lynx_env_config_)
                     .SetEnableDiffWithoutLayout(enable_diff_without_layout_)
                     .SetEnableElementManagerVsyncMonitor(true)
                     .SetLazyBundleLoader(loader_)
                     .SetEnablePreUpdateData(enable_pre_update_data_)
                     .SetEnableLayoutOnly(enable_layout_only_)
                     .SetTasmLocale(locale_)
                     .SetLayoutContextPlatformImpl(nullptr)
                     .SetStrategy(strategy_)
                     .SetEngineActor([](auto& actor) {})
                     .SetShellOption(*option_)
                     .build());

    shell_->runtime_actor_ = std::make_shared<LynxActor<runtime::LynxRuntime>>(
        nullptr, shell_->runners_.GetUITaskRunner());
  }

  void TearDown() override { shell_ = nullptr; }

  intptr_t facade_;
  intptr_t painting_context_;
  std::unique_ptr<lynx::tasm::LynxEnvConfig> lynx_env_config_ = nullptr;
  bool enable_diff_without_layout_ = true;
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
  // SetEnableDiffWithoutLayout() test
  EXPECT_EQ(out_lynx_engine->GetTasm()
                ->page_proxy()
                ->element_manager()
                ->enable_diff_without_layout_,
            enable_diff_without_layout_);
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
  EXPECT_EQ(out_layout_context->platform_impl_.get(), nullptr);

  // SetStrategy() test
  EXPECT_EQ(shell_->ThreadStrategy(), strategy_);
  // SetShellOption() test
  EXPECT_EQ(shell_builder_->shell_option_.enable_auto_concurrency_,
            this->option_->enable_auto_concurrency_);
}

}  // namespace shell
}  // namespace lynx
