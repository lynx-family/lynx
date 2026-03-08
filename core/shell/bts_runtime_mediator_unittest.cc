// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/shell/runtime/bts/bts_runtime_mediator.h"

#include "base/include/fml/task_runner.h"
#include "core/resource/external_resource/external_resource_loader.h"
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

}  // namespace shell
}  // namespace lynx
