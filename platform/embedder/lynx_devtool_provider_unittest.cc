// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_devtool/lynx_devtool_provider.h"

#include <memory>

#include "base/include/fml/message_loop.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/template_assembler.h"
#include "devtool/embedder/core/inspector_owner_embedder.h"
#include "platform/embedder/core/lynx_template_renderer.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"
#include "platform/embedder/public/lynx_view.h"
#include "platform/embedder/testing/lynx_ui_renderer_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace embedder {
namespace {

class TestTemplateRenderer : public LynxTemplateRenderer {
 public:
  explicit TestTemplateRenderer(tasm::UIDelegate* delegate)
      : LynxTemplateRenderer(TestSettings(), delegate, nullptr, nullptr) {}

  EmbedderDevTool* devtools() { return devtools_.get(); }
  devtool::LynxInspectorOwner* inspector_owner() { return inspector_owner_; }
  fml::RefPtr<fml::TaskRunner> ui_runner() {
    return shell_->GetRunners()->GetUITaskRunner();
  }
  void ReleaseDevTool() { devtools_.reset(); }
  bool HasInspectorObservers() {
    return shell_->GetTasm()->page_proxy()->element_manager()->GetDevToolFlag();
  }

 private:
  static Settings TestSettings() {
    Settings settings;
    settings.screen_size = {320, 480};
    settings.viewport_size = {320, 480};
    settings.enable_js = false;
    return settings;
  }
};

class LynxDevToolProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    base::UIThread::Init();
    auto& env = DevToolEnvEmbedder::GetInstance();
    debug_enabled_ = env.IsLynxDebugEnabled();
    devtool_enabled_ = IsEmbedderDevToolEnabled();
    logbox_enabled_ = IsEmbedderLogBoxEnabled();
    env.EnableLynxDebug(true);
    SetEmbedderDevToolEnabled(true);
    SetEmbedderLogBoxEnabled(false);
  }

  void TearDown() override {
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
    SetEmbedderLogBoxEnabled(logbox_enabled_);
    SetEmbedderDevToolEnabled(devtool_enabled_);
    DevToolEnvEmbedder::GetInstance().EnableLynxDebug(debug_enabled_);
  }

  bool debug_enabled_ = false;
  bool devtool_enabled_ = false;
  bool logbox_enabled_ = false;
};

TEST_F(LynxDevToolProviderTest, BuiltinOwnerFollowsRendererLifecycle) {
  pub::LynxView::Builder builder;
  TestLynxUIRenderer ui_renderer(builder.Impl());
  TestTemplateRenderer renderer(ui_renderer.GetUIDelegate());
  SetEmbedderDevToolEnabled(false);
  renderer.Initialize();
  EXPECT_EQ(renderer.devtools(), nullptr);
  EXPECT_EQ(renderer.inspector_owner(), nullptr);

  SetEmbedderDevToolEnabled(true);
  renderer.Reset();
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  ASSERT_NE(renderer.devtools(), nullptr);
  EXPECT_EQ(renderer.inspector_owner(),
            renderer.devtools()->GetInspectorOwner());
  auto* inspector =
      static_cast<devtool::InspectorOwnerEmbedder*>(renderer.inspector_owner());
  ASSERT_NE(inspector, nullptr);
  EXPECT_EQ(inspector->GetUITaskRunner(), renderer.ui_runner());
  ASSERT_TRUE(renderer.HasInspectorObservers());

  auto* devtools = renderer.devtools();
  const auto instance_id = renderer.GetInstanceId();
  SetEmbedderDevToolEnabled(false);
  DevToolEnvEmbedder::GetInstance().EnableLynxDebug(false);
  renderer.Reset();
  fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_NE(renderer.GetInstanceId(), instance_id);
  EXPECT_EQ(renderer.devtools(), devtools);
  EXPECT_EQ(renderer.inspector_owner(), inspector);
  EXPECT_EQ(inspector->GetUITaskRunner(), renderer.ui_runner());
  EXPECT_TRUE(renderer.HasInspectorObservers());

  renderer.ReleaseDevTool();
  EXPECT_EQ(renderer.inspector_owner(), nullptr);
  EXPECT_EQ(renderer.GetDevtoolSessionId(), 0);
  EXPECT_TRUE(renderer.GetDevtoolTemplateUrl().empty());
}
}  // namespace
}  // namespace embedder
}  // namespace lynx
