// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_PROCESS_RUNTIME_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_PROCESS_RUNTIME_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "base/include/fml/task_runner.h"
#include "core/base/lynx_export.h"
#include "core/base/memory/unsafe_owning_ptr.h"

namespace lynx {
namespace runtime {
namespace js {
class Runtime;
}  // namespace js
}  // namespace runtime
namespace shell {

// Exactly three process-owned control contexts, one per BTS/MTS/UI domain.
// Each context stays on the runner supplied at initialization; pages cannot
// create additional control contexts or change those runners.
// These do not replace a page's JS or MTS context.
class LYNX_EXPORT_FOR_DEVTOOL ProcessRuntime final {
 public:
  enum class Domain { kBTS, kMTS, kUI };
  enum class InitializationMode { kEager, kLazy };

  // Optional capabilities are created, attached and destroyed on their owner.
  // They do not participate in cross-runtime scheduling.
  class RuntimeBindings {
   public:
    virtual ~RuntimeBindings() = default;
    virtual bool Attach(base::UnsafeWeakPtr<runtime::js::Runtime> runtime,
                        fml::RefPtr<fml::TaskRunner> runner) = 0;
  };
  using BindingFactory =
      std::function<std::unique_ptr<RuntimeBindings>(Domain)>;

  struct Runners {
    fml::RefPtr<fml::TaskRunner> bts;
    fml::RefPtr<fml::TaskRunner> mts;
    fml::RefPtr<fml::TaskRunner> ui;
  };

  struct Result {
    bool success = false;
    // Undefined has no value; JSON null has value_json == "null".
    bool has_value = false;
    std::string value_json;
    std::string error;
    std::string domain;
    uint64_t runtime_id = 0;
    uint64_t context_id = 0;
    uint64_t owner_runner_id = 0;
    std::string owner_thread;
    std::string executing_thread;
  };

  // Callbacks must not block their runner or throw C++ exceptions.
  using Completion = std::function<void(Result)>;
  // Runs on the target runner just before evaluation; must be thread-safe.
  using Guard = std::function<bool()>;

  static ProcessRuntime& GetInstance();
  static const char* DomainName(Domain domain);

  // Publishes three fixed execution domains on the supplied runners. Eager
  // initialization creates all contexts before accepting scripts. Lazy
  // initialization creates a context, installs its bindings and runs bootstrap
  // on InitializeBindings or the first Evaluate/RunOnThread for that domain.
  // Readiness is reported once per domain on its owner. An unused lazy domain
  // reports cancellation at shutdown without creating a context. Bootstrap
  // must complete synchronously.
  // Runners must remain alive and service tasks until Shutdown completes.
  bool Initialize(Runners runners, Completion per_domain_ready,
                  BindingFactory binding_factory = {},
                  std::string bootstrap = {},
                  InitializationMode mode = InitializationMode::kEager);
  // Requests context/binding initialization on all three owners without
  // evaluating a script. Repeated calls reuse the current generation.
  void InitializeBindings();
  // Whether this domain's context and bindings are initialized. In lazy mode,
  // false does not prevent submission. Never posts or waits for another runner.
  bool IsReady(Domain domain) const;

  // Executes source synchronously on the selected runtime without blocking the
  // caller. Results are copied as JSON; Promise/thenable results are rejected.
  // A submitted task completes on its target runner. Admission errors complete
  // on the calling thread; callers must marshal callbacks to their own owner.
  void Evaluate(Domain domain, std::string source, std::string url,
                Completion completion);
  void RunOnThread(Domain domain, std::string source, std::string url,
                   Completion completion, Guard guard = {});

  // Closes admission immediately, then frees each VM on its owner. Completes
  // on the UI lifecycle runner after all three owners acknowledge cleanup.
  // Reinitialization is allowed only after completion. With no generation the
  // callback runs immediately. Does not interrupt already-running JavaScript.
  void Shutdown(Completion completion);

 private:
  ProcessRuntime();
  ~ProcessRuntime();
  ProcessRuntime(const ProcessRuntime&) = delete;
  ProcessRuntime& operator=(const ProcessRuntime&) = delete;

  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_PROCESS_RUNTIME_H_
