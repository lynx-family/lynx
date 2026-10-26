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
#include "core/renderer/js_bundle_holder_impl.h"
#include "core/renderer/template_entry_holder.h"
#include "core/resource/external_resource/external_resource_loader.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/common/bindings/resource/response_promise.h"
#include "core/runtime/js/js_bundle.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class ImmediateTaskRunner : public fml::TaskRunner {
 public:
  ImmediateTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

// Count engine dispatches without executing them on the calling JS thread.
class CountingEngineTaskRunner : public fml::TaskRunner {
 public:
  CountingEngineTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return false; }
  void PostTask(base::closure) override { ++post_task_count_; }

  int post_task_count_{0};
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

  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto lazy_bundle_loader =
      std::make_shared<tasm::LazyBundleLoader>(resource_loader);
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
  EXPECT_EQ(resource_loader->load_resource_count_, 0);
}

TEST(BTSRuntimeMediatorTest, FetchBundleRegisteredBundleSkipsEngineDispatch) {
  auto js_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto engine_runner = fml::MakeRefCounted<CountingEngineTaskRunner>();
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(
      std::make_unique<LynxEngine>(nullptr, nullptr, nullptr,
                                   base::LogContext{}, kUnknownInstanceId),
      engine_runner);
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto mediator = BTSRuntimeMediator(
      nullptr, engine_actor, nullptr, nullptr, js_runner,
      std::make_unique<ExternalResourceLoader>(resource_loader));
  mediator.runtime_standalone_mode_ = false;
  auto loader = std::make_shared<tasm::LazyBundleLoader>(resource_loader);
  mediator.SetLazyBundleLoader(loader);

  // Use the same registration path as LynxEngine::InsertLynxTemplateBundle.
  tasm::TemplateEntryHolder holder;
  holder.SetLazyBundleLoader(loader);
  constexpr const char kBundleUrl[] = "https://example.com/registered.bundle";
  holder.InsertLynxTemplateBundle(kBundleUrl, tasm::LynxTemplateBundle{});

  auto response =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
  mediator.FetchBundle(kBundleUrl, response);

  auto result = response->Wait(0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->url, kBundleUrl);
  EXPECT_EQ(result->code, tasm::LYNX_BUNDLE_RESOURCE_INFO_SUCCESS);
  EXPECT_EQ(engine_runner->post_task_count_, 0);
  EXPECT_EQ(resource_loader->load_resource_count_, 0);
}

TEST(BTSRuntimeMediatorTest, FetchBundleCacheMissDispatchesToEngine) {
  for (bool has_loader : {false, true}) {
    SCOPED_TRACE(has_loader);
    auto js_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
    auto engine_runner = fml::MakeRefCounted<CountingEngineTaskRunner>();
    // Keep the engine alive so a cache miss exercises dispatch instead of the
    // destroyed-engine error path. The counting runner never executes the task.
    auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(
        std::make_unique<LynxEngine>(nullptr, nullptr, nullptr,
                                     base::LogContext{}, kUnknownInstanceId),
        engine_runner);
    auto resource_loader = std::make_shared<CountingResourceLoader>();
    auto mediator = BTSRuntimeMediator(
        nullptr, engine_actor, nullptr, nullptr, js_runner,
        std::make_unique<ExternalResourceLoader>(resource_loader));
    mediator.runtime_standalone_mode_ = false;
    if (has_loader) {
      auto loader = std::make_shared<tasm::LazyBundleLoader>(resource_loader);
      loader->InsertTemplateBundle("https://example.com/other.bundle",
                                   tasm::LynxTemplateBundle{});
      mediator.SetLazyBundleLoader(loader);
    }

    auto response =
        std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
    mediator.FetchBundle("https://example.com/missing.bundle", response);

    EXPECT_FALSE(response->Wait(0).has_value());
    EXPECT_EQ(engine_runner->post_task_count_, 1);
    EXPECT_EQ(resource_loader->load_resource_count_, 0);
  }
}

TEST(BTSRuntimeMediatorTest, FetchBundleStandaloneCacheMissFetchesResource) {
  auto js_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto mediator = BTSRuntimeMediator(
      nullptr, nullptr, nullptr, nullptr, js_runner,
      std::make_unique<ExternalResourceLoader>(resource_loader));
  mediator.SetLazyBundleLoader(
      std::make_shared<tasm::LazyBundleLoader>(resource_loader));
  auto response =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
  constexpr const char kBundleUrl[] = "https://example.com/missing.bundle";

  mediator.FetchBundle(kBundleUrl, response);

  auto result = response->Wait(0);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->url, kBundleUrl);
  EXPECT_EQ(result->code, tasm::LYNX_BUNDLE_RESOURCE_INFO_REQUEST_FAILED);
  EXPECT_EQ(resource_loader->load_resource_count_, 1);
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
