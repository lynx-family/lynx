// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/process_runtime.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/devtool_state.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace {

using Domain = ProcessRuntime::Domain;
using Result = ProcessRuntime::Result;
constexpr auto kTimeout = std::chrono::seconds(10);

class Results {
 public:
  void Add(Result result) {
    std::lock_guard<std::mutex> lock(mutex_);
    results_.push_back(std::move(result));
    changed_.notify_all();
  }

  bool WaitFor(size_t count) {
    std::unique_lock<std::mutex> lock(mutex_);
    return changed_.wait_for(lock, kTimeout,
                             [&] { return results_.size() >= count; });
  }

  std::vector<Result> Values() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return results_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<Result> results_;
};

ProcessRuntime::Completion Collect(const std::shared_ptr<Results>& results) {
  return [results](Result result) { results->Add(std::move(result)); };
}

class ProcessRuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(
        tasm::DevToolState::ENABLED);
    threads_[0] = std::make_unique<fml::Thread>("hsr_test_bts");
    threads_[1] = std::make_unique<fml::Thread>("hsr_test_mts");
    threads_[2] = std::make_unique<fml::Thread>("hsr_test_ui");
    runners_.bts = threads_[0]->GetTaskRunner();
    runners_.mts = threads_[1]->GetTaskRunner();
    runners_.ui = threads_[2]->GetTaskRunner();
  }

  void TearDown() override {
    auto stopped = std::make_shared<Results>();
    runtime_.Shutdown(Collect(stopped));
    EXPECT_TRUE(stopped->WaitFor(1));
    for (auto& thread : threads_) {
      thread->Join();
    }
    tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(
        tasm::DevToolState::UNAVAILABLE);
  }

  std::shared_ptr<Results> Initialize(const std::string& bootstrap) {
    auto ready = std::make_shared<Results>();
    EXPECT_TRUE(runtime_.Initialize(runners_, Collect(ready), {}, bootstrap));
    EXPECT_TRUE(ready->WaitFor(3));
    return ready;
  }

  std::shared_ptr<Results> Evaluate(Domain domain, const std::string& source) {
    auto results = std::make_shared<Results>();
    runtime_.Evaluate(domain, source, "host-script-test.js", Collect(results));
    EXPECT_TRUE(results->WaitFor(1));
    return results;
  }

  ProcessRuntime& runtime_ = ProcessRuntime::GetInstance();
  std::array<std::unique_ptr<fml::Thread>, 3> threads_;
  ProcessRuntime::Runners runners_;
};

TEST_F(ProcessRuntimeTest, BootstrapIgnoresSynchronousCompletionValue) {
  auto ready = Initialize("globalThis.helper = () => 42;");
  for (const auto& result : ready->Values()) {
    EXPECT_TRUE(result.success) << result.error;
    EXPECT_FALSE(result.has_value);
  }

  auto evaluated = Evaluate(Domain::kBTS, "helper()");
  ASSERT_EQ(evaluated->Values().size(), 1u);
  EXPECT_TRUE(evaluated->Values().front().success);
  EXPECT_EQ(evaluated->Values().front().value_json, "42");
}

TEST_F(ProcessRuntimeTest, BootstrapRejectsAsyncCompletionValue) {
  auto ready = Initialize("Promise.resolve(42)");
  for (const auto& result : ready->Values()) {
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "ASYNC_RESULT_UNSUPPORTED");
  }
  EXPECT_FALSE(runtime_.IsReady(Domain::kBTS));
  EXPECT_FALSE(runtime_.IsReady(Domain::kMTS));
  EXPECT_FALSE(runtime_.IsReady(Domain::kUI));
}

TEST_F(ProcessRuntimeTest, EvaluateStillRejectsAsyncCompletionValue) {
  auto ready = Initialize("globalThis.helper = () => 42;");
  for (const auto& result : ready->Values()) {
    ASSERT_TRUE(result.success) << result.error;
  }

  auto evaluated = Evaluate(Domain::kBTS, "Promise.resolve(42)");
  ASSERT_EQ(evaluated->Values().size(), 1u);
  EXPECT_FALSE(evaluated->Values().front().success);
  EXPECT_EQ(evaluated->Values().front().error, "ASYNC_RESULT_UNSUPPORTED");
}

}  // namespace
}  // namespace shell
}  // namespace lynx
