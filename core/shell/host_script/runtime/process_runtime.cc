// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/process_runtime.h"

#include <array>
#include <atomic>
#include <mutex>
#include <optional>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/lynx_actor.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/runtime/js/js_executor.h"
#include "core/shell/host_script/runtime/host_script_thread_bindings.h"

namespace lynx {
namespace shell {

bool IsHostScriptRuntimeEnabled() {
#if defined(OS_ANDROID)
  // Only the Android debug engine links the real process runtime.
  return true;
#else
  return tasm::DevToolLifecycle::GetInstance().IsEnabled();
#endif
}

namespace {

using Domain = ProcessRuntime::Domain;
using Result = ProcessRuntime::Result;
using Completion = ProcessRuntime::Completion;
using Guard = ProcessRuntime::Guard;
using BindingFactory = ProcessRuntime::BindingFactory;
using InitializationMode = ProcessRuntime::InitializationMode;
enum class ProcessState { kRunning, kStopping, kStopped };
enum class CompletionValue { kJSON, kSynchronous, kIgnored };

std::string CurrentThread() {
  std::ostringstream stream;
  stream << std::this_thread::get_id();
  return stream.str();
}

using Owners = std::array<fml::RefPtr<fml::TaskRunner>, kHostScriptDomainCount>;

bool IsValidDomain(Domain domain) {
  return HostScriptDomainIndex(domain) < kHostScriptDomainCount;
}

// Input-byte normalization only; JavaScript syntax belongs to the engine.
bool NormalizeScriptSource(std::string& source, bool allow_empty = false) {
  constexpr size_t kMaxSourceBytes = 512 * 1024;
  if ((!allow_empty && source.empty()) || source.size() > kMaxSourceBytes ||
      !base::IsValidUtf8(reinterpret_cast<const uint8_t*>(source.data()),
                         source.size()))
    return false;
  if (source.compare(0, 3, "\xEF\xBB\xBF") == 0) source.erase(0, 3);
  return allow_empty || !source.empty();
}

Result WithError(Result result, std::string error) {
  result.success = false;
  result.has_value = false;
  result.value_json.clear();
  result.error = std::move(error);
  result.executing_thread = CurrentThread();
  return result;
}

// Owns one existing JSExecutor and its JSI handles on the selected runner.
class DomainRuntime final {
 public:
  DomainRuntime(Result identity, fml::RefPtr<fml::TaskRunner> runner,
                std::unique_ptr<ProcessRuntime::RuntimeBindings> bindings,
                std::shared_ptr<std::atomic<ProcessState>> lifetime)
      : identity_(std::move(identity)),
        runner_(std::move(runner)),
        bindings_(std::move(bindings)),
        lifetime_(std::move(lifetime)) {}

  ~DomainRuntime() {
    DCHECK(runner_->RunsTasksOnCurrentThread());
    if (thread_bindings_) {
      thread_bindings_->Detach();
      thread_bindings_.reset();
    }
    bindings_.reset();
    stringify_.reset();
    if (executor_) executor_->Destroy();
  }

  Result Initialize() {
    DCHECK(runner_->RunsTasksOnCurrentThread());
    if (!IsHostScriptRuntimeEnabled())
      return WithError(Identity(), "HSR_DEBUG_DISABLED");
    identity_.owner_thread = CurrentThread();
    executor_ =
        std::make_unique<runtime::js::JSExecutor>("-1", nullptr, nullptr, true);
    runtime::js::JSRuntimeExternalParams params;
    // These contexts have no page and must not update page instrumentation.
    params.runtime_id = tasm::PageOptions::kUnknownInstanceID;
    params.group_id = "-1";
    executor_->loadPreJSBundle(
        [] {
          return std::vector<
              std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>{};
        },
        true, std::move(params), tasm::PageOptions{});
    auto* runtime = executor_->GetJSRuntime().Lock();
    if (!runtime) return WithError(Identity(), "RUNTIME_ALLOCATION_FAILED");
    runtime->SetEnableJsBindingApiThrowException(true);
    if (bindings_ && !bindings_->Attach(executor_->GetJSRuntime(), runner_))
      return WithError(Identity(), "RUNTIME_BINDINGS_ATTACH_FAILED");
    identity_.context_id =
        reinterpret_cast<uintptr_t>(runtime->getSharedContext().get());
    runtime::js::Scope scope(*runtime);
    runtime::js::JSINativeExceptionCollector::Scope exceptions;
    auto json = runtime->global().getPropertyAsObject(*runtime, "JSON");
    if (!json) return FailedCall();
    stringify_ = json->getPropertyAsFunction(*runtime, "stringify");
    if (!stringify_) return FailedCall();
    thread_bindings_ = HostScriptThreadBindings::Create(
        *runtime, runner_,
        [lifetime = lifetime_](Domain domain, std::string source,
                               std::string url, Completion completion,
                               Guard guard) {
          ProcessRuntime::GetInstance().RunOnThread(
              domain, std::move(source), std::move(url), std::move(completion),
              [lifetime, guard = std::move(guard)] {
                return lifetime->load(std::memory_order_acquire) ==
                           ProcessState::kRunning &&
                       (!guard || guard());
              });
        });
    if (!thread_bindings_ || !thread_bindings_->Install(identity_.domain))
      return WithError(Identity(), "HOST_SCRIPT_THREAD_BINDINGS_ATTACH_FAILED");
    auto result = Identity();
    result.success = true;
    return result;
  }

  Result Identity() const {
    Result result = identity_;
    result.executing_thread = CurrentThread();
    return result;
  }

  void NotifyViewCreated(const ProcessRuntime::ViewCreated& event) {
    DCHECK(runner_->RunsTasksOnCurrentThread());
    if (bindings_) bindings_->NotifyViewCreated(event);
  }

  Result Execute(const std::string& source, const std::string& url,
                 CompletionValue completion_value = CompletionValue::kJSON) {
    DCHECK(runner_->RunsTasksOnCurrentThread());
    // Recheck owner admission on platforms with a revocable lifecycle.
    if (!IsHostScriptRuntimeEnabled())
      return WithError(Identity(), "HSR_DEBUG_DISABLED");
    if (executing_) return WithError(Identity(), "REENTRANT_EVALUATION");
    executing_ = true;
    struct Reset {
      bool& flag;
      ~Reset() { flag = false; }
    } reset{executing_};
    auto& runtime = *executor_->GetJSRuntime().Lock();
    runtime::js::Scope scope(runtime);
    runtime::js::JSINativeExceptionCollector::Scope exceptions;
    auto value = runtime.evaluateJavaScript(
        std::make_shared<runtime::js::StringBuffer>(source), url);
    if (!value.has_value())
      return WithError(Identity(), value.error().message());
    Result result = Identity();
    if (completion_value != CompletionValue::kIgnored && value->isObject()) {
      auto then = value->getObject(runtime).getProperty(runtime, "then");
      if (!then) return FailedCall();
      if (then->isObject() && then->getObject(runtime).isFunction(runtime))
        return WithError(Identity(), "ASYNC_RESULT_UNSUPPORTED");
    }
    if (completion_value == CompletionValue::kJSON && !value->isUndefined()) {
      auto encoded = stringify_->call(
          runtime, static_cast<const runtime::js::Value*>(&*value), size_t{1});
      if (!encoded) return FailedCall();
      if (!encoded->isString())
        return WithError(Identity(), "RESULT_NOT_JSON_SERIALIZABLE");
      result.has_value = true;
      result.value_json = encoded->getString(runtime).utf8(runtime);
    }
    result.success = true;
    return result;
  }

 private:
  Result FailedCall() const {
    const auto& error =
        runtime::js::JSINativeExceptionCollector::Instance()->GetException();
    return WithError(Identity(),
                     error ? error->message() : "JAVASCRIPT_CALL_FAILED");
  }

  Result identity_;
  const fml::RefPtr<fml::TaskRunner> runner_;
  std::unique_ptr<runtime::js::JSExecutor> executor_;
  std::shared_ptr<HostScriptThreadBindings> thread_bindings_;
  std::unique_ptr<ProcessRuntime::RuntimeBindings> bindings_;
  const std::shared_ptr<std::atomic<ProcessState>> lifetime_;
  std::optional<runtime::js::Function> stringify_;
  bool executing_ = false;
};

// Only immutable identity and an atomic readiness snapshot cross runners.
struct DomainPublication {
  DomainPublication(Domain domain, uint64_t runtime_id,
                    const fml::RefPtr<fml::TaskRunner>& runner) {
    identity.domain = HostScriptDomainName(domain);
    identity.runtime_id = runtime_id;
    identity.owner_runner_id = static_cast<size_t>(runner->GetTaskQueueId());
  }
  Result Snapshot() const {
    auto initialized = std::atomic_load(&initialized_identity);
    return initialized ? *initialized : identity;
  }
  Result identity;
  std::shared_ptr<const Result> initialized_identity;
  std::atomic<bool> ready{false};
};

class DomainState final {
 public:
  using Actor = LynxActor<DomainState>;

  DomainState(fml::RefPtr<fml::TaskRunner> runner,
              std::shared_ptr<DomainPublication> publication,
              std::shared_ptr<std::atomic<ProcessState>> process_state,
              Completion ready, Domain domain, BindingFactory binding_factory,
              std::string bootstrap)
      : runner_(std::move(runner)),
        publication_(std::move(publication)),
        process_state_(std::move(process_state)),
        identity_(publication_->identity),
        ready_callback_(std::move(ready)),
        domain_(domain),
        binding_factory_(std::move(binding_factory)),
        bootstrap_(std::move(bootstrap)) {}

  Result Initialize() {
    AssertOwner();
    if (initialization_result_) return *initialization_result_;
    identity_.owner_thread = CurrentThread();
    Result result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    if (IsRunning()) {
      runtime_ = std::make_unique<DomainRuntime>(
          identity_, runner_,
          binding_factory_ ? binding_factory_(domain_) : nullptr,
          process_state_);
      result = runtime_->Initialize();
      if (result.success && !bootstrap_.empty()) {
        result = runtime_->Execute(bootstrap_, "host-script-bootstrap.js",
                                   CompletionValue::kSynchronous);
      }
      bootstrap_.clear();
      identity_ = runtime_->Identity();
      if (!IsRunning()) result = WithError(Identity(), "RUNTIME_SHUTDOWN");
      if (!result.success) runtime_.reset();
    }
    binding_factory_ = {};
    std::atomic_store(&publication_->initialized_identity,
                      std::make_shared<const Result>(identity_));
    publication_->ready.store(result.success, std::memory_order_release);
    initialization_result_ = result;
    LOGI("Host Script initialized: domain="
         << result.domain << " runtime=" << result.runtime_id << " owner="
         << result.owner_thread << " executing=" << result.executing_thread
         << " success=" << result.success);
    auto ready = std::move(ready_callback_);
    if (ready) ready(result);
    return result;
  }

  void Evaluate(const std::string& source, const std::string& url,
                Completion completion, const Guard& guard,
                CompletionValue completion_value = CompletionValue::kJSON) {
    AssertOwner();
    Result result;
    if (!IsRunning()) {
      result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    } else if (guard && !guard()) {
      result = WithError(Identity(), "SOURCE_RUNTIME_DETACHED");
    } else if (!IsRunning()) {
      // A guard is allowed to request shutdown.
      result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    } else {
      result = Initialize();
      // Bootstrap or the readiness callback may have detached the source.
      if (result.success && guard && !guard()) {
        result = WithError(Identity(), "SOURCE_RUNTIME_DETACHED");
      }
      if (result.success && IsRunning()) {
        result = runtime_->Execute(source, url, completion_value);
      }
      if (!IsRunning()) result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    }
    LOGI("Host Script execution: domain="
         << result.domain << " runtime=" << result.runtime_id << " owner="
         << result.owner_thread << " executing=" << result.executing_thread
         << " success=" << result.success);
    // A synchronous task owns its sole completion; no target request table.
    if (completion) completion(std::move(result));
  }

  void Stop() {
    AssertOwner();
    // Complete readiness for unused lazy domains without allocating a VM.
    Initialize();
    publication_->ready.store(false, std::memory_order_release);
    runtime_.reset();
  }

  void NotifyViewCreated(const ProcessRuntime::ViewCreated& event) {
    AssertOwner();
    if (IsHostScriptRuntimeEnabled() && IsRunning() && runtime_)
      runtime_->NotifyViewCreated(event);
  }

 private:
  void AssertOwner() const { DCHECK(runner_->RunsTasksOnCurrentThread()); }
  bool IsRunning() const {
    return process_state_->load(std::memory_order_acquire) ==
           ProcessState::kRunning;
  }
  Result Identity() const {
    Result result = identity_;
    result.executing_thread = CurrentThread();
    return result;
  }

  const fml::RefPtr<fml::TaskRunner> runner_;
  const std::shared_ptr<DomainPublication> publication_;
  const std::shared_ptr<std::atomic<ProcessState>> process_state_;
  // Engine state and destruction remain on runner_. Queued tasks carry their
  // own completion, while Promise handles remain on the calling runtime.
  Result identity_;
  Completion ready_callback_;
  std::optional<Result> initialization_result_;
  const Domain domain_;
  BindingFactory binding_factory_;
  std::string bootstrap_;
  std::unique_ptr<DomainRuntime> runtime_;
};

Result ShutdownResult() {
  Result result;
  result.success = true;
  result.domain = "process";
  result.executing_thread = CurrentThread();
  return result;
}

// A domain generation owns one immutable endpoint. Independent admission and
// lifetime state prevent replacement from redirecting old tasks to a new VM.
class RuntimeGeneration final
    : public std::enable_shared_from_this<RuntimeGeneration> {
 public:
  struct Endpoint {
    std::shared_ptr<DomainPublication> publication;
    std::shared_ptr<DomainState::Actor> actor;
  };

  RuntimeGeneration(Domain domain, fml::RefPtr<fml::TaskRunner> owner,
                    fml::RefPtr<fml::TaskRunner> control, uint64_t runtime_id,
                    const Completion& ready,
                    const BindingFactory& binding_factory,
                    const std::string& bootstrap, InitializationMode mode)
      : control_(std::move(control)),
        state_(std::make_shared<std::atomic<ProcessState>>(
            ProcessState::kRunning)),
        mode_(mode) {
    auto publication =
        std::make_shared<DomainPublication>(domain, runtime_id, owner);
    auto target = std::make_unique<DomainState>(
        owner, publication, state_, ready, domain, binding_factory, bootstrap);
    endpoint_ = {std::move(publication), std::make_shared<DomainState::Actor>(
                                             std::move(target), owner)};
  }

  ProcessState State() const { return state_->load(std::memory_order_acquire); }
  bool IsLazy() const { return mode_ == InitializationMode::kLazy; }

  const Endpoint* Find(Domain domain) const {
    return IsValidDomain(domain) ? &endpoint_ : nullptr;
  }

  // Count admission until work is queued or an inline event returns. Closing
  // never waits: the last submission wakes the lifecycle runner.
  template <typename Factory>
  bool Post(const Endpoint& endpoint, Factory&& task,
            bool allow_inline = false) {
    auto postings = postings_.load(std::memory_order_acquire);
    do {
      if (postings & kClosed) return false;
    } while (!postings_.compare_exchange_weak(postings, postings + 1,
                                              std::memory_order_acq_rel));
    if (allow_inline) {
      endpoint.actor->Act(task());
    } else {
      endpoint.actor->ActAsync(task());
    }
    if (postings_.fetch_sub(1, std::memory_order_acq_rel) == kClosed + 1) {
      control_->PostTask(
          [self = shared_from_this()] { self->StopOnControl(); });
    }
    return true;
  }

  void Start() {
    control_->PostTask([self = shared_from_this()] { self->StartOnControl(); });
  }

  void InitializeBindings() {
    if (State() != ProcessState::kRunning ||
        initialization_posted_.exchange(true))
      return;
    Post(endpoint_, [] { return [](auto& owner) { owner->Initialize(); }; });
  }

  // This also invalidates guards originating in this domain before owner-local
  // destruction can run. It is safe while another domain is executing.
  void Close() {
    auto expected = ProcessState::kRunning;
    state_->compare_exchange_strong(expected, ProcessState::kStopping,
                                    std::memory_order_acq_rel);
    postings_.fetch_or(kClosed, std::memory_order_acq_rel);
  }

  void Shutdown(Completion completion) {
    Close();
    control_->PostTask([self = shared_from_this(),
                        completion = std::move(completion)]() mutable {
      self->ShutdownOnControl(std::move(completion));
    });
  }

 private:
  void StartOnControl() {
    DCHECK(control_->RunsTasksOnCurrentThread());
    if (started_) return;
    started_ = true;
    if (!IsLazy()) InitializeBindings();
  }

  void ShutdownOnControl(Completion completion) {
    DCHECK(control_->RunsTasksOnCurrentThread());
    if (State() == ProcessState::kStopped) {
      if (completion) completion(ShutdownResult());
      return;
    }
    if (completion) shutdown_waiters_.push_back(std::move(completion));
    if (shutdown_started_) return;
    shutdown_started_ = true;
    // Even an unused lazy domain produces its readiness receipt before cleanup.
    StartOnControl();
    StopOnControl();
  }

  void StopOnControl() {
    DCHECK(control_->RunsTasksOnCurrentThread());
    if (!shutdown_started_ || stop_posted_ ||
        postings_.load(std::memory_order_acquire) != kClosed)
      return;
    stop_posted_ = true;
    // Every admitted submission is now enqueued ahead of its owner's Stop.
    endpoint_.actor->ActAsync([self = shared_from_this()](auto& owner) {
      owner->Stop();
      self->control_->PostTask([self] { self->DomainStopped(); });
    });
  }

  void DomainStopped() {
    DCHECK(control_->RunsTasksOnCurrentThread());
    state_->store(ProcessState::kStopped, std::memory_order_release);
    auto waiters = std::move(shutdown_waiters_);
    shutdown_waiters_.clear();
    for (auto& waiter : waiters) waiter(ShutdownResult());
  }

  static constexpr uint64_t kClosed = uint64_t{1} << 63;
  std::atomic<uint64_t> postings_{0};
  std::atomic<bool> initialization_posted_{false};
  const fml::RefPtr<fml::TaskRunner> control_;
  const std::shared_ptr<std::atomic<ProcessState>> state_;
  const InitializationMode mode_;
  Endpoint endpoint_;
  // Only control_ accesses these fields. No per-request process registry.
  std::vector<Completion> shutdown_waiters_;
  bool started_ = false;
  bool shutdown_started_ = false;
  bool stop_posted_ = false;
};

}  // namespace

class ProcessRuntime::Impl {
 public:
  bool Initialize(Runners runners, Completion ready,
                  BindingFactory binding_factory, std::string bootstrap,
                  InitializationMode mode) {
    if (!IsHostScriptRuntimeEnabled()) return false;
    if (!bootstrap.empty() && !NormalizeScriptSource(bootstrap)) return false;
    if (!runners.bts || !runners.mts || !runners.ui) return false;
    std::array<std::shared_ptr<RuntimeGeneration>, kHostScriptDomainCount>
        domains;
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      if (load_ || state_ != ProcessState::kStopped) return false;
      runners_ = runners;
      binding_factory_ = std::move(binding_factory);
      bootstrap_ = std::move(bootstrap);
      ++process_id_;
      state_ = ProcessState::kRunning;
      for (size_t i = 0; i < domains.size(); ++i) {
        auto domain = static_cast<Domain>(i);
        domains[i] = MakeDomain(domain, ready, mode);
        std::atomic_store(&current_[i], domains[i]);
      }
    }
    for (const auto& domain : domains) domain->Start();
    return true;
  }

  void InitializeBindings() {
    if (!IsHostScriptRuntimeEnabled()) return;
    for (size_t i = 0; i < current_.size(); ++i) {
      if (auto domain = Current(static_cast<Domain>(i)))
        domain->InitializeBindings();
    }
  }

  bool IsReady(Domain domain) const {
    auto generation = Current(domain);
    auto* endpoint = generation ? generation->Find(domain) : nullptr;
    return IsHostScriptRuntimeEnabled() && !IsLoading(domain) && generation &&
           generation->State() == ProcessState::kRunning && endpoint &&
           endpoint->publication->ready.load(std::memory_order_acquire);
  }

  void Evaluate(Domain domain, std::string source, std::string url,
                Completion completion, Guard guard, bool from_binding = false) {
    auto generation = Current(domain);
    auto* endpoint = generation ? generation->Find(domain) : nullptr;
    const char* error =
        !IsHostScriptRuntimeEnabled()        ? "HSR_DEBUG_DISABLED"
        : IsLoading(domain) && !from_binding ? "RUNTIME_LOADING"
        : !IsValidDomain(domain)             ? "INVALID_DOMAIN"
        : !generation || generation->State() != ProcessState::kRunning
            ? "RUNTIME_NOT_RUNNING"
        : !generation->IsLazy() &&
                !endpoint->publication->ready.load(std::memory_order_acquire)
            ? "RUNTIME_NOT_READY"
            : nullptr;
    auto identity = endpoint ? endpoint->publication->Snapshot() : Result{};
    identity.domain = HostScriptDomainName(domain);
    if (error) {
      if (completion) completion(WithError(std::move(identity), error));
      return;
    }
    if (!NormalizeScriptSource(source) || url.empty()) {
      if (completion)
        completion(WithError(std::move(identity), "INVALID_SCRIPT_SOURCE"));
      return;
    }
    if (!generation->Post(
            *endpoint,
            [&] {
              return [source = std::move(source), url = std::move(url),
                      completion = std::move(completion),
                      guard = std::move(guard)](auto& owner) mutable {
                owner->Evaluate(source, url, std::move(completion), guard);
              };
            }) &&
        completion) {
      completion(WithError(std::move(identity), "RUNTIME_NOT_RUNNING"));
    }
  }

  void NotifyViewCreated(const ViewCreated& event) {
    for (size_t i = 0; i < current_.size(); ++i) {
      auto generation = Current(static_cast<Domain>(i));
      if (!generation || generation->State() != ProcessState::kRunning ||
          !generation->Find(static_cast<Domain>(i))
               ->publication->ready.load(std::memory_order_acquire))
        continue;
      generation->Post(
          *generation->Find(static_cast<Domain>(i)),
          [&] {
            return [event](auto& owner) { owner->NotifyViewCreated(event); };
          },
          /*allow_inline=*/true);
    }
  }

  void Shutdown(Completion completion) {
    std::array<std::shared_ptr<RuntimeGeneration>, kHostScriptDomainCount>
        domains;
    std::shared_ptr<Load> load;
    uint64_t process_id;
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      load = load_;
      if (load) load->cancelled = true;
      runners_ = {};
      binding_factory_ = {};
      bootstrap_.clear();
      process_id = process_id_;
      for (size_t i = 0; i < domains.size(); ++i) {
        domains[i] = Current(static_cast<Domain>(i));
        if (domains[i]) domains[i]->Close();
      }
      if (domains[0]) state_ = ProcessState::kStopping;
    }
    if (!domains[0]) {
      if (completion) completion(ShutdownResult());
      return;
    }
    // Every domain acknowledges on the same lifecycle runner.
    auto remaining = std::make_shared<size_t>(domains.size());
    auto done = std::make_shared<Completion>(std::move(completion));
    for (const auto& domain : domains) {
      domain->Shutdown([this, load, remaining, done, process_id](Result) {
        if (--*remaining != 0) return;
        {
          std::lock_guard<std::mutex> lock(lifecycle_mutex_);
          if (process_id_ == process_id) state_ = ProcessState::kStopped;
        }
        if (load) FinishLoad(load, WithError({}, "RUNTIME_SHUTDOWN"));
        if (*done) {
          auto completion = std::move(*done);
          completion(ShutdownResult());
        }
      });
    }
  }

  void LoadScript(Domain domain, std::string source, std::string url,
                  Completion completion) {
    if (!IsValidDomain(domain)) {
      if (completion) completion(WithError({}, "INVALID_DOMAIN"));
      return;
    }
    if (!NormalizeScriptSource(source, true) || url.empty()) {
      if (completion) completion(WithError({}, "INVALID_SCRIPT_SOURCE"));
      return;
    }
    auto load = std::make_shared<Load>();
    load->domain = domain;
    load->source = std::move(source);
    load->url = std::move(url);
    load->completion = std::move(completion);
    std::shared_ptr<RuntimeGeneration> generation;
    const char* error = nullptr;
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      generation = Current(domain);
      error = !IsHostScriptRuntimeEnabled() ? "HSR_DEBUG_DISABLED"
              : load_                       ? "RUNTIME_LOADING"
              : state_ != ProcessState::kRunning || !generation
                  ? "RUNTIME_NOT_RUNNING"
                  : nullptr;
      if (!error) {
        load_ = load;
        loading_domain_.store(static_cast<int>(domain),
                              std::memory_order_release);
        generation->Close();
      }
    }
    if (error) {
      if (load->completion) load->completion(WithError({}, error));
      return;
    }
    generation->Shutdown([this, load](Result) { RestartLoad(load); });
  }

 private:
  struct Load {
    Domain domain = Domain::kBTS;
    std::string source;
    std::string url;
    Completion completion;
    bool cancelled = false;
  };

  std::shared_ptr<RuntimeGeneration> MakeDomain(Domain domain,
                                                const Completion& ready,
                                                InitializationMode mode) {
    const Owners owners{runners_.bts, runners_.mts, runners_.ui};
    return std::make_shared<RuntimeGeneration>(
        domain, owners[HostScriptDomainIndex(domain)], runners_.ui,
        next_runtime_id_.fetch_add(1) + 1, ready, binding_factory_, bootstrap_,
        mode);
  }

  void RestartLoad(const std::shared_ptr<Load>& load) {
    std::shared_ptr<RuntimeGeneration> next;
    const char* error = "RUNTIME_SHUTDOWN";
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      if (load_ != load) return;
      if (!load->cancelled && state_ == ProcessState::kRunning) {
        if (IsHostScriptRuntimeEnabled()) {
          next = MakeDomain(
              load->domain,
              [this, load](Result result) { LoadDomainReady(load, result); },
              InitializationMode::kEager);
          std::atomic_store(&current_[HostScriptDomainIndex(load->domain)],
                            next);
        } else {
          error = "HSR_DEBUG_DISABLED";
        }
      }
    }
    if (next) {
      next->Start();
    } else {
      FinishLoad(load, WithError({}, error));
    }
  }

  void LoadDomainReady(const std::shared_ptr<Load>& load, const Result& ready) {
    std::shared_ptr<RuntimeGeneration> generation;
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      if (load_ != load || load->cancelled) return;
      generation = Current(load->domain);
    }
    if (!ready.success) {
      StopFailedLoad(load, ready);
      return;
    }
    if (!generation->Post(*generation->Find(load->domain), [this, load] {
          return [this, load](auto& owner) {
            owner->Evaluate(
                load->source, load->url,
                [this, load](Result result) {
                  if (result.success) {
                    FinishLoad(load, std::move(result));
                  } else {
                    StopFailedLoad(load, std::move(result));
                  }
                },
                {}, CompletionValue::kIgnored);
          };
        })) {
      StopFailedLoad(load, WithError({}, "RUNTIME_SHUTDOWN"));
    }
  }

  void StopFailedLoad(const std::shared_ptr<Load>& load, Result result) {
    std::shared_ptr<RuntimeGeneration> generation;
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      if (load_ != load) return;
      generation = Current(load->domain);
    }
    generation->Shutdown(
        [this, load, result = std::move(result)](Result) mutable {
          FinishLoad(load, std::move(result));
        });
  }

  void FinishLoad(const std::shared_ptr<Load>& load, Result result) {
    Completion completion;
    bool cleanup = false;
    {
      std::lock_guard<std::mutex> lock(lifecycle_mutex_);
      if (load_ != load) return;
      cleanup =
          result.success && (load->cancelled || !IsHostScriptRuntimeEnabled());
      if (load->cancelled)
        result = WithError(std::move(result), "RUNTIME_SHUTDOWN");
      else if (cleanup)
        result = WithError(std::move(result), "HSR_DEBUG_DISABLED");
      if (!cleanup) {
        completion = std::move(load->completion);
        load_.reset();
        loading_domain_.store(-1, std::memory_order_release);
      }
    }
    if (cleanup) StopFailedLoad(load, std::move(result));
    if (completion) completion(std::move(result));
  }

  bool IsLoading(Domain domain) const {
    return IsValidDomain(domain) &&
           loading_domain_.load(std::memory_order_acquire) ==
               static_cast<int>(domain);
  }

  std::shared_ptr<RuntimeGeneration> Current(Domain domain) const {
    return IsValidDomain(domain)
               ? std::atomic_load(&current_[HostScriptDomainIndex(domain)])
               : nullptr;
  }

  std::array<std::shared_ptr<RuntimeGeneration>, kHostScriptDomainCount>
      current_;
  std::atomic<uint64_t> next_runtime_id_{0};
  // Serializes replacement with explicit initialization/shutdown, never JS.
  std::mutex lifecycle_mutex_;
  ProcessState state_ = ProcessState::kStopped;
  uint64_t process_id_ = 0;
  std::atomic<int> loading_domain_{-1};
  std::shared_ptr<Load> load_;
  Runners runners_;
  BindingFactory binding_factory_;
  std::string bootstrap_;
};

ProcessRuntime& ProcessRuntime::GetInstance() {
  // Process lifetime avoids destroying VM state from a static destructor.
  static auto* runtime = new ProcessRuntime();
  return *runtime;
}

ProcessRuntime::ProcessRuntime() : impl_(std::make_unique<Impl>()) {}
ProcessRuntime::~ProcessRuntime() = default;

bool ProcessRuntime::Initialize(Runners runners, Completion per_domain_ready,
                                BindingFactory binding_factory,
                                std::string bootstrap,
                                InitializationMode mode) {
  return impl_->Initialize(std::move(runners), std::move(per_domain_ready),
                           std::move(binding_factory), std::move(bootstrap),
                           mode);
}

void ProcessRuntime::InitializeBindings() { impl_->InitializeBindings(); }

bool ProcessRuntime::IsReady(Domain domain) const {
  return impl_->IsReady(domain);
}

void ProcessRuntime::NotifyViewCreated(ViewCreated event) {
  impl_->NotifyViewCreated(event);
}

void ProcessRuntime::Evaluate(Domain domain, std::string source,
                              std::string url, Completion completion) {
  impl_->Evaluate(domain, std::move(source), std::move(url),
                  std::move(completion), {});
}

void ProcessRuntime::RunOnThread(Domain domain, std::string source,
                                 std::string url, Completion completion,
                                 Guard guard) {
  impl_->Evaluate(domain, std::move(source), std::move(url),
                  std::move(completion), std::move(guard), true);
}

void ProcessRuntime::LoadScript(std::string source, std::string url,
                                Completion completion, Domain domain) {
  impl_->LoadScript(domain, std::move(source), std::move(url),
                    std::move(completion));
}

void ProcessRuntime::Shutdown(Completion completion) {
  impl_->Shutdown(std::move(completion));
}

}  // namespace shell
}  // namespace lynx
