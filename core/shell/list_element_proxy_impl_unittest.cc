// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/shell/list_element_proxy_impl.h"

#include <thread>

#include "base/include/value/base_value.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class ListElementProxyImplTest : public ::testing::Test {
 protected:
  ListElementProxyImplTest() = default;
  ~ListElementProxyImplTest() override = default;
  void SetUp() override {
    base::UIThread::Init();

    auto lynx_engine_creator = [](auto delegate) {
      return std::make_unique<LynxEngine>(nullptr, std::move(delegate), nullptr,
                                          shell::kUnknownInstanceId);
    };

    // FIXME(heshan): tricky, here must ensure manufactor not create thread
    // POSIX thread exit may cause crash...
    lynx::shell::ShellOption option;
    shell_.reset(
        lynx::shell::LynxShellBuilder()
            .SetLynxEngineCreator(lynx_engine_creator)
            .SetEngineActor([this](auto& actor) {
              list_element_proxy_ =
                  std::make_unique<ListElementProxyImpl>(actor);
            })
            .SetShellOption(option)
            .SetPropBundleCreator(
                std::make_shared<lynx::tasm::PropBundleCreatorDefault>())
            .build());

    // hook runners, use for test
    shell_->runners_ =
        MockRunnerManufactor(base::ThreadStrategyForRendering::MULTI_THREADS);
    shell_->engine_actor_->runner_ = shell_->runners_.GetTASMTaskRunner();
  }
  void TearDown() override {}
  std::unique_ptr<LynxShell> shell_;
  std::unique_ptr<ListElementProxyImpl> list_element_proxy_;
};

TEST_F(ListElementProxyImplTest, ScrollByListContainer) {
  list_element_proxy_->ScrollByListContainer(0, 0, 0, 0, 0);
}

TEST_F(ListElementProxyImplTest, ScrollToPosition) {
  list_element_proxy_->ScrollToPosition(0, 0, 0, 0, false);
}

TEST_F(ListElementProxyImplTest, ScrollStopped) {
  list_element_proxy_->ScrollStopped(0);
}

TEST_F(ListElementProxyImplTest, ObtainListChild) {
  list_element_proxy_->GetLegacy()->ObtainListChild(0, 0, 0, false);
}

TEST_F(ListElementProxyImplTest, ObtainListChildAsync) {
  list_element_proxy_->GetLegacy()->ObtainListChildAsync(0, 0, 0, false);
}

TEST_F(ListElementProxyImplTest, RecycleListChild) {
  list_element_proxy_->GetLegacy()->RecycleListChild(0, 0);
}

TEST_F(ListElementProxyImplTest, RecycleListChildAsync) {
  list_element_proxy_->GetLegacy()->RecycleListChildAsync(0, 0);
}

TEST_F(ListElementProxyImplTest, RenderListChild) {
  list_element_proxy_->GetLegacy()->RenderListChild(0, 0, false);
}

TEST_F(ListElementProxyImplTest, UpdateListChild) {
  list_element_proxy_->GetLegacy()->UpdateListChild(0, 0, 0, 0);
}

}  // namespace shell
}  // namespace lynx
