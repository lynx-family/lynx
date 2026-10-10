// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/lynx_env.h"
#include "platform/embedder/public/lynx_view.h"
#include "platform/embedder/testing/lynx_ui_renderer_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace {

class RecordingViewClient : public lynx::pub::LynxViewClient {
 public:
  void OnPageStart(const char* url) override {
    events.push_back("start");
    loaded_url = url;
  }
  void OnLoadSuccess() override { events.push_back("loaded"); }
  void OnFirstScreen() override { ++first_screen_count; }
  void OnReceivedError(int code, const char* message) override {
    errors.push_back(std::to_string(code) + ": " + message);
  }

  std::vector<std::string> events;
  std::vector<std::string> errors;
  std::string loaded_url;
  int first_screen_count = 0;
};

class LynxViewTest : public ::testing::Test {
 protected:
  void SetUp() override {
    lynx::base::UIThread::Init();
    auto& env = lynx::pub::LynxEnv::GetInstance();
    devtool_enabled_ = env.IsDevtoolEnabled();
    logbox_enabled_ = env.IsLogboxEnabled();
    env.SetDevtoolEnabled(false);
    env.SetLogboxEnabled(false);
  }

  void TearDown() override {
    view_.reset();
    lynx::fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
    auto& env = lynx::pub::LynxEnv::GetInstance();
    env.SetLogboxEnabled(logbox_enabled_);
    env.SetDevtoolEnabled(devtool_enabled_);
  }

  std::unique_ptr<lynx::pub::LynxView> view_;
  bool devtool_enabled_ = false;
  bool logbox_enabled_ = false;
};

TEST_F(LynxViewTest, LoadsBundleAndRendersFirstScreen) {
  // Encode with the current build's codec instead of storing a binary fixture.
  Json::Value input;
  auto& options = input["compilerOptions"];
  options["enableFiberArch"] = true;
  options["targetSdkVersion"] = "2.8";
  options["bundleModuleMode"] = "ReturnByFunction";
  options["useLepusNG"] = true;
  input["sourceContent"]["dsl"] = "tt";
  input["sourceContent"]["appType"] = "card";
  input["sourceContent"]["config"] = Json::Value(Json::objectValue);
  input["css"]["cssMap"]["0"] = Json::Value(Json::arrayValue);
  input["css"]["cssSource"]["0"] = "index.css";
  input["manifest"] = Json::Value(Json::objectValue);
  input["lepusCode"]["root"] = R"(
    function processData(data) { return data; }
    function renderPage() {
      const page = __CreatePage("0", 0);
      const child = __CreateView(0);
      __SetInlineStyles(child, "width: 100px; height: 40px; background-color: red;");
      __AppendElement(page, child);
      __FlushElementTree(page);
    }
  )";
  auto encoded = lynx::tasm::encode(Json::FastWriter().write(input));
  ASSERT_EQ(encoded.status, 0) << encoded.error_msg;
  ASSERT_FALSE(encoded.buffer.empty());
  auto bundle = std::make_shared<lynx::pub::LynxTemplateBundle>(
      encoded.buffer.data(), encoded.buffer.size());
  ASSERT_TRUE(bundle->IsValid()) << bundle->GetErrorMessage();

  lynx::pub::LynxView::Builder builder;
  // Exercise the main-thread rendering path without a background JS runtime.
  view_ = builder.SetScreenSize(320, 480, 1)
              .SetFrame(0, 0, 320, 480)
              .SetEnableJSRuntime(false)
              .Build();
  ASSERT_NE(view_, nullptr);
  ASSERT_NE(view_->Impl(), nullptr);
  auto client = std::make_shared<RecordingViewClient>();
  view_->AddClient(client);

  auto meta = std::make_shared<lynx::pub::LynxLoadMeta>();
  meta->SetUrl("test://lynx-view/basic");
  meta->SetTemplateBundle(bundle);
  view_->LoadTemplate(meta);
  lynx::fml::MessageLoop::GetCurrent().RunExpiredTasksNow();

  EXPECT_TRUE(client->errors.empty())
      << (client->errors.empty() ? "" : client->errors.front());
  EXPECT_EQ(client->events, (std::vector<std::string>{"start", "loaded"}));
  EXPECT_EQ(client->first_screen_count, 1);
  EXPECT_EQ(client->loaded_url, "test://lynx-view/basic");
  // Inspect the node tree and layout emitted by the real rendering engine.
  auto* renderer = static_cast<lynx::embedder::TestLynxUIRenderer*>(
      view_->Impl()->lynx_ui_renderer.get());
  auto painting = renderer->painting_state();
  ASSERT_EQ(painting->nodes.size(), 2u);
  int page_id = -1;
  int child_id = -1;
  for (const auto& entry : painting->nodes) {
    if (entry.second.tag == "page") page_id = entry.first;
    if (entry.second.tag == "view") child_id = entry.first;
  }
  ASSERT_NE(page_id, -1);
  ASSERT_NE(child_id, -1);
  EXPECT_FLOAT_EQ(painting->nodes.at(page_id).width, 320);
  EXPECT_FLOAT_EQ(painting->nodes.at(page_id).height, 480);
  EXPECT_EQ(painting->nodes.at(child_id).parent, page_id);
  EXPECT_FLOAT_EQ(painting->nodes.at(child_id).width, 100);
  EXPECT_FLOAT_EQ(painting->nodes.at(child_id).height, 40);
}

}  // namespace
