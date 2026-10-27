// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "devtool/lynx_devtool/lynx_devtool_ng.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/lynx_env_config.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/native_facade_empty_implementation.h"
#include "devtool/base_devtool/native/test/mock_receiver.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace devtool {
namespace testing {

class LynxDevToolNGTest : public ::testing::Test {
 public:
  LynxDevToolNGTest() {}
  ~LynxDevToolNGTest() override {}
  void SetUp() override {}
};

TEST_F(LynxDevToolNGTest, GlobalMessageHandler) {
  constexpr char kTypeGetStopAtEntry[] = "GetStopAtEntry";
  constexpr char kTypeSetStopAtEntry[] = "SetStopAtEntry";
  constexpr char kTypeGetFetchDebugInfo[] = "GetFetchDebugInfo";
  constexpr char kTypeSetFetchDebugInfo[] = "SetFetchDebugInfo";

  auto& global_dispatcher =
      lynx::devtool::AbstractDevTool::GetGlobalMessageDispatcherInstance();

  auto devtool1 = std::make_shared<lynx::testing::LynxDevToolNGMock>();
  auto it = global_dispatcher.handler_map_.find(kTypeGetStopAtEntry);
  EXPECT_NE(it, global_dispatcher.handler_map_.end());
  const auto& get_stop_at_entry_handler = it->second;
  it = global_dispatcher.handler_map_.find(kTypeSetStopAtEntry);
  EXPECT_NE(it, global_dispatcher.handler_map_.end());
  const auto& set_stop_at_entry_handler = it->second;
  it = global_dispatcher.handler_map_.find(kTypeGetFetchDebugInfo);
  EXPECT_NE(it, global_dispatcher.handler_map_.end());
  const auto& get_fetch_debug_info_handler = it->second;
  it = global_dispatcher.handler_map_.find(kTypeSetFetchDebugInfo);
  EXPECT_NE(it, global_dispatcher.handler_map_.end());
  const auto& set_fetch_debug_info_handler = it->second;

  auto devtool2 = std::make_shared<lynx::testing::LynxDevToolNGMock>();
  it = global_dispatcher.handler_map_.find(kTypeGetStopAtEntry);
  EXPECT_EQ(it->second, get_stop_at_entry_handler);
  it = global_dispatcher.handler_map_.find(kTypeSetStopAtEntry);
  EXPECT_EQ(it->second, set_stop_at_entry_handler);
  it = global_dispatcher.handler_map_.find(kTypeGetFetchDebugInfo);
  EXPECT_EQ(it->second, get_fetch_debug_info_handler);
  it = global_dispatcher.handler_map_.find(kTypeSetFetchDebugInfo);
  EXPECT_EQ(it->second, set_fetch_debug_info_handler);
}

TEST_F(LynxDevToolNGTest, AttachSetsMediatorAttached) {
  auto devtool = std::make_shared<lynx::devtool::LynxDevToolNG>(true);
  devtool->Attach("https://example/template.js");
  ASSERT_NE(devtool->devtool_mediator_, nullptr);
  EXPECT_TRUE(devtool->devtool_mediator_->attached_);
}

TEST_F(LynxDevToolNGTest, OnTasmCreatedInitializesMediator) {
  lynx::base::UIThread::Init();
  auto devtool = std::make_shared<lynx::devtool::LynxDevToolNG>(true);
  lynx::shell::LynxShellBuilder builder;
  auto facade = std::make_unique<lynx::shell::NativeFacadeEmptyImpl>();
  auto painting_context =
      std::make_unique<lynx::tasm::PaintingContextPlatformImpl>();
  auto painting_context_creator = [&](lynx::shell::LynxShell* shell) {
    return std::move(painting_context);
  };
  lynx::tasm::LynxEnvConfig env_config(60, 90, 1.f, 1.f);
  lynx::shell::ShellOption option;
  std::unique_ptr<lynx::shell::LynxShell> shell(
      builder.SetNativeFacade(std::move(facade))
          .SetPaintingContextCreator(painting_context_creator)
          .SetLynxEnvConfig(env_config)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetStrategy(lynx::base::ThreadStrategyForRendering::ALL_ON_UI)
          .SetEngineActor([](auto&) {})
          .SetShellOption(option)
          .build());

  devtool->OnTasmCreated(reinterpret_cast<intptr_t>(shell.get()));
  ASSERT_NE(devtool->devtool_mediator_, nullptr);
  EXPECT_TRUE(devtool->devtool_mediator_->fully_initialized_);
  EXPECT_NE(devtool->devtool_mediator_->GetTasmExecutor(), nullptr);
  EXPECT_NE(devtool->devtool_mediator_->GetUIExecutor(), nullptr);
  EXPECT_NE(devtool->devtool_mediator_->GetDevToolExecutor(), nullptr);
  EXPECT_NE(devtool->devtool_mediator_->GetJSDebugger(), nullptr);
  EXPECT_NE(devtool->devtool_mediator_->GetLepusDebugger(), nullptr);
}

}  // namespace testing
}  // namespace devtool
}  // namespace lynx
