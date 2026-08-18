// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/bts/bts_runtime_mediator.h"

#include <string>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/runtime/common/bindings/resource/response_promise.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {

class ImmediateTaskRunner : public fml::TaskRunner {
 public:
  ImmediateTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
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
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto lazy_bundle_loader =
      std::make_shared<tasm::LazyBundleLoader>(resource_loader);
  auto mediator = BTSRuntimeMediator(nullptr, nullptr, nullptr, nullptr,
                                     task_runner, lazy_bundle_loader);

  tasm::LynxTemplateBundle template_bundle;
  constexpr const char kBundleUrl[] = "standalone://cached_bundle";
  lazy_bundle_loader->InsertTemplateBundle(kBundleUrl,
                                           std::move(template_bundle));
  auto response_promise =
      std::make_shared<runtime::ResponsePromise<tasm::BundleResourceInfo>>();
  mediator.FetchBundle(kBundleUrl, response_promise);
  auto result = response_promise->Wait(0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->url, kBundleUrl);
  EXPECT_EQ(result->code, tasm::LYNX_BUNDLE_RESOURCE_INFO_SUCCESS);
  EXPECT_EQ(resource_loader->load_resource_count_, 0);
}

TEST(BTSRuntimeMediatorTest,
     AttachSharesBundleManagerAndMergesExistingBundles) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto runtime_loader = std::make_shared<tasm::LazyBundleLoader>(nullptr);
  auto engine_loader = std::make_shared<tasm::LazyBundleLoader>(nullptr);
  constexpr const char kRuntimeBundleUrl[] = "standalone://runtime_bundle";
  constexpr const char kEngineBundleUrl[] = "standalone://engine_bundle";
  runtime_loader->InsertTemplateBundle(kRuntimeBundleUrl,
                                       tasm::LynxTemplateBundle{});
  engine_loader->InsertTemplateBundle(kEngineBundleUrl,
                                      tasm::LynxTemplateBundle{});
  auto mediator = BTSRuntimeMediator(nullptr, nullptr, nullptr, nullptr,
                                     task_runner, runtime_loader);
  auto facade_actor = std::make_shared<LynxActor<NativeFacade>>(
      std::unique_ptr<NativeFacade>{}, task_runner);
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(
      std::unique_ptr<LynxEngine>{}, task_runner);

  mediator.AttachToLynxShell(facade_actor, engine_actor,
                             std::make_shared<LynxCardCacheDataManager>(),
                             engine_loader);

  EXPECT_EQ(runtime_loader->GetBundleManager(),
            engine_loader->GetBundleManager());
  EXPECT_TRUE(runtime_loader->GetTemplateBundle(kRuntimeBundleUrl).has_value());
  EXPECT_TRUE(runtime_loader->GetTemplateBundle(kEngineBundleUrl).has_value());
  EXPECT_TRUE(engine_loader->GetTemplateBundle(kRuntimeBundleUrl).has_value());
  EXPECT_TRUE(engine_loader->GetTemplateBundle(kEngineBundleUrl).has_value());
}

TEST(BTSRuntimeMediatorTest, PreRegisteredTemplateBundleSkipsResourceFetch) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto lazy_bundle_loader =
      std::make_shared<tasm::LazyBundleLoader>(resource_loader);
  auto facade_actor = std::make_shared<LynxActor<NativeFacade>>(
      std::unique_ptr<NativeFacade>{}, task_runner);
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(
      std::unique_ptr<LynxEngine>{}, task_runner);
  auto mediator =
      BTSRuntimeMediator(facade_actor, engine_actor, nullptr,
                         std::make_shared<LynxCardCacheDataManager>(),
                         task_runner, lazy_bundle_loader);
  constexpr const char kBundleUrl[] = "https://example.com/lazy.bundle";

  lazy_bundle_loader->InsertTemplateBundle(kBundleUrl,
                                           tasm::LynxTemplateBundle{});
  mediator.LoadDynamicComponentFromJS(kBundleUrl, runtime::js::ApiCallBack(1),
                                      {"component-id"});

  EXPECT_EQ(resource_loader->load_resource_count_, 0);
}

TEST(BTSRuntimeMediatorTest, MissingTemplateBundleUsesResourceLoader) {
  auto task_runner = fml::MakeRefCounted<ImmediateTaskRunner>();
  auto resource_loader = std::make_shared<CountingResourceLoader>();
  auto lazy_bundle_loader =
      std::make_shared<tasm::LazyBundleLoader>(resource_loader);
  auto facade_actor = std::make_shared<LynxActor<NativeFacade>>(
      std::unique_ptr<NativeFacade>{}, task_runner);
  auto engine_actor = std::make_shared<LynxActor<LynxEngine>>(
      std::unique_ptr<LynxEngine>{}, task_runner);
  auto mediator =
      BTSRuntimeMediator(facade_actor, engine_actor, nullptr,
                         std::make_shared<LynxCardCacheDataManager>(),
                         task_runner, lazy_bundle_loader);
  constexpr const char kBundleUrl[] = "https://example.com/lazy.bundle";

  mediator.LoadDynamicComponentFromJS(kBundleUrl, runtime::js::ApiCallBack(1),
                                      {"component-id"});

  EXPECT_EQ(resource_loader->load_resource_count_, 1);
}

}  // namespace shell
}  // namespace lynx
