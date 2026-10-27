// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/external_resource/external_resource_loader.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/runtime/js/mock_template_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::shell {
namespace {
class ResourceErrorTaskRunner : public fml::TaskRunner {
 public:
  ResourceErrorTaskRunner() : fml::TaskRunner(nullptr) {}
  bool RunsTasksOnCurrentThread() override { return true; }
  void PostTask(base::closure task) override { task(); }
};

class ResourceErrorDelegate : public runtime::test::MockTemplateDelegate {
 public:
  void OnErrorOccurred(base::LynxError error) override {
    errors.push_back(std::move(error));
  }
  std::vector<base::LynxError> errors;
};

class TestResourceLoader : public pub::LynxResourceLoader {
 public:
  bool fail = true;
  bool defer_response = false;
  base::MoveOnlyClosure<void, pub::LynxResourceResponse&> pending_callback;

  void LoadBytecode(const pub::LynxResourceRequest& request,
                    base::MoveOnlyClosure<void, pub::LynxResourceResponse&>
                        callback) override {
    LoadResourceInternal(request, std::move(callback));
  }

 protected:
  void LoadResourceInternal(
      const pub::LynxResourceRequest&,
      base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback)
      override {
    if (defer_response) {
      pending_callback = std::move(callback);
      return;
    }
    pub::LynxResourceResponse response;
    if (fail) {
      response.err_code = -1;
      response.err_msg = "resource unavailable";
    } else {
      response.data = {'j', 's'};
    }
    callback(response);
  }
};

class ExternalResourceLoaderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    auto delegate = std::make_unique<ResourceErrorDelegate>();
    errors_ = delegate.get();
    runtime_ = std::make_shared<LynxActor<BTSRuntime>>(
        std::make_unique<BTSRuntime>("resource-test", base::LogContext{}, 1,
                                     std::move(delegate), "", 0,
                                     tasm::PageOptions{}),
        fml::MakeRefCounted<ResourceErrorTaskRunner>());
    resources_ = std::make_shared<TestResourceLoader>();
    loader_ = std::make_shared<ExternalResourceLoader>(resources_);
    loader_->SetRuntimeActor(runtime_);
  }

  void ExpectError(int code, const std::string& url) {
    ASSERT_EQ(errors_->errors.size(), 1u);
    const auto& error = errors_->errors.front();
    EXPECT_EQ(error.error_code_, code);
    EXPECT_EQ(error.custom_info_.at("src"), url);
    EXPECT_EQ(error.root_cause_, "resource unavailable");
    EXPECT_EQ(error.custom_info_.count("root_cause"), 0u);
    EXPECT_EQ(error.custom_info_.count("lynx_context_root_cause"), 0u);
  }

  ResourceErrorDelegate* errors_;
  std::shared_ptr<TestResourceLoader> resources_;
  std::shared_ptr<ExternalResourceLoader> loader_;
  std::shared_ptr<LynxActor<BTSRuntime>> runtime_;
};

TEST_F(ExternalResourceLoaderTest, LocalFailureReportsToRequestRuntime) {
  EXPECT_TRUE(loader_->LoadJSSource("assets://missing.js").empty());
  ExpectError(error::E_RESOURCE_EXTERNAL_RESOURCE_LOCAL_RESOURCE_LOAD_FAIL,
              "assets://missing.js");
}

TEST_F(ExternalResourceLoaderTest,
       ScriptFailurePreservesResponseAndReportsOnce) {
  auto response = loader_->LoadScript("https://example.com/missing.js", 1);
  EXPECT_FALSE(response.Success());
  EXPECT_EQ(response.err_msg, "resource unavailable");
  ExpectError(error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
              "https://example.com/missing.js");
}

TEST_F(ExternalResourceLoaderTest, LateScriptFailureStillReportsAfterTimeout) {
  resources_->defer_response = true;
  const std::string url = "https://example.com/slow.js";
  auto result = loader_->LoadScript(url, 1);
  EXPECT_FALSE(result.Success());
  EXPECT_EQ(result.err_msg, "timeout");
  EXPECT_TRUE(errors_->errors.empty());

  pub::LynxResourceResponse response{.err_code = -1,
                                     .err_msg = "resource unavailable"};
  resources_->pending_callback(response);
  ExpectError(error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED, url);
}

TEST_F(ExternalResourceLoaderTest,
       LateScriptFailureAfterLoaderDestroyedDoesNotReport) {
  resources_->defer_response = true;
  auto result = loader_->LoadScript("https://example.com/slow.js", 1);
  ASSERT_EQ(result.err_msg, "timeout");

  std::weak_ptr<ExternalResourceLoader> weak_loader = loader_;
  loader_.reset();
  ASSERT_TRUE(weak_loader.expired());

  pub::LynxResourceResponse response{.err_code = -1,
                                     .err_msg = "resource unavailable"};
  resources_->pending_callback(response);
  EXPECT_TRUE(errors_->errors.empty());
}

TEST_F(ExternalResourceLoaderTest, AsyncScriptFailureReportsOnce) {
  loader_->LoadScriptAsync("https://example.com/missing.js", 1);
  ExpectError(error::E_RESOURCE_EXTERNAL_RESOURCE_REQUEST_FAILED,
              "https://example.com/missing.js");
}

TEST_F(ExternalResourceLoaderTest, BytecodeFailureRemainsSilentForFallback) {
  EXPECT_FALSE(loader_->LoadByteCode("bytecode", 1).Success());
  EXPECT_TRUE(errors_->errors.empty());
}

TEST_F(ExternalResourceLoaderTest,
       SharedPlatformLoaderKeepsErrorOwnersSeparate) {
  auto other_delegate = std::make_unique<ResourceErrorDelegate>();
  auto* other_errors = other_delegate.get();
  auto other_runtime = std::make_shared<LynxActor<BTSRuntime>>(
      std::make_unique<BTSRuntime>("resource-test", base::LogContext{}, 2,
                                   std::move(other_delegate), "", 0,
                                   tasm::PageOptions{}),
      fml::MakeRefCounted<ResourceErrorTaskRunner>());
  auto other_loader = std::make_shared<ExternalResourceLoader>(resources_);
  other_loader->SetRuntimeActor(other_runtime);
  loader_->LoadJSSource("assets://missing.js");
  ExpectError(error::E_RESOURCE_EXTERNAL_RESOURCE_LOCAL_RESOURCE_LOAD_FAIL,
              "assets://missing.js");
  EXPECT_TRUE(other_errors->errors.empty());

  resources_->fail = false;
  EXPECT_EQ(other_loader->LoadJSSource("assets://present.js"),
            (std::vector<uint8_t>{'j', 's'}));
  EXPECT_TRUE(other_errors->errors.empty());
}
}  // namespace
}  // namespace lynx::shell
