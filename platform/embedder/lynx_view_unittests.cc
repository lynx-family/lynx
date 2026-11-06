// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/message_loop.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/services/performance/performance_controller.h"
#include "core/template_bundle/template_codec/binary_encoder/encoder.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/lynx_env.h"
#include "platform/embedder/public/lynx_view.h"
#include "platform/embedder/testing/lynx_ui_renderer_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace {

class InspectableTemplateRenderer
    : public lynx::embedder::LynxTemplateRenderer {
 public:
  using LynxTemplateRenderer::LynxTemplateRenderer;
  lynx::shell::LynxShell* shell() { return shell_.get(); }
};

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

TEST_F(LynxViewTest, SendGlobalEventReachesMTSWithAndWithoutRuntimeProxy) {
  class RecordingRuntimeProxy : public lynx::shell::LynxBTSRuntimeProxyImpl {
   public:
    RecordingRuntimeProxy() : LynxBTSRuntimeProxyImpl(nullptr) {}
    void CallJSFunction(std::string module_id, std::string method_id,
                        std::unique_ptr<lynx::pub::Value> params) override {
      EXPECT_EQ(module_id, "GlobalEventEmitter");
      EXPECT_EQ(method_id, "emit");
      arguments = lynx::pub::ValueUtils::ConvertValueToLepusValue(*params);
      ++call_count;
    }
    lynx::lepus::Value arguments;
    int call_count = 0;
  };
  class TestTemplateRenderer : public lynx::embedder::LynxTemplateRenderer {
   public:
    using LynxTemplateRenderer::LynxTemplateRenderer;
    auto GetEngineActor() { return shell_->GetEngineActor(); }
    void SetRuntimeProxy(
        const std::shared_ptr<RecordingRuntimeProxy>& runtime_proxy) {
      runtime_proxy_ = runtime_proxy;
    }
  };

  for (bool enable_runtime_proxy : {false, true}) {
    SCOPED_TRACE(enable_runtime_proxy);
    lynx::pub::LynxView::Builder builder;
    lynx::embedder::TestLynxUIRenderer ui_renderer(builder.Impl());
    lynx::embedder::LynxTemplateRenderer::Settings settings;
    settings.screen_size = {320, 480};
    settings.viewport_size = {320, 480};
    settings.enable_js = false;
    TestTemplateRenderer renderer(settings, ui_renderer.GetUIDelegate(),
                                  nullptr, nullptr);
    renderer.Initialize();
    auto runtime_proxy = std::make_shared<RecordingRuntimeProxy>();
    if (enable_runtime_proxy) {
      renderer.SetRuntimeProxy(runtime_proxy);
    }
    auto engine_actor = renderer.GetEngineActor();
    int event_count = 0;
    lynx::lepus::Value received_args;
    engine_actor->ActSync([&](auto& engine) {
      auto* proxy = engine->GetTasm()->GetContextProxy(
          lynx::runtime::ContextProxy::Type::kNative);
      proxy->AddEventListener(
          "view-event", std::make_shared<lynx::event::ClosureEventListener>(
                            [&](lynx::lepus::Value args) {
                              ++event_count;
                              received_args =
                                  lynx::lepus::Value::ShallowCopy(args);
                            }));
    });
    auto params = lynx::lepus::CArray::Create();
    params->emplace_back("payload");
    params->emplace_back(42);
    renderer.SendGlobalEvent("view-event",
                             lynx::lepus::Value(std::move(params)));
    engine_actor->ActSync([](auto&) {});
    EXPECT_EQ(event_count, 1);
    EXPECT_EQ(received_args.GetProperty(0), lynx::lepus::Value("view-event"));
    auto received_params = received_args.GetProperty(1);
    ASSERT_TRUE(received_params.IsArray());
    ASSERT_EQ(received_params.GetLength(), 2U);
    EXPECT_EQ(received_params.GetProperty(0), lynx::lepus::Value("payload"));
    EXPECT_EQ(received_params.GetProperty(1), lynx::lepus::Value(42));
    EXPECT_EQ(runtime_proxy->call_count, enable_runtime_proxy ? 1 : 0);
    if (enable_runtime_proxy) {
      EXPECT_EQ(runtime_proxy->arguments, received_args);
    }
  }
}

TEST_F(LynxViewTest, ResetRecreatesPlatformPerformanceController) {
  lynx::pub::LynxView::Builder builder;
  lynx::embedder::TestLynxUIRenderer ui_renderer(builder.Impl());
  InspectableTemplateRenderer::Settings settings;
  settings.screen_size = {320, 480};
  settings.viewport_size = {320, 480};
  settings.enable_js = false;
  InspectableTemplateRenderer renderer(settings, ui_renderer.GetUIDelegate(),
                                       nullptr, nullptr);
  renderer.Initialize();
  ASSERT_NE(
      renderer.shell()->GetPerfControllerActor()->Impl()->GetPlatformImpl(),
      nullptr);

  renderer.Reset();
  EXPECT_NE(
      renderer.shell()->GetPerfControllerActor()->Impl()->GetPlatformImpl(),
      nullptr);
}

TEST_F(LynxViewTest, DisabledJSInitializesMTSWithoutBTS) {
  lynx::pub::LynxView::Builder builder;
  lynx::embedder::TestLynxUIRenderer ui_renderer(builder.Impl());
  InspectableTemplateRenderer::Settings settings;
  settings.screen_size = {320, 480};
  settings.viewport_size = {320, 480};
  settings.enable_js = false;
  int native_manager_count = 0;
  settings.native_module_manager_creator = [&]() {
    ++native_manager_count;
    return std::make_unique<lynx::pub::LynxNativeModuleManager>();
  };
  InspectableTemplateRenderer renderer(settings, ui_renderer.GetUIDelegate(),
                                       nullptr, nullptr);

  for (int reset_count = 1; reset_count <= 2; ++reset_count) {
    if (reset_count == 1) {
      renderer.Initialize();
    } else {
      renderer.Reset();
    }
    EXPECT_EQ(native_manager_count, reset_count);
    EXPECT_EQ(renderer.GetModuleManager(), nullptr);
    EXPECT_EQ(renderer.GetRuntimeProxy(), nullptr);
  }
}

TEST_F(LynxViewTest, CustomVSyncSurvivesBuilderReplacementAndViewSharing) {
  struct State {
    int requests = 0;
    int finalized = 0;
  } first, replacement;
  auto create = [](State* state) {
    auto* monitor = lynx_vsync_monitor_create_with_finalizer(
        state,
        [](auto*, void* data) { ++static_cast<State*>(data)->finalized; });
    lynx_vsync_monitor_bind_request_vsync_func(
        monitor, [](auto* monitor, auto callback, void* context) {
          auto* state =
              static_cast<State*>(lynx_vsync_monitor_get_user_data(monitor));
          ++state->requests;
          callback(context, 10, 20);
        });
    return monitor;
  };
  using View = std::unique_ptr<lynx_view_t, decltype(&lynx_view_release)>;
  View view(nullptr, lynx_view_release);
  View second_view(nullptr, lynx_view_release);
  View replacement_view(nullptr, lynx_view_release);
  {
    lynx::pub::LynxView::Builder builder;
    builder.SetEnableJSRuntime(false);
    lynx_view_builder_set_custom_vsync_monitor(builder.Impl(), create(&first));
    // Use the regular test renderer without configuring a windowless renderer.
    view.reset(lynx_view_create(builder.Impl(), nullptr));
    ASSERT_NE(view, nullptr);
    second_view.reset(lynx_view_create(builder.Impl(), nullptr));
    ASSERT_NE(second_view, nullptr);
    lynx_view_builder_set_custom_vsync_monitor(builder.Impl(),
                                               create(&replacement));
    replacement_view.reset(lynx_view_create(builder.Impl(), nullptr));
    ASSERT_NE(replacement_view, nullptr);
    lynx_view_builder_set_custom_vsync_monitor(builder.Impl(), nullptr);
  }
  EXPECT_EQ(first.finalized, 0);
  EXPECT_EQ(replacement.finalized, 0);
  int signals = 0;
  {
    auto source =
        view->lynx_template_renderer->GetSettings().vsync_monitor_platform_impl;
    auto second_source = second_view->lynx_template_renderer->GetSettings()
                             .vsync_monitor_platform_impl;
    auto replacement_source =
        replacement_view->lynx_template_renderer->GetSettings()
            .vsync_monitor_platform_impl;
    ASSERT_NE(source, nullptr);
    ASSERT_EQ(source, second_source);
    ASSERT_NE(source, replacement_source);
    ASSERT_NE(replacement_source, nullptr);
    for (const auto& monitor : {source, second_source, replacement_source}) {
      monitor->RequestVSync([&](int64_t start, int64_t target) {
        EXPECT_EQ(start, 10);
        EXPECT_EQ(target, 20);
        ++signals;
      });
    }
  }
  EXPECT_EQ(first.requests, 2);
  EXPECT_EQ(replacement.requests, 1);
  EXPECT_EQ(signals, 3);
  view.reset();
  lynx::fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(first.finalized, 0);
  second_view.reset();
  lynx::fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(first.finalized, 1);
  EXPECT_EQ(replacement.finalized, 0);
  replacement_view.reset();
  lynx::fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
  EXPECT_EQ(replacement.finalized, 1);
}

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
