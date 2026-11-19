// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <string_view>

#include "base/include/fml/message_loop.h"
#include "base/include/value/array.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/embedder/lynx_view_priv.h"
#include "platform/embedder/public/capi/lynx_capi_module_capi.h"
#include "platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h"
#include "platform/embedder/public/capi/lynx_vsync_monitor_capi.h"
#include "platform/embedder/public/lynx_env.h"
#include "platform/embedder/public/lynx_group.h"
#include "platform/embedder/public/lynx_view.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#if ENABLE_NAPI_BINDING
#include "core/runtime/lepusng/quick_context.h"
#include "third_party/napi/include/napi_env.h"
#include "third_party/napi/include/napi_env_quickjs.h"
#include "third_party/napi/include/napi_runtime.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif
#endif

namespace lynx {
namespace embedder {
namespace {

struct ModuleProbe {
  int invocation_count = 0;
  int finalizer_count = 0;
};

void FetchTestCoreJs(lynx_generic_resource_fetcher_t*,
                     lynx_resource_request_t* request,
                     lynx_resource_response_t* response) {
  if (std::string_view(lynx_resource_request_get_url(request)) ==
      "assets://lynx_core.js") {
    uint8_t source[] = "void 0;";
    lynx_resource_response_set_code(response, 0);
    lynx_resource_response_set_data(response, source, sizeof(source) - 1,
                                    nullptr, nullptr);
  } else {
    lynx_resource_response_set_code(response, -1);
  }
  lynx_resource_response_callback(response);
  lynx_resource_response_release(response);
  lynx_resource_request_release(request);
}

class LynxViewCapiModuleTest : public testing::TestWithParam<const char*> {
 protected:
  void SetUp() override {
    base::UIThread::Init();
    auto& env = pub::LynxEnv::GetInstance();
    devtool_enabled_ = env.IsDevtoolEnabled();
    logbox_enabled_ = env.IsLogboxEnabled();
    env.SetDevtoolEnabled(false);
    env.SetLogboxEnabled(false);
  }

  void TearDown() override {
    view_.reset();
    if (js_runner_) {
      js_runner_->PostSyncTask([] {});
    }
    mts_manager_.reset();
#if ENABLE_NAPI_BINDING
    if (napi_env_) {
      napi_detach_runtime(napi_env_);
      napi_detach_quickjs(napi_env_);
      napi_free_env(napi_env_);
      napi_env_ = nullptr;
    }
    context_.reset();
#endif
    fml::MessageLoop::GetCurrent().RunExpiredTasksNow();
    auto& env = pub::LynxEnv::GetInstance();
    env.SetDevtoolEnabled(devtool_enabled_);
    env.SetLogboxEnabled(logbox_enabled_);
  }

  std::unique_ptr<pub::LynxView> view_;
  // Keep this manager alive until release, when the View detaches its most
  // recently created MTS factories.
  std::unique_ptr<pub::LynxNativeModuleManager> mts_manager_;
  fml::RefPtr<fml::TaskRunner> js_runner_;
  ModuleProbe probe_;
#if ENABLE_NAPI_BINDING
  std::unique_ptr<lepus::QuickContext> context_;
  napi_env napi_env_ = nullptr;
#endif

 private:
  bool devtool_enabled_ = false;
  bool logbox_enabled_ = false;
};

TEST_P(LynxViewCapiModuleTest, RegisteredModuleIsAttachedToBothRuntimes) {
  {
    // Keep the test core script off the shared JS thread's script cache.
    auto group = std::make_shared<pub::LynxGroup>("CapiModuleTest");
    group->SetEnableJSGroupThread(true);
    auto fetcher =
        std::unique_ptr<lynx_generic_resource_fetcher_t,
                        decltype(&lynx_generic_resource_fetcher_release)>(
            lynx_generic_resource_fetcher_create(nullptr),
            lynx_generic_resource_fetcher_release);
    lynx_generic_resource_fetcher_bind_fetch_resource(fetcher.get(),
                                                      FetchTestCoreJs);
    pub::LynxView::Builder builder;
    builder.SetScreenSize(320, 480, 1)
        .SetFrame(0, 0, 320, 480)
        .SetLynxGroup(group)
        .SetEnableJSRuntime(true);
    lynx_view_builder_set_generic_resource_fetcher(builder.Impl(),
                                                   fetcher.get());
    // The desktop test host has no platform VSync source for the JS runtime.
    lynx_view_builder_set_custom_vsync_monitor(
        builder.Impl(),
        lynx_vsync_monitor_create_with_finalizer(nullptr, nullptr));
    const lynx_capi_module_method_t method{"call", 0};
    ASSERT_EQ(lynx_view_builder_register_capi_module(
                  builder.Impl(), GetParam(), &method, 1,
                  [](void* opaque, const char*, const lynx_value*, size_t,
                     const lynx_capi_module_callback_t*, size_t,
                     lynx_value*) -> int32_t {
                    ++static_cast<ModuleProbe*>(opaque)->invocation_count;
                    return LYNX_CAPI_MODULE_SUCCESS;
                  },
                  &probe_,
                  [](void* opaque) {
                    ++static_cast<ModuleProbe*>(opaque)->finalizer_count;
                  }),
              LYNX_CAPI_MODULE_SUCCESS);
    view_ = builder.Build();
    ASSERT_NE(view_, nullptr);
  }
  EXPECT_EQ(probe_.finalizer_count, 0);

  const auto& renderer = view_->Impl()->lynx_template_renderer;
  js_runner_ =
      base::TaskRunnerManufactor::GetJSRunner(renderer->GetSettings().group_id);
  // Wait for the real BTS runtime to attach its NAPI factory before lookup.
  js_runner_->PostSyncTask([] {});
  const auto& create_mts_manager =
      renderer->GetSettings().native_module_manager_creator;
  ASSERT_TRUE(create_mts_manager);
  mts_manager_ = create_mts_manager();
  auto bts_manager = renderer->GetModuleManager();
  ASSERT_NE(bts_manager, nullptr);
#if ENABLE_NAPI_BINDING
  // Make NAPI factories eligible before lookup, including the built-in
  // LynxFetchModule that collides with one of the CAPI registrations.
  context_ = std::make_unique<lepus::QuickContext>(nullptr, false);
  napi_env_ = napi_new_env();
  auto config = napi_create_runtime_configuration();
  napi_attach_runtime_with_configuration(napi_env_, config);
  napi_delete_runtime_configuration(config);
  napi_attach_quickjs(napi_env_, context_->context());
  auto* env = reinterpret_cast<napi_env_weak>(napi_env_);
  mts_manager_->AttachOpaqueContext(env);
  auto fallback =
      view_->Impl()->mts_module_factory->CreateModule("LynxFetchModule");
  ASSERT_NE(fallback, nullptr);
  EXPECT_EQ(fallback->GetMethodList().count("fetch"), 1u);
  fallback.reset();
#endif
  auto mts_module = mts_manager_->GetModule(GetParam());
  auto bts_module = static_cast<pub::LynxNativeModuleManager&>(*bts_manager)
                        .GetModule(GetParam());
  ASSERT_NE(mts_module, nullptr);
  ASSERT_NE(bts_module, nullptr);
  ASSERT_EQ(mts_module->GetMethodList().count("call"), 1u);
  ASSERT_EQ(bts_module->GetMethodList().count("call"), 1u);

  auto arguments = [] {
    return std::make_unique<pub::ValueImplLepus>(
        lepus::Value(lepus::CArray::Create()));
  };
  EXPECT_TRUE(mts_module->InvokeMethod("call", arguments(), 0, {}).has_value());
  EXPECT_TRUE(bts_module->InvokeMethod("call", arguments(), 0, {}).has_value());
  EXPECT_EQ(probe_.invocation_count, 2);

  view_.reset();
  EXPECT_EQ(probe_.finalizer_count, 1);
  EXPECT_FALSE(
      mts_module->InvokeMethod("call", arguments(), 0, {}).has_value());
  EXPECT_FALSE(
      bts_module->InvokeMethod("call", arguments(), 0, {}).has_value());
  EXPECT_EQ(probe_.invocation_count, 2);
}

INSTANTIATE_TEST_SUITE_P(ModuleNames, LynxViewCapiModuleTest,
                         testing::Values("CapiProbe", "LynxFetchModule"));

}  // namespace
}  // namespace embedder
}  // namespace lynx

#if ENABLE_NAPI_BINDING && defined(USE_PRIMJS_NAPI)
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif
