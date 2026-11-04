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
#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"
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

class Events {
 public:
  struct Entry {
    ProcessRuntime::Event event;
    ProcessRuntime::Guard current;
  };
  void Add(ProcessRuntime::Event event, ProcessRuntime::Guard current) {
    std::lock_guard<std::mutex> lock(mutex_);
    entries_.push_back({std::move(event), std::move(current)});
    changed_.notify_all();
  }
  bool WaitFor(size_t count) {
    std::unique_lock<std::mutex> lock(mutex_);
    return changed_.wait_for(lock, kTimeout,
                             [&] { return entries_.size() >= count; });
  }
  std::vector<Entry> Values() {
    std::lock_guard<std::mutex> lock(mutex_);
    return entries_;
  }

 private:
  std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<Entry> entries_;
};

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
    runtime_.SetEventSink({});
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

  std::shared_ptr<Events> CollectEvents() {
    auto events = std::make_shared<Events>();
    runtime_.SetEventSink([events](auto event, auto current) {
      events->Add(std::move(event), std::move(current));
    });
    return events;
  }

  ProcessRuntime& runtime_ = ProcessRuntime::GetInstance();
  std::array<std::unique_ptr<fml::Thread>, 3> threads_;
  ProcessRuntime::Runners runners_;
};

TEST_F(ProcessRuntimeTest, EmitEventCopiesJSONAndIdentifiesEveryOwner) {
  Initialize("");
  auto events = CollectEvents();
  size_t count = 0;
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI}) {
    auto result = Evaluate(domain, R"(
      const payload = {type: 'sample', message: 'hello 🌍'};
      JSON.stringify = () => 'replaced';
      __hostScriptThread.emitEvent(payload);
      payload.message = 'changed after emission';
      void 0;
    )")
                      ->Values()
                      .front();
    ASSERT_TRUE(result.success) << result.error;
    ASSERT_TRUE(events->WaitFor(++count));
    const auto entry = events->Values().back();
    EXPECT_EQ(entry.event.thread, HostScriptDomainName(domain));
    EXPECT_EQ(entry.event.runtime_id, result.runtime_id);
    EXPECT_EQ(entry.event.payload_json,
              R"({"type":"sample","message":"hello 🌍"})");
    EXPECT_TRUE(entry.current());
  }
}

TEST_F(ProcessRuntimeTest, EmitEventRejectsInvalidAndOversizedPayloads) {
  Initialize("");
  auto events = CollectEvents();
  for (const char* source :
       {"__hostScriptThread.emitEvent()", "__hostScriptThread.emitEvent(null)",
        "__hostScriptThread.emitEvent('text')",
        "__hostScriptThread.emitEvent([])",
        "__hostScriptThread.emitEvent(() => {})",
        "const cycle = {}; cycle.self = cycle; "
        "__hostScriptThread.emitEvent(cycle)",
        "__hostScriptThread.emitEvent({message: 'x'.repeat(65536)})",
        "__hostScriptThread.emitEvent({toJSON() { throw Error('failed'); }})",
        "__hostScriptThread.emitEvent({toJSON() { return undefined; }})"}) {
    auto result = Evaluate(Domain::kBTS, source)->Values().front();
    EXPECT_FALSE(result.success) << source;
    EXPECT_FALSE(result.error.empty()) << source;
  }
  EXPECT_TRUE(events->Values().empty());
}

TEST_F(ProcessRuntimeTest, ViewErrorPublishesAfterLoadScriptCompletes) {
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready),
                                  &HostScriptInterceptorBindings::Create));
  ASSERT_TRUE(ready->WaitFor(3));
  auto events = CollectEvents();
  auto loaded = Load(R"(
    globalThis.errorSubscription = interceptor.on('view.error', event => {
      __hostScriptThread.emitEvent({type: 'lynx.error', level: 'error',
        viewId: event.viewId, code: event.code, message: event.message});
    });
  )");
  ASSERT_TRUE(loaded.success) << loaded.error;
  EXPECT_TRUE(events->Values().empty());
  runtime_.NotifyViewError({"42", 1001, "later error 🌍"});
  ASSERT_TRUE(events->WaitFor(1));
  const auto event = events->Values().front().event;
  EXPECT_EQ(event.thread, "bts");
  EXPECT_EQ(event.runtime_id, loaded.runtime_id);
  EXPECT_EQ(
      event.payload_json,
      R"({"type":"lynx.error","level":"error","viewId":"42","code":1001,"message":"later error 🌍"})");
  ASSERT_TRUE(Evaluate(Domain::kBTS, "errorSubscription.dispose()")
                  ->Values()
                  .front()
                  .success);
  runtime_.NotifyViewError({"42", 1002, "after unsubscribe"});
  ASSERT_TRUE(Evaluate(Domain::kBTS, "void 0")->Values().front().success);
  EXPECT_EQ(events->Values().size(), 1u);
}

TEST_F(ProcessRuntimeTest, ViewErrorObserversReceiveIndependentCopies) {
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready),
                                  &HostScriptInterceptorBindings::Create, R"(
    interceptor.on('view.error', event => {
      __hostScriptThread.emitEvent(event);
      event.message = 'changed';
      throw Error('observer failure');
    });
  )"));
  ASSERT_TRUE(ready->WaitFor(3));
  auto events = CollectEvents();
  runtime_.NotifyViewError({"7", 5, "original", "warn"});
  ASSERT_TRUE(events->WaitFor(3));
  for (const auto& entry : events->Values()) {
    // Object key order is owned by the N-API dictionary conversion.
    EXPECT_NE(entry.event.payload_json.find("\"message\":\"original\""),
              std::string::npos);
    EXPECT_NE(entry.event.payload_json.find("\"level\":\"warn\""),
              std::string::npos);
  }
}

TEST_F(ProcessRuntimeTest, ReplacementInvalidatesEventDeliveryAndObserver) {
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready),
                                  &HostScriptInterceptorBindings::Create));
  ASSERT_TRUE(ready->WaitFor(3));
  auto events = CollectEvents();
  ASSERT_TRUE(Load(R"(
    interceptor.on('view.error', event => __hostScriptThread.emitEvent(event));
  )")
                  .success);
  runtime_.NotifyViewError({"1", 1, "old"});
  ASSERT_TRUE(events->WaitFor(1));
  auto previous = events->Values().front();
  EXPECT_TRUE(previous.current());
  auto loaded = Load("");
  ASSERT_TRUE(loaded.success);
  EXPECT_FALSE(previous.current());
  runtime_.NotifyViewError({"1", 2, "must not reach old observer"});
  ASSERT_TRUE(Evaluate(Domain::kBTS, "void 0")->Values().front().success);
  EXPECT_EQ(events->Values().size(), 1u);
  ASSERT_TRUE(
      Evaluate(Domain::kBTS, "__hostScriptThread.emitEvent({message:'new'})")
          ->Values()
          .front()
          .success);
  ASSERT_TRUE(events->WaitFor(2));
  EXPECT_EQ(events->Values().back().event.runtime_id, loaded.runtime_id);
  EXPECT_TRUE(events->Values().back().current());
  auto stopped = std::make_shared<Results>();
  runtime_.Shutdown(Collect(stopped));
  ASSERT_TRUE(stopped->WaitFor(1));
  EXPECT_FALSE(events->Values().back().current());
}

TEST_F(ProcessRuntimeTest, QueuedViewErrorIsDroppedWhenGenerationCloses) {
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready),
                                  &HostScriptInterceptorBindings::Create));
  ASSERT_TRUE(ready->WaitFor(3));
  auto events = CollectEvents();
  ASSERT_TRUE(Load(R"(
    interceptor.on('view.error', event => __hostScriptThread.emitEvent(event));
  )")
                  .success);
  RunnerBlock blocked(runners_.bts);
  runtime_.NotifyViewError({"1", 1, "queued"});
  auto loaded = std::make_shared<Results>();
  runtime_.LoadScript("", "replacement.js", Collect(loaded));
  blocked.Release();
  ASSERT_TRUE(loaded->WaitFor(1));
  ASSERT_TRUE(loaded->Values().front().success);
  EXPECT_TRUE(events->Values().empty());
}

TEST_F(ProcessRuntimeTest, ViewCreatedBroadcastsIndependentSnapshotsToOwners) {
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready),
                                  &HostScriptInterceptorBindings::Create, R"(
        globalThis.created = [];
        interceptor.on('view.created', event => {
          created.push([event.viewId, event.url, __hostScriptThread.thread]);
          event.url = 'changed in this domain';
          if (__hostScriptThread.thread === 'bts') throw Error('observer failed');
        });
      )"));
  ASSERT_TRUE(ready->WaitFor(3));
  for (const auto& result : ready->Values()) ASSERT_TRUE(result.success);
  runtime_.NotifyViewCreated({"1", "https://example.com/🌍"});
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI}) {
    auto result = Evaluate(domain, "created")->Values().front();
    ASSERT_TRUE(result.success) << result.error;
    EXPECT_EQ(result.value_json,
              std::string("[[\"1\",\"https://example.com/🌍\",\"") +
                  HostScriptDomainName(domain) + "\"]]");
  }
}

TEST_F(ProcessRuntimeTest,
       ViewCreatedDoesNotInitializeOrReplayToUnusedDomains) {
  ASSERT_TRUE(runtime_.Initialize(runners_, {},
                                  &HostScriptInterceptorBindings::Create, "",
                                  ProcessRuntime::InitializationMode::kLazy));
  runtime_.NotifyViewCreated({"before", ""});
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI})
    EXPECT_FALSE(runtime_.IsReady(domain));
  const char* listen = R"(
    globalThis.created = [];
    interceptor.on('view.created', event => created.push(event.viewId));
  )";
  ASSERT_TRUE(Evaluate(Domain::kUI, listen)->Values().front().success);
  runtime_.NotifyViewCreated({"1", ""});
  EXPECT_EQ(Evaluate(Domain::kUI, "created")->Values().front().value_json,
            "[\"1\"]");
  EXPECT_FALSE(runtime_.IsReady(Domain::kBTS));
  EXPECT_FALSE(runtime_.IsReady(Domain::kMTS));
  ASSERT_TRUE(Evaluate(Domain::kBTS, listen)->Values().front().success);
  EXPECT_EQ(Evaluate(Domain::kBTS, "created")->Values().front().value_json,
            "[]");
  runtime_.NotifyViewCreated({"2", ""});
  EXPECT_EQ(Evaluate(Domain::kUI, "created")->Values().front().value_json,
            "[\"1\",\"2\"]");
  EXPECT_EQ(Evaluate(Domain::kBTS, "created")->Values().front().value_json,
            "[\"2\"]");
  EXPECT_FALSE(runtime_.IsReady(Domain::kMTS));
}

TEST_F(ProcessRuntimeTest, ViewCreatedRunsInlineOnOwnerAndDropsAfterShutdown) {
  class Binding final : public ProcessRuntime::RuntimeBindings {
   public:
    explicit Binding(std::shared_ptr<std::atomic<int>> calls)
        : calls_(std::move(calls)) {}
    bool Attach(base::UnsafeWeakPtr<runtime::js::Runtime>,
                fml::RefPtr<fml::TaskRunner> runner) override {
      runner_ = std::move(runner);
      return true;
    }
    void NotifyViewCreated(const ProcessRuntime::ViewCreated&) override {
      EXPECT_TRUE(runner_->RunsTasksOnCurrentThread());
      ++*calls_;
    }

   private:
    std::shared_ptr<std::atomic<int>> calls_;
    fml::RefPtr<fml::TaskRunner> runner_;
  };
  auto calls = std::make_shared<std::atomic<int>>(0);
  auto ready = std::make_shared<Results>();
  ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready), [calls](Domain) {
    return std::make_unique<Binding>(calls);
  }));
  ASSERT_TRUE(ready->WaitFor(3));
  RunnerBlock bts(runners_.bts);
  RunnerBlock mts(runners_.mts);
  auto done = std::make_shared<std::promise<void>>();
  auto future = done->get_future();
  runners_.ui->PostTask([this, calls, done] {
    runtime_.NotifyViewCreated({"1", ""});
    EXPECT_EQ(calls->load(), 1);
    done->set_value();
  });
  ASSERT_EQ(future.wait_for(kTimeout), std::future_status::ready);
  auto stopped = std::make_shared<Results>();
  runtime_.Shutdown(Collect(stopped));
  bts.Release();
  mts.Release();
  ASSERT_TRUE(stopped->WaitFor(1));
  runtime_.NotifyViewCreated({"2", ""});
  EXPECT_EQ(calls->load(), 1);
}

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
