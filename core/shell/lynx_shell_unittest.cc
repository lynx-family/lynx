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
#include "core/runtime/vm/lepus/lepus_value.h"
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
    facade_ = new MockNativeFacade;

    auto lynx_engine_creator = [](auto delegate) {
      return std::make_unique<LynxEngine>(nullptr, std::move(delegate), nullptr,
                                          shell::kUnknownInstanceId);
    };

    // FIXME(heshan): tricky, here must ensure manufactor not create thread
    // POSIX thread exit may cause crash...
    lynx::shell::ShellOption option;
    shell_.reset(
        lynx::shell::LynxShellBuilder()
            .SetNativeFacade(std::unique_ptr<NativeFacade>(facade_))
            .SetLynxEngineCreator(lynx_engine_creator)
            .SetLayoutContextPlatformImpl(nullptr)
            .SetStrategy(base::ThreadStrategyForRendering::ALL_ON_UI)
            .SetEngineActor([](auto& actor) {})
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
    shell_->runtime_actor_ = std::make_shared<LynxActor<runtime::LynxRuntime>>(
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

}  // namespace shell
}  // namespace lynx
