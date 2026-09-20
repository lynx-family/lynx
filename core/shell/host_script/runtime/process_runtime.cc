// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/process_runtime.h"

#include <array>
#include <atomic>
#include <optional>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#include "base/include/log/logging.h"
#include "base/include/lynx_actor.h"
#include "base/include/string/string_utils.h"
#include "core/runtime/js/js_executor.h"
#include "core/shell/host_script/runtime/host_script_thread_bindings.h"

namespace lynx {
namespace shell {
namespace {

using Domain = ProcessRuntime::Domain;
using Result = ProcessRuntime::Result;
using Completion = ProcessRuntime::Completion;
using Guard = ProcessRuntime::Guard;
using BindingFactory = ProcessRuntime::BindingFactory;
enum class ProcessState { kRunning, kStopping, kStopped };

std::string CurrentThread() {
  std::ostringstream stream;
  stream << std::this_thread::get_id();
  return stream.str();
}

size_t DomainIndex(Domain domain) { return static_cast<size_t>(domain); }
bool IsValidDomain(Domain domain) { return DomainIndex(domain) < 3; }

// Input-byte normalization only; JavaScript syntax belongs to the engine.
bool NormalizeScriptSource(std::string& source) {
  constexpr size_t kMaxSourceBytes = 512 * 1024;
  if (source.empty() || source.size() > kMaxSourceBytes ||
      !base::IsValidUtf8(reinterpret_cast<const uint8_t*>(source.data()),
                         source.size()))
    return false;
  if (source.compare(0, 3, "\xEF\xBB\xBF") == 0) source.erase(0, 3);
  return !source.empty();
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
                std::unique_ptr<ProcessRuntime::RuntimeBindings> bindings)
      : identity_(std::move(identity)),
        runner_(std::move(runner)),
        bindings_(std::move(bindings)) {}

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
        [](Domain domain, std::string source, std::string url,
           Completion completion, Guard guard) {
          ProcessRuntime::GetInstance().RunOnThread(
              domain, std::move(source), std::move(url), std::move(completion),
              std::move(guard));
        });
    if (!thread_bindings_ || !thread_bindings_->Install())
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

  Result Execute(const std::string& source, const std::string& url,
                 bool serialize_result = true) {
    DCHECK(runner_->RunsTasksOnCurrentThread());
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
    if (value->isObject()) {
      auto then = value->getObject(runtime).getProperty(runtime, "then");
      if (!then) return FailedCall();
      if (then->isObject() && then->getObject(runtime).isFunction(runtime))
        return WithError(Identity(), "ASYNC_RESULT_UNSUPPORTED");
    }
    if (serialize_result && !value->isUndefined()) {
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
  std::optional<runtime::js::Function> stringify_;
  bool executing_ = false;
};

// Only immutable identity and an atomic readiness snapshot cross runners.
struct DomainPublication {
  DomainPublication(Domain domain, uint64_t runtime_id,
                    const fml::RefPtr<fml::TaskRunner>& runner) {
    identity.domain = ProcessRuntime::DomainName(domain);
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

  void Initialize() {
    AssertOwner();
    identity_.owner_thread = CurrentThread();
    Result result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    if (IsRunning()) {
      runtime_ = std::make_unique<DomainRuntime>(
          identity_, runner_,
          binding_factory_ ? binding_factory_(domain_) : nullptr);
      result = runtime_->Initialize();
      if (result.success && !bootstrap_.empty()) {
        result =
            runtime_->Execute(bootstrap_, "host-script-bootstrap.js", false);
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
    auto ready = std::move(ready_callback_);
    if (ready) ready(std::move(result));
  }

  void Evaluate(const std::string& source, const std::string& url,
                Completion completion, const Guard& guard) {
    AssertOwner();
    Result result;
    if (!IsRunning()) {
      result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    } else if (guard && !guard()) {
      result = WithError(Identity(), "SOURCE_RUNTIME_DETACHED");
    } else if (!IsRunning()) {
      // A guard is allowed to request shutdown.
      result = WithError(Identity(), "RUNTIME_SHUTDOWN");
    } else if (!runtime_) {
      result = WithError(Identity(), "RUNTIME_NOT_READY");
    } else {
      result = runtime_->Execute(source, url);
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
    publication_->ready.store(false, std::memory_order_release);
    runtime_.reset();
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

// A published generation has immutable endpoints. Only its lifecycle runner
// coordinates initialization and shutdown; ordinary execution never hops there.
class RuntimeGeneration final
    : public std::enable_shared_from_this<RuntimeGeneration> {
 public:
  struct Endpoint {
    std::shared_ptr<DomainPublication> publication;
    std::shared_ptr<DomainState::Actor> actor;
  };

  RuntimeGeneration(std::array<fml::RefPtr<fml::TaskRunner>, 3> owners,
                    uint64_t first_id, const Completion& ready,
                    const BindingFactory& binding_factory,
                    const std::string& bootstrap)
      : control_(owners[2]),
        state_(std::make_shared<std::atomic<ProcessState>>(
            ProcessState::kRunning)) {
    for (size_t i = 0; i < endpoints_.size(); ++i) {
      auto domain = static_cast<Domain>(i);
      auto publication =
          std::make_shared<DomainPublication>(domain, first_id + i, owners[i]);
      auto owner =
          std::make_unique<DomainState>(owners[i], publication, state_, ready,
                                        domain, binding_factory, bootstrap);
      endpoints_[i] = {
          std::move(publication),
          std::make_shared<DomainState::Actor>(std::move(owner), owners[i])};
    }
  }

  ProcessState State() const { return state_->load(std::memory_order_acquire); }

  const Endpoint* Find(Domain domain) const {
    return IsValidDomain(domain) ? &endpoints_[DomainIndex(domain)] : nullptr;
  }

  // Count only submissions between admission and enqueueing. Closing never
  // waits on the calling thread: the last poster wakes the lifecycle runner.
  template <typename Factory>
  bool Post(const Endpoint& endpoint, Factory&& task) {
    auto postings = postings_.load(std::memory_order_acquire);
    do {
      if (postings & kClosed) return false;
    } while (!postings_.compare_exchange_weak(postings, postings + 1,
                                              std::memory_order_acq_rel));
    endpoint.actor->ActAsync(task());
    if (postings_.fetch_sub(1, std::memory_order_acq_rel) == kClosed + 1) {
      control_->PostTask(
          [self = shared_from_this()] { self->StopOnControl(); });
    }
    return true;
  }

  void Start() {
    control_->PostTask([self = shared_from_this()] { self->StartOnControl(); });
  }

  void Shutdown(Completion completion) {
    auto expected = ProcessState::kRunning;
    state_->compare_exchange_strong(expected, ProcessState::kStopping,
                                    std::memory_order_acq_rel);
    postings_.fetch_or(kClosed, std::memory_order_acq_rel);
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
    for (const auto& endpoint : endpoints_) {
      endpoint.actor->ActAsync([](auto& owner) { owner->Initialize(); });
    }
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
    // Shutdown can reach the control queue before Start's task. Still produce
    // all three initialization receipts, followed by owner-local cleanup.
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
    for (const auto& endpoint : endpoints_) {
      endpoint.actor->ActAsync([self = shared_from_this()](auto& owner) {
        owner->Stop();
        self->control_->PostTask([self] { self->DomainStopped(); });
      });
    }
  }

  void DomainStopped() {
    DCHECK(control_->RunsTasksOnCurrentThread());
    if (++stopped_domains_ != endpoints_.size()) return;
    state_->store(ProcessState::kStopped, std::memory_order_release);
    auto waiters = std::move(shutdown_waiters_);
    shutdown_waiters_.clear();
    for (auto& waiter : waiters) waiter(ShutdownResult());
  }

  static constexpr uint64_t kClosed = uint64_t{1} << 63;
  std::atomic<uint64_t> postings_{0};
  const fml::RefPtr<fml::TaskRunner> control_;
  const std::shared_ptr<std::atomic<ProcessState>> state_;
  std::array<Endpoint, 3> endpoints_;
  // Only control_ accesses these fields. No per-request process registry.
  std::vector<Completion> shutdown_waiters_;
  size_t stopped_domains_ = 0;
  bool started_ = false;
  bool shutdown_started_ = false;
  bool stop_posted_ = false;
};

}  // namespace

class ProcessRuntime::Impl {
 public:
  bool Initialize(Runners runners, Completion ready,
                  BindingFactory binding_factory, std::string bootstrap) {
    if (!bootstrap.empty() && !NormalizeScriptSource(bootstrap)) return false;
    if (!runners.bts || !runners.mts || !runners.ui) return false;
    auto current = Current();
    if (current && current->State() != ProcessState::kStopped) return false;
    auto next = std::make_shared<RuntimeGeneration>(
        std::array<fml::RefPtr<fml::TaskRunner>, 3>{std::move(runners.bts),
                                                    std::move(runners.mts),
                                                    std::move(runners.ui)},
        next_runtime_id_.fetch_add(3) + 1, ready, binding_factory, bootstrap);
    if (!std::atomic_compare_exchange_strong(&current_, &current, next))
      return false;
    next->Start();
    return true;
  }

  bool IsReady(Domain domain) const {
    auto generation = Current();
    auto* endpoint = generation ? generation->Find(domain) : nullptr;
    return generation && generation->State() == ProcessState::kRunning &&
           endpoint &&
           endpoint->publication->ready.load(std::memory_order_acquire);
  }

  void Evaluate(Domain domain, std::string source, std::string url,
                Completion completion, Guard guard) {
    auto generation = Current();
    auto* endpoint = generation ? generation->Find(domain) : nullptr;
    const char* error =
        !IsValidDomain(domain) ? "INVALID_DOMAIN"
        : !generation || generation->State() != ProcessState::kRunning
            ? "RUNTIME_NOT_RUNNING"
        : !endpoint->publication->ready.load(std::memory_order_acquire)
            ? "RUNTIME_NOT_READY"
            : nullptr;
    auto identity = endpoint ? endpoint->publication->Snapshot() : Result{};
    identity.domain = DomainName(domain);
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

  void Shutdown(Completion completion) {
    auto generation = Current();
    if (generation) {
      generation->Shutdown(std::move(completion));
    } else if (completion) {
      completion(ShutdownResult());
    }
  }

 private:
  std::shared_ptr<RuntimeGeneration> Current() const {
    return std::atomic_load(&current_);
  }
  // Publish immutable routing only; no process-level request registry.
  std::shared_ptr<RuntimeGeneration> current_;
  std::atomic<uint64_t> next_runtime_id_{0};
};

ProcessRuntime& ProcessRuntime::GetInstance() {
  // Process lifetime avoids destroying VM state from a static destructor.
  static auto* runtime = new ProcessRuntime();
  return *runtime;
}

const char* ProcessRuntime::DomainName(Domain domain) {
  switch (domain) {
    case Domain::kBTS:
      return "bts";
    case Domain::kMTS:
      return "mts";
    case Domain::kUI:
      return "ui";
  }
  return "unknown";
}

ProcessRuntime::ProcessRuntime() : impl_(std::make_unique<Impl>()) {}
ProcessRuntime::~ProcessRuntime() = default;

bool ProcessRuntime::Initialize(Runners runners, Completion per_domain_ready,
                                BindingFactory binding_factory,
                                std::string bootstrap) {
  return impl_->Initialize(std::move(runners), std::move(per_domain_ready),
                           std::move(binding_factory), std::move(bootstrap));
}

bool ProcessRuntime::IsReady(Domain domain) const {
  return impl_->IsReady(domain);
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
                  std::move(completion), std::move(guard));
}

void ProcessRuntime::Shutdown(Completion completion) {
  impl_->Shutdown(std::move(completion));
}

}  // namespace shell
}  // namespace lynx
