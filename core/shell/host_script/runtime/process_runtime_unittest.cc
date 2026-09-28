// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/process_runtime.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
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

// Keep a lifecycle transition pending without sleeping or blocking the test's
// calling thread. Releasing in the destructor also makes failed assertions
// safe.
class RunnerBlock {
 public:
  explicit RunnerBlock(const fml::RefPtr<fml::TaskRunner>& runner)
      : release_(std::make_shared<std::promise<void>>()) {
    auto entered = std::make_shared<std::promise<void>>();
    auto entered_future = entered->get_future();
    runner->PostTask([entered, released = release_->get_future().share()] {
      entered->set_value();
      released.wait_for(kTimeout);
    });
    EXPECT_EQ(entered_future.wait_for(kTimeout), std::future_status::ready);
  }
  ~RunnerBlock() { Release(); }
  void Release() {
    if (release_) {
      release_->set_value();
      release_.reset();
    }
  }

 private:
  std::shared_ptr<std::promise<void>> release_;
};

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

  Result Load(const std::string& source) {
    auto results = std::make_shared<Results>();
    runtime_.LoadScript(source, "host-script-entry.js", Collect(results));
    EXPECT_TRUE(results->WaitFor(1));
    return results->Values().front();
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

TEST_F(ProcessRuntimeTest, LoadReplacesAllDomainsAndExecutesEntryOnlyOnBTS) {
  Initialize("globalThis.helper = () => 42;");
  std::array<uint64_t, 3> old_ids;
  for (size_t i = 0; i < old_ids.size(); ++i) {
    old_ids[i] = Evaluate(static_cast<Domain>(i), "globalThis.oldState = true")
                     ->Values()
                     .front()
                     .runtime_id;
  }

  auto loaded = Load("globalThis.entry = helper();");
  ASSERT_TRUE(loaded.success) << loaded.error;
  EXPECT_EQ(loaded.domain, "bts");
  EXPECT_FALSE(loaded.has_value);
  for (size_t i = 0; i < old_ids.size(); ++i) {
    auto result = Evaluate(static_cast<Domain>(i),
                           "[typeof oldState, typeof entry, helper()]")
                      ->Values()
                      .front();
    EXPECT_TRUE(result.success) << result.error;
    EXPECT_NE(result.runtime_id, old_ids[i]);
    EXPECT_EQ(result.value_json, i == 0 ? "[\"undefined\",\"number\",42]"
                                        : "[\"undefined\",\"undefined\",42]");
  }
}

TEST_F(ProcessRuntimeTest, LoadIgnoresCompletionValuesAndAllowsEmptySource) {
  Initialize("");
  for (const auto& source :
       {"globalThis.helper = () => 42", "Promise.resolve(42)",
        "({ get then() { throw new Error('do not read then'); } })", ""}) {
    auto result = Load(source);
    EXPECT_TRUE(result.success) << result.error;
    EXPECT_FALSE(result.has_value);
    EXPECT_TRUE(result.value_json.empty());
  }
}

TEST_F(ProcessRuntimeTest, LoadEntryCanScheduleWorkOnOtherDomains) {
  Initialize("");
  auto loaded = Load(
      "__hostScriptThread.runOnMainThread('globalThis.fromEntry = 1');"
      "__hostScriptThread.runOnUIThread('globalThis.fromEntry = 2');");
  ASSERT_TRUE(loaded.success) << loaded.error;
  EXPECT_EQ(Evaluate(Domain::kMTS, "fromEntry")->Values().front().value_json,
            "1");
  EXPECT_EQ(Evaluate(Domain::kUI, "fromEntry")->Values().front().value_json,
            "2");
}

TEST_F(ProcessRuntimeTest, InvalidLoadSourcePreservesCurrentGeneration) {
  Initialize("");
  const auto before = Evaluate(Domain::kBTS, "globalThis.keep = 42")
                          ->Values()
                          .front()
                          .runtime_id;
  for (const auto& source :
       {std::string(1, '\xff'), std::string(512 * 1024 + 1, ' ')}) {
    auto loaded = Load(source);
    EXPECT_FALSE(loaded.success);
    EXPECT_EQ(loaded.error, "INVALID_SCRIPT_SOURCE");
  }
  auto invalid_url = std::make_shared<Results>();
  runtime_.LoadScript("", "", Collect(invalid_url));
  ASSERT_TRUE(invalid_url->WaitFor(1));
  EXPECT_EQ(invalid_url->Values().front().error, "INVALID_SCRIPT_SOURCE");
  auto after = Evaluate(Domain::kBTS, "keep")->Values().front();
  EXPECT_EQ(after.runtime_id, before);
  EXPECT_EQ(after.value_json, "42");
}

TEST_F(ProcessRuntimeTest, FailedEntryCleansUpReplacement) {
  Initialize("");
  auto loaded =
      Load("globalThis.partial = true; throw new Error('entry failed')");
  EXPECT_FALSE(loaded.success);
  EXPECT_NE(loaded.error.find("entry failed"), std::string::npos);
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI}) {
    EXPECT_FALSE(runtime_.IsReady(domain));
    EXPECT_EQ(Evaluate(domain, "typeof partial")->Values().front().error,
              "RUNTIME_NOT_RUNNING");
  }
  EXPECT_TRUE(Load("globalThis.recovered = true").success);
  EXPECT_EQ(Evaluate(Domain::kBTS, "[typeof partial, recovered]")
                ->Values()
                .front()
                .value_json,
            "[\"undefined\",true]");
}

TEST_F(ProcessRuntimeTest, FailedBindingsCleanUpEveryReplacementDomain) {
  class Binding final : public ProcessRuntime::RuntimeBindings {
   public:
    explicit Binding(const std::shared_ptr<std::atomic<bool>>& fail)
        : fail_(fail) {}
    bool Attach(base::UnsafeWeakPtr<runtime::js::Runtime>,
                fml::RefPtr<fml::TaskRunner>) override {
      return !fail_->load();
    }

   private:
    std::shared_ptr<std::atomic<bool>> fail_;
  };
  auto fail = std::make_shared<std::atomic<bool>>(false);
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready), [fail](Domain) {
    return std::make_unique<Binding>(fail);
  }));
  ASSERT_TRUE(ready->WaitFor(3));
  fail->store(true);
  auto loaded = Load("42");
  EXPECT_FALSE(loaded.success);
  EXPECT_EQ(loaded.error, "RUNTIME_BINDINGS_ATTACH_FAILED");
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI})
    EXPECT_FALSE(runtime_.IsReady(domain));
}

TEST_F(ProcessRuntimeTest, ReplacementRejectsExternalWorkAndInitialization) {
  Initialize("");
  RunnerBlock blocked(runners_.bts);
  auto loaded = std::make_shared<Results>();
  runtime_.LoadScript("42", "entry.js", Collect(loaded));
  auto second = std::make_shared<Results>();
  runtime_.LoadScript("43", "entry.js", Collect(second));
  ASSERT_TRUE(second->WaitFor(1));
  EXPECT_EQ(second->Values().front().error, "RUNTIME_LOADING");
  EXPECT_EQ(Evaluate(Domain::kUI, "1")->Values().front().error,
            "RUNTIME_LOADING");
  EXPECT_FALSE(runtime_.Initialize(runners_, {}));
  runtime_.InitializeBindings();
  EXPECT_FALSE(runtime_.IsReady(Domain::kBTS));
  blocked.Release();
  ASSERT_TRUE(loaded->WaitFor(1));
  EXPECT_TRUE(loaded->Values().front().success);
  EXPECT_EQ(loaded->Values().size(), 1u);
}

TEST_F(ProcessRuntimeTest, ShutdownCancelsReplacementWithoutRestarting) {
  Initialize("");
  RunnerBlock blocked(runners_.bts);
  auto loaded = std::make_shared<Results>();
  runtime_.LoadScript("globalThis.shouldNotRun = true", "entry.js",
                      Collect(loaded));
  auto stopped = std::make_shared<Results>();
  runtime_.Shutdown(Collect(stopped));
  blocked.Release();
  ASSERT_TRUE(stopped->WaitFor(1));
  ASSERT_TRUE(loaded->WaitFor(1));
  EXPECT_EQ(loaded->Values().size(), 1u);
  EXPECT_EQ(loaded->Values().front().error, "RUNTIME_SHUTDOWN");
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI})
    EXPECT_FALSE(runtime_.IsReady(domain));
  Initialize("");
  EXPECT_EQ(Evaluate(Domain::kBTS, "typeof shouldNotRun")
                ->Values()
                .front()
                .value_json,
            "\"undefined\"");
}

TEST_F(ProcessRuntimeTest, DebugDisableDuringReplacementPreventsRestart) {
  Initialize("");
  RunnerBlock blocked(runners_.bts);
  auto loaded = std::make_shared<Results>();
  runtime_.LoadScript("42", "entry.js", Collect(loaded));
  tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(
      tasm::DevToolState::ATTACHED);
  blocked.Release();
  ASSERT_TRUE(loaded->WaitFor(1));
  EXPECT_FALSE(loaded->Values().front().success);
  EXPECT_EQ(loaded->Values().front().error, "HSR_DEBUG_DISABLED");
  tasm::DevToolLifecycle::GetInstance().SyncStateFromPlatform(
      tasm::DevToolState::ENABLED);
  EXPECT_EQ(Evaluate(Domain::kBTS, "1")->Values().front().error,
            "RUNTIME_NOT_RUNNING");
}

TEST_F(ProcessRuntimeTest,
       ShutdownWhileNewBindingsAttachCleansUpNewGeneration) {
  auto block = std::make_shared<std::atomic<bool>>(false);
  auto entered = std::make_shared<std::promise<void>>();
  auto entered_future = entered->get_future();
  std::promise<void> release;
  auto released = release.get_future().share();
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(
      runners_, Collect(ready), [block, entered, released](Domain domain) {
        if (domain == Domain::kBTS && block->load()) {
          entered->set_value();
          released.wait_for(kTimeout);
        }
        return std::unique_ptr<ProcessRuntime::RuntimeBindings>{};
      }));
  ASSERT_TRUE(ready->WaitFor(3));
  block->store(true);
  auto loaded = std::make_shared<Results>();
  runtime_.LoadScript("globalThis.partial = true", "entry.js", Collect(loaded));
  ASSERT_EQ(entered_future.wait_for(kTimeout), std::future_status::ready);
  auto stopped = std::make_shared<Results>();
  runtime_.Shutdown(Collect(stopped));
  release.set_value();
  ASSERT_TRUE(stopped->WaitFor(1));
  ASSERT_TRUE(loaded->WaitFor(1));
  EXPECT_EQ(loaded->Values().size(), 1u);
  EXPECT_EQ(loaded->Values().front().error, "RUNTIME_SHUTDOWN");
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI})
    EXPECT_FALSE(runtime_.IsReady(domain));
}

TEST_F(ProcessRuntimeTest, ExplicitShutdownDiscardsLoadConfiguration) {
  Initialize("");
  auto stopped = std::make_shared<Results>();
  runtime_.Shutdown(Collect(stopped));
  ASSERT_TRUE(stopped->WaitFor(1));
  EXPECT_EQ(Load("42").error, "RUNTIME_NOT_RUNNING");
  Initialize("");
  EXPECT_TRUE(Load("42").success);
}

}  // namespace
}  // namespace shell
}  // namespace lynx
