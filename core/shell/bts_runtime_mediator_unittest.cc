// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/shell/runtime/bts/bts_runtime_mediator.h"

#include <optional>
#include <string>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "base/include/value/array.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/js_bundle_holder_impl.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/common/bindings/event/runtime_constants.h"
#include "core/runtime/common/bindings/resource/response_promise.h"
#include "core/runtime/js/bindings/event/context_proxy_in_js.h"
#include "core/runtime/js/js_bundle.h"
#include "core/runtime/js/jsi/jsi_unittest.h"
#include "core/runtime/js/mock_template_delegate.h"
#include "core/shell/runtime/bts/bts_runtime.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class ImmediateTaskRunner : public fml::TaskRunner {
 public:
  ImmediateTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

class PausedDispatchTaskRunner : public fml::TaskRunner {
 public:
  PausedDispatchTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return false; }
  void PostTask(base::closure task) override {
    tasks.emplace_back(std::move(task));
  }
  std::vector<base::closure> tasks;
};

TEST(BTSRuntimeMediatorTest, DispatchLoggingPreservesQueuedTasks) {
  const auto original_level =
      base::logging::ParseLogLevel(base::logging::GetMinLogLevelName());
  for (int level :
       {base::logging::LOG_INFO, base::logging::detail::LOG_OBSERVE}) {
    auto runner = fml::MakeRefCounted<PausedDispatchTaskRunner>();
    auto actor = std::make_shared<LynxActor<BTSRuntime>>(
        std::unique_ptr<BTSRuntime>(), runner);
    auto args = lepus::CArray::Create();
    args->emplace_back("component");
    args->emplace_back("handler");
    args->emplace_back(42);
    auto event = fml::MakeRefCounted<runtime::MessageEvent>(
        runtime::kMessageEventTypePublishComponentEvent,
        runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext,
        std::make_unique<pub::ValueImplLepus>(lepus::Value(std::move(args))));
    event->EnableDispatchDiagnostics();
    base::logging::SetMinLogLevel(level);
    ::testing::internal::CaptureStdout();
    BTSRuntimeMediator::DispatchMessageEventToRuntime(actor, std::move(event));
    const auto output = ::testing::internal::GetCapturedStdout();
    base::logging::SetMinLogLevel(original_level);
    EXPECT_EQ(runner->tasks.size(), 1u);
    EXPECT_EQ(output.find("stage=enqueue type=") != std::string::npos,
              level == base::logging::detail::LOG_OBSERVE);
    if (level == base::logging::detail::LOG_OBSERVE) {
      EXPECT_NE(output.find("target=component handler=handler"),
                std::string::npos);
    }
  }
}

TEST(BTSRuntimeMediatorTest, CachedDispatchReplaysAcrossLogLevelChanges) {
  const auto original_level =
      base::logging::ParseLogLevel(base::logging::GetMinLogLevelName());
  for (int level :
       {base::logging::LOG_INFO, base::logging::detail::LOG_OBSERVE}) {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    base::UnsafeOwningPtr<runtime::js::Runtime> js_runtime(
        runtime::js::test::MakeRuntimeFactory<runtime::js::QuickjsRuntime>(
            nullptr)
            .release());
    BTSRuntime bts_runtime(
        "", {}, 1, std::make_unique<runtime::test::MockTemplateDelegate>(), "",
        0, {});
    bts_runtime.app_ = runtime::js::App::Create(
        1, js_runtime.GetWeakPtr(), bts_runtime.GetDelegate(), nullptr,
        runtime::js::Object(*js_runtime), nullptr, "", {});
    auto event = fml::MakeRefCounted<runtime::MessageEvent>(
        "cached_dispatch", runtime::ContextProxy::Type::kCoreContext,
        runtime::ContextProxy::Type::kJSContext,
        std::make_unique<pub::ValueImplLepus>(lepus::Value(42)));
    event->EnableDispatchDiagnostics();
    int calls = 0;
    bts_runtime.app_
        ->GetOrCreateContextProxyImpl(runtime::ContextProxy::Type::kCoreContext)
        ->AddEventListener("cached_dispatch",
                           std::make_unique<event::ClosureEventListener>(
                               [&calls](lepus::Value args) {
                                 ++calls;
                                 EXPECT_EQ(args.Int32(), 42);
                               }));
    base::logging::SetMinLogLevel(level);
    ::testing::internal::CaptureStdout();
    bts_runtime.OnReceiveMessageEvent(std::move(event));
    const auto output = ::testing::internal::GetCapturedStdout();
    base::logging::SetMinLogLevel(original_level);
    EXPECT_EQ(output.find("stage=cached state=") != std::string::npos,
              level == base::logging::detail::LOG_OBSERVE);
    EXPECT_EQ(calls, 0);
    EXPECT_EQ(bts_runtime.cached_tasks_.size(), 1u);
    base::logging::SetMinLogLevel(level == base::logging::LOG_INFO
                                      ? base::logging::detail::LOG_OBSERVE
                                      : base::logging::LOG_INFO);
    bts_runtime.state_ = BTSRuntime::State::kRuntimeReady;
    bts_runtime.OnRuntimeReady();
    base::logging::SetMinLogLevel(original_level);
    EXPECT_EQ(calls, 1);
    EXPECT_TRUE(bts_runtime.cached_tasks_.empty());
    bts_runtime.app_->Destroy();
    bts_runtime.app_ = nullptr;
    bts_runtime.state_ = BTSRuntime::State::kNotStarted;
  }
}

class MockLazyBundleLoaderForMediatorTest : public tasm::LazyBundleLoader {
 public:
  MockLazyBundleLoaderForMediatorTest() = default;
  ~MockLazyBundleLoaderForMediatorTest() override = default;

  using tasm::LazyBundleLoader::FetchBundle;
  void FetchBundle(tasm::lazy_bundle::LynxLazyBundleRequest request) {
    ++fetch_bundle_call_count_;
    if (request.response_promise) {
      request.response_promise->SetValue(
          {.url = request.url,
           .code = tasm::LYNX_BUNDLE_RESOURCE_INFO_REQUEST_FAILED});
    }
  }

  int fetch_bundle_call_count_{0};
};

class EmptyBundleProxy : public tasm::JsBundleHolderImpl::BundleProxy {
 public:
  std::optional<tasm::LynxTemplateBundle> FindTemplateBundle(
      const std::string&) override {
    return std::nullopt;
  }
};

class CountingResourceLoader : public pub::LynxResourceLoader {
 public:
  int load_resource_count_{0};

 protected:
  void LoadResourceInternal(
      const pub::LynxResourceRequest&,
      base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback)
      override {
    ++load_resource_count_;
    pub::LynxResourceResponse response;
    response.err_code = -1;
    response.err_msg = "test resource miss";
    callback(response);
  }
};

TEST(BTSRuntimeMediatorTest,
     FetchBundleStandaloneShouldReturnEarlyWhenTemplateBundleExists) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto external_resource_loader = std::make_unique<ExternalResourceLoader>();
  auto mediator =
      BTSRuntimeMediator(nullptr, nullptr, nullptr, nullptr, task_runner,
                         std::move(external_resource_loader));

  auto lazy_bundle_loader =
      std::make_shared<MockLazyBundleLoaderForMediatorTest>();
  tasm::LynxTemplateBundle template_bundle;
  constexpr const char kBundleUrl[] = "standalone://cached_bundle";
  lazy_bundle_loader->InsertTemplateBundle(kBundleUrl,
                                           std::move(template_bundle));
  mediator.SetLazyBundleLoader(lazy_bundle_loader);

  auto response_promise =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
  mediator.FetchBundle(kBundleUrl, response_promise);
  auto result = response_promise->Wait(0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->url, kBundleUrl);
  EXPECT_EQ(result->code, tasm::LYNX_BUNDLE_RESOURCE_INFO_SUCCESS);
  EXPECT_EQ(lazy_bundle_loader->fetch_bundle_call_count_, 0);
}

TEST(BTSRuntimeMediatorTest,
     FetchBundleShouldFailImmediatelyWhenEngineIsDestroyed) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(
      std::unique_ptr<LynxEngine>(), task_runner);
  auto external_resource_loader = std::make_unique<ExternalResourceLoader>();
  auto mediator =
      BTSRuntimeMediator(nullptr, engine_actor, nullptr, nullptr, task_runner,
                         std::move(external_resource_loader));
  mediator.runtime_standalone_mode_ = false;

  constexpr const char kBundleUrl[] = "https://example.com/lazy.bundle";
  auto response_promise =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
  mediator.FetchBundle(kBundleUrl, response_promise);
  auto result = response_promise->Wait(0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->url, kBundleUrl);
  EXPECT_EQ(result->code, tasm::LYNX_BUNDLE_RESOURCE_INFO_REQUEST_FAILED);
  EXPECT_EQ(result->error_msg,
            "fetchBundle failed: Lynx engine has been destroyed");
}

TEST(JsBundleHolderImplTest, TemplateBundleCacheIsClearedWhenJSBundleInserted) {
  EmptyBundleProxy proxy;
  tasm::JsBundleHolderImpl holder(proxy);
  constexpr const char kBundleUrl[] = "https://example.com/lazy.bundle";

  tasm::LynxTemplateBundle template_bundle;
  holder.InsertTemplateBundle(kBundleUrl, template_bundle);
  EXPECT_TRUE(holder.GetTemplateBundleFromBT(kBundleUrl).has_value());

  runtime::js::JsBundle js_bundle;
  js_bundle.AddJsContent(
      "component.js",
      runtime::js::JsContent("component source",
                             runtime::js::JsContent::Type::SOURCE));
  holder.InsertJSBundle(kBundleUrl, js_bundle);

  EXPECT_FALSE(holder.GetTemplateBundleFromBT(kBundleUrl).has_value());

  holder.SetEnable(true);
  auto ready_bundle = holder.GetJSBundleFromBT(kBundleUrl);
  ASSERT_TRUE(ready_bundle.has_value());
  EXPECT_TRUE(ready_bundle->GetJsContent("component.js").has_value());
}

TEST(BTSRuntimeMediatorTest,
     PreRegisteredTemplateBundleSkipsExternalResourceFetch) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto external_resource_loader =
      std::make_unique<ExternalResourceLoader>(resource_loader);
  auto mediator =
      BTSRuntimeMediator(nullptr, nullptr, nullptr, nullptr, task_runner,
                         std::move(external_resource_loader));
  mediator.runtime_standalone_mode_ = false;
  constexpr const char kBundleUrl[] = "https://example.com/lazy.bundle";

  std::optional<tasm::LynxTemplateBundle> template_bundle{
      tasm::LynxTemplateBundle{}};
  mediator.LoadDynamicComponentFromJS(kBundleUrl, runtime::js::ApiCallBack(1),
                                      {"component-id"},
                                      std::move(template_bundle));

  EXPECT_EQ(resource_loader->load_resource_count_, 0);
}

TEST(BTSRuntimeMediatorTest, MissingTemplateBundleUsesExternalResourceFetch) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto external_resource_loader =
      std::make_unique<ExternalResourceLoader>(resource_loader);
  auto mediator =
      BTSRuntimeMediator(nullptr, nullptr, nullptr, nullptr, task_runner,
                         std::move(external_resource_loader));
  mediator.runtime_standalone_mode_ = false;
  constexpr const char kBundleUrl[] = "https://example.com/lazy.bundle";

  mediator.LoadDynamicComponentFromJS(kBundleUrl, runtime::js::ApiCallBack(1),
                                      {"component-id"}, std::nullopt);

  EXPECT_EQ(resource_loader->load_resource_count_, 1);
}

}  // namespace shell
}  // namespace lynx
