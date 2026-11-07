// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/process_runtime.h"

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/devtool_state.h"
#include "core/shell/host_script/lynx_view/lynx_view_registry.h"
#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"
#include "core/shell/host_script/runtime/interceptor.h"
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

// Records the platform operations a script requested.
class CommandLog {
 public:
  void Add(std::string command) {
    std::lock_guard<std::mutex> lock(mutex_);
    commands_.push_back(std::move(command));
  }
  std::vector<std::string> Values() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return commands_;
  }

 private:
  mutable std::mutex mutex_;
  std::vector<std::string> commands_;
};

class FakeLynxView final : public LynxViewRefProxy {
 public:
  explicit FakeLynxView(std::shared_ptr<CommandLog> log)
      : log_(std::move(log)) {}
  bool LoadTemplate(LynxViewRefLoadTemplateRequest request) override {
    std::string command = "loadTemplate:" + request.url;
    if (request.has_template) command += "+bytes";
    if (!request.initial_data_json.empty())
      command += " " + request.initial_data_json;
    if (!request.processor_name.empty())
      command += " " + request.processor_name;
    if (request.read_only) command += " ro";
    return Add(std::move(command));
  }
  bool LoadSSR(LynxViewRefSsrRequest request) override {
    return Add("loadSSR:" + request.url);
  }
  bool HydrateSSR(LynxViewRefSsrRequest request) override {
    return Add("hydrateSSR:" + request.url);
  }
  bool UpdateMetaData(LynxViewRefUpdateMetaDataRequest request) override {
    return Add("updateMetaData:" + request.data_json);
  }
  bool ReloadTemplate(LynxViewRefReloadTemplateRequest request) override {
    return Add("reloadTemplate:" + request.data_json);
  }
  bool SendGlobalEvent(LynxViewRefGlobalEventRequest request) override {
    return Add("sendGlobalEvent:" + request.name + request.params_json);
  }
  void Invalidate() override {}

 private:
  bool Add(std::string command) {
    log_->Add(std::move(command));
    return true;
  }
  std::shared_ptr<CommandLog> log_;
};

lepus::Value LoadEvent(const std::string& view) {
  lepus::Value source(lepus::Dictionary::Create());
  source.SetProperty("kind", lepus::Value("url"));
  lepus::Value request(lepus::Dictionary::Create());
  request.SetProperty("url", lepus::Value("https://example.com/original"));
  request.SetProperty("source", source);
  lepus::Value event(lepus::Dictionary::Create());
  event.SetProperty("viewId", lepus::Value(view));
  event.SetProperty("origin", lepus::Value("host"));
  event.SetProperty("request", request);
  return event;
}

void OnRunner(const fml::RefPtr<fml::TaskRunner>& runner,
              std::function<void()> task) {
  auto done = std::make_shared<std::promise<void>>();
  auto future = done->get_future();
  runner->PostTask([task = std::move(task), done] {
    task();
    done->set_value();
  });
  EXPECT_EQ(future.wait_for(kTimeout), std::future_status::ready);
}

class ProcessRuntimeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // The platform UI thread that owns Views, separate from the UI domain.
    platform_ = std::make_unique<fml::Thread>("hsr_test_platform");
    LynxViewRegistry::GetInstance().SetUIRunner(platform_->GetTaskRunner());
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
    for (auto view : registered_)
      LynxViewRegistry::GetInstance().Unregister(view);
    LynxViewRegistry::GetInstance().SetUIRunner(nullptr);
    platform_->Join();
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

  void InitializeWithInterceptor() {
    auto ready = std::make_shared<Results>();
    ASSERT_TRUE(runtime_.Initialize(runners_, Collect(ready),
                                    &HostScriptInterceptorBindings::Create));
    ASSERT_TRUE(ready->WaitFor(3));
    for (const auto& result : ready->Values()) ASSERT_TRUE(result.success);
  }

  // The JSON value of a successful evaluation.
  std::string Value(Domain domain, const std::string& source) {
    auto result = Evaluate(domain, source)->Values().front();
    EXPECT_TRUE(result.success) << result.error;
    return result.value_json;
  }

  void RegisterView(base::LynxEntityId view, const std::string& url) {
    registered_.insert(view);
    OnRunner(platform_->GetTaskRunner(), [this, view, url] {
      EXPECT_TRUE(LynxViewRegistry::GetInstance().Register(
          view, std::make_shared<FakeLynxView>(commands_), url));
    });
  }

  // Waits for platform commands queued so far.
  void Flush() {
    OnRunner(platform_->GetTaskRunner(), [] {});
  }
  std::vector<std::string> Commands() const { return commands_->Values(); }

  Result Load(const std::string& source) {
    auto results = std::make_shared<Results>();
    runtime_.LoadScript(source, "host-script-entry.js", Collect(results));
    EXPECT_TRUE(results->WaitFor(1));
    return results->Values().front();
  }

  ProcessRuntime& runtime_ = ProcessRuntime::GetInstance();
  std::array<std::unique_ptr<fml::Thread>, 3> threads_;
  ProcessRuntime::Runners runners_;
  std::unique_ptr<fml::Thread> platform_;
  std::set<base::LynxEntityId> registered_;
  std::shared_ptr<CommandLog> commands_ = std::make_shared<CommandLog>();
};

TEST_F(ProcessRuntimeTest, ViewCreatedReplaysViewsOncePerSubscription) {
  RegisterView(1, "https://example.com/🌍");
  InitializeWithInterceptor();
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI}) {
    // Replay is asynchronous: nothing runs inside the registration call.
    EXPECT_EQ(Value(domain, R"(
      globalThis.seen = [];
      globalThis.subscription = interceptor.on('view.created', view =>
          seen.push([view.viewId, view.url, view.available]));
      seen.length)"),
              "0");
  }
  RegisterView(2, "");
  for (auto domain : {Domain::kBTS, Domain::kMTS, Domain::kUI}) {
    EXPECT_EQ(Value(domain, "seen"),
              "[[\"1\",\"https://example.com/🌍\",true],[\"2\",\"\",true]]");
  }

  // A live creation queued before the replay is not delivered twice.
  auto subscribed = std::make_shared<Results>();
  {
    RunnerBlock blocked(runners_.ui);
    runtime_.Evaluate(Domain::kUI, R"(
      subscription.dispose();
      seen = [];
      interceptor.on('view.created', view => seen.push(view.viewId));
    )",
                      "host-script-test.js", Collect(subscribed));
    RegisterView(3, "");
  }
  ASSERT_TRUE(subscribed->WaitFor(1));
  EXPECT_EQ(Value(Domain::kUI, "seen"), "[\"3\",\"1\",\"2\"]");
}

TEST_F(ProcessRuntimeTest,
       LynxViewProxiesAreCachedPerContextAndPassedToDecisions) {
  RegisterView(1, "https://example.com/a");
  InitializeWithInterceptor();
  const char* save = R"(
    globalThis.saved = new Map();
    interceptor.on('view.created', view => saved.set(view.viewId, view));
    0)";
  ASSERT_EQ(Value(Domain::kUI, save), "0");
  ASSERT_EQ(Value(Domain::kBTS, save), "0");
  ASSERT_EQ(Value(Domain::kUI, R"(
    globalThis.log = [];
    interceptor.on('view.loadTemplate', (event, chain, view) => {
      log.push(view === null ? null : view === saved.get(event.viewId));
      if (view) {
        log.push(view.updateMetaData({updateData: {from: 'decision'}}));
        Promise.resolve().then(() => view.loadURL('https://example.com/later'));
      }
      return chain.proceed({url: 'https://example.com/patched'});
    });
    0)"),
            "0");
  for (const char* view : {"1", "99"}) {
    OnRunner(runners_.ui, [view] {
      auto provider = Interceptor::Current({InterceptKind::kLoadTemplate},
                                           Interceptor::Domain::kUI);
      ASSERT_TRUE(provider);
      auto result =
          provider->Dispatch(InterceptKind::kLoadTemplate, LoadEvent(view));
      ASSERT_FALSE(result.failed);
      EXPECT_EQ(result.patch.GetProperty("url").StdString(),
                "https://example.com/patched");
    });
  }
  // Render-only Views have no operable proxy.
  EXPECT_EQ(Value(Domain::kUI, "log"), "[true,true,null]");
  // Each context has its own object for the same View identity.
  EXPECT_EQ(Value(Domain::kBTS, "[...saved.keys()]"), "[\"1\"]");
  EXPECT_EQ(Value(Domain::kBTS, "saved.get('1').url"),
            "\"https://example.com/a\"");
  Flush();
  EXPECT_EQ(Commands(), (std::vector<std::string>{
                            "updateMetaData:{\"from\":\"decision\"}",
                            "loadTemplate:https://example.com/later"}));
}

TEST_F(ProcessRuntimeTest, LynxViewProxyValidatesAndQueuesOperations) {
  RegisterView(4, "");
  InitializeWithInterceptor();
  ASSERT_EQ(Value(Domain::kBTS, R"(
    interceptor.on('view.created', view => { globalThis.view = view; });
    0)"),
            "0");
  EXPECT_EQ(Value(Domain::kBTS, R"(
    const bytes = new Uint8Array([1, 2]).buffer;
    [Object.isFrozen(view), view.viewId,
     view.loadTemplate({template: bytes, url: 'https://t', initialData: {a: 1},
                        processor: 'p', readOnly: true}),
     view.loadSSR({data: bytes, url: 'https://s'}),
     view.hydrateSSR({data: bytes, url: 'https://h'}),
     view.reloadTemplate(),
     view.sendGlobalEvent('evt', 1, 'two'),
     view.on('loadSuccess', () => {}) === view])"),
            "[true,\"4\",true,true,true,true,true,true]");
  EXPECT_EQ(Value(Domain::kBTS, R"(
    const codes = [];
    const cyclic = {};
    cyclic.self = cyclic;
    for (const call of [
      () => view.loadTemplate({url: 'https://t'}),
      () => view.updateMetaData({}),
      () => view.loadURL(''),
      () => view.loadURL('https://u', {data: 1}),
      () => view.sendGlobalEvent('evt', cyclic),
      () => view.on('ready', () => {}),
    ]) {
      try { call(); codes.push('ok'); } catch (e) { codes.push(e.code); }
    }
    codes)"),
            "[\"INVALID_ARGUMENT\",\"INVALID_ARGUMENT\",\"INVALID_ARGUMENT\","
            "\"INVALID_ARGUMENT\",\"INVALID_ARGUMENT\",\"INVALID_ARGUMENT\"]");
  Flush();
  EXPECT_EQ(Commands(), (std::vector<std::string>{
                            "loadTemplate:https://t+bytes {\"a\":1} p ro",
                            "loadSSR:https://s",
                            "hydrateSSR:https://h",
                            "reloadTemplate:{}",
                            "sendGlobalEvent:evt[1,\"two\"]",
                        }));
}

TEST_F(ProcessRuntimeTest, DestroyedViewInvalidatesProxiesAndQueuedCommands) {
  RegisterView(5, "");
  InitializeWithInterceptor();
  ASSERT_EQ(Value(Domain::kBTS, R"(
    globalThis.events = [];
    interceptor.on('view.created', view => {
      globalThis.view = view;
      view.on('loadSuccess', () => events.push('loadSuccess'))
          .on('destroyed', () => events.push('destroyed'));
    });
    0)"),
            "0");
  ASSERT_EQ(Value(Domain::kBTS, "view.viewId"), "\"5\"");
  OnRunner(platform_->GetTaskRunner(), [] {
    LynxViewRegistry::GetInstance().NotifyLifecycle(5, "loadSuccess");
  });
  {
    RunnerBlock blocked(platform_->GetTaskRunner());
    EXPECT_EQ(Value(Domain::kBTS, "view.loadURL('https://example.com/late')"),
              "true");
    // The queued command must not reach the destroyed View.
    LynxViewRegistry::GetInstance().Unregister(5);
    registered_.erase(5);
  }
  Flush();
  EXPECT_EQ(Value(Domain::kBTS, R"(
    let code;
    try { view.on('error', () => {}); } catch (e) { code = e.code; }
    [view.available, view.loadURL('https://example.com/after'), events, code,
     view.off('error', () => {}) === view])"),
            "[false,false,[\"loadSuccess\",\"destroyed\"],\"INVALID_STATE\","
            "true]");
  EXPECT_TRUE(Commands().empty());
}

TEST_F(ProcessRuntimeTest, ReplacedContextDropsCommandsAndReobtainsViews) {
  RegisterView(7, "");
  InitializeWithInterceptor();
  ASSERT_EQ(Value(Domain::kBTS, R"(
    globalThis.views = [];
    interceptor.on('view.created', view => views.push(view));
    0)"),
            "0");
  ASSERT_EQ(Value(Domain::kBTS, "views.length"), "1");
  {
    RunnerBlock blocked(platform_->GetTaskRunner());
    EXPECT_EQ(Value(Domain::kBTS, "views[0].sendGlobalEvent('old')"), "true");
    auto loaded = Load(R"(
      globalThis.views = [];
      interceptor.on('view.created', view => views.push(view));
    )");
    ASSERT_TRUE(loaded.success) << loaded.error;
  }
  Flush();
  EXPECT_TRUE(Commands().empty());
  EXPECT_EQ(Value(Domain::kBTS,
                  "[views.length, views[0].viewId, "
                  "views[0].sendGlobalEvent('new')]"),
            "[1,\"7\",true]");
  Flush();
  EXPECT_EQ(Commands(), (std::vector<std::string>{"sendGlobalEvent:new[]"}));
}

TEST_F(ProcessRuntimeTest, ClosingContextDropsOnlyItsQueuedViewCommands) {
  RegisterView(8, "");
  InitializeWithInterceptor();
  for (auto domain : {Domain::kBTS, Domain::kUI}) {
    ASSERT_EQ(Value(domain, R"(
      interceptor.on('view.created', view => { globalThis.view = view; });
      0)"),
              "0");
    ASSERT_EQ(Value(domain, "view.viewId"), "\"8\"");
  }
  auto blocked_platform =
      std::make_unique<RunnerBlock>(platform_->GetTaskRunner());
  EXPECT_EQ(Value(Domain::kBTS, "view.sendGlobalEvent('old')"), "true");
  EXPECT_EQ(Value(Domain::kUI, "view.sendGlobalEvent('ui')"), "true");
  auto loaded = std::make_shared<Results>();
  {
    RunnerBlock blocked_owner(runners_.bts);
    runtime_.LoadScript("0", "replacement.js", Collect(loaded));
    // Close has happened, but the blocked owner cannot reach Detach yet.
    blocked_platform.reset();
    Flush();
    EXPECT_EQ(Commands(), (std::vector<std::string>{"sendGlobalEvent:ui[]"}));
  }
  ASSERT_TRUE(loaded->WaitFor(1));
  EXPECT_TRUE(loaded->Values().front().success);
}

TEST_F(ProcessRuntimeTest, ClosingContextDropsQueuedViewLifecycleCallbacks) {
  RegisterView(9, "");
  InitializeWithInterceptor();
  ASSERT_EQ(Value(Domain::kBTS, R"(
    interceptor.on('view.created', view => {
      globalThis.view = view;
      view.on('loadSuccess', () => {
        interceptor.on('jsb.call', (event, chain) => chain.proceed());
      });
    });
    0)"),
            "0");
  ASSERT_EQ(Value(Domain::kBTS, "view.viewId"), "\"9\"");
  std::shared_ptr<Interceptor> provider;
  OnRunner(runners_.bts, [&provider] {
    provider = Interceptor::Current({InterceptKind::kCreated}, Domain::kBTS);
  });
  ASSERT_TRUE(provider);
  auto observed = std::make_shared<std::promise<bool>>();
  auto observed_future = observed->get_future();
  auto loaded = std::make_shared<Results>();
  {
    RunnerBlock blocked_owner(runners_.bts);
    OnRunner(platform_->GetTaskRunner(), [] {
      LynxViewRegistry::GetInstance().NotifyLifecycle(9, "loadSuccess");
    });
    // Inspect the provider directly: Current would hide it through its guard.
    runners_.bts->PostTask([provider, observed] {
      observed->set_value(provider->HasHandlers(InterceptKind::kCall));
    });
    runtime_.LoadScript("0", "replacement.js", Collect(loaded));
  }
  ASSERT_EQ(observed_future.wait_for(kTimeout), std::future_status::ready);
  EXPECT_FALSE(observed_future.get());
  ASSERT_TRUE(loaded->WaitFor(1));
  EXPECT_TRUE(loaded->Values().front().success);
}

// A host can talk to a loaded entry by evaluating in its context: it drains
// the entry's queued requests and delivers replies that resume its Promises.
TEST_F(ProcessRuntimeTest, EvaluateSharesLoadedEntryGlobalsForHostPolling) {
  InitializeWithInterceptor();
  auto loaded = Load(R"(
    const queue = [];
    const waiting = new Map();
    let next = 1;
    globalThis.__e2e = {
      request(method, args) {
        const id = next++;
        queue.push([id, method, JSON.stringify(args)]);
        return new Promise((resolve, reject) =>
            waiting.set(id, { resolve, reject }));
      },
      drain: () => queue.splice(0),
      deliver(id, json, error) {
        const entry = waiting.get(id);
        if (!entry) return false;
        waiting.delete(id);
        if (error) entry.reject(new Error(error));
        else entry.resolve(JSON.parse(json));
        return true;
      },
    };
    globalThis.log = [];
    (async () => {
      log.push((await __e2e.request('getConfig', [])).caseId);
      try {
        await __e2e.request('fail', ['x']);
      } catch (error) {
        log.push(error.message);
      }
    })();)");
  ASSERT_TRUE(loaded.success) << loaded.error;

  EXPECT_EQ(Value(Domain::kBTS, "__e2e.drain()"), R"([[1,"getConfig","[]"]])");
  EXPECT_EQ(Value(Domain::kBTS, "__e2e.drain()"), "[]");
  EXPECT_EQ(Value(Domain::kBTS, R"(__e2e.deliver(1, '{"caseId":"c1"}'))"),
            "true");
  // The resumed entry queued its next request before the following poll.
  EXPECT_EQ(Value(Domain::kBTS, "[log, __e2e.drain()]"),
            R"([["c1"],[[2,"fail","[\"x\"]"]]])");
  EXPECT_EQ(Value(Domain::kBTS, "__e2e.deliver(2, null, 'host failed')"),
            "true");
  EXPECT_EQ(Value(Domain::kBTS, "log"), R"(["c1","host failed"])");
  EXPECT_EQ(Value(Domain::kBTS, "__e2e.deliver(2, 'null')"), "false");

  // Unloading the entry also removes its channel.
  ASSERT_TRUE(Load("").success);
  EXPECT_EQ(Value(Domain::kBTS, "typeof __e2e"), R"("undefined")");
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
                fml::RefPtr<fml::TaskRunner>, ProcessRuntime::Guard) override {
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
