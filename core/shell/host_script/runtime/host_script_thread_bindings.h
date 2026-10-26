// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_THREAD_BINDINGS_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_THREAD_BINDINGS_H_

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "base/include/fml/task_runner.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/shell/host_script/runtime/process_runtime.h"

namespace lynx {
namespace shell {

// Runtime admission is independent of debug-engine compilation. A compiled
// implementation accepts work only while its debug lifecycle is enabled.
bool IsHostScriptRuntimeEnabled();

// One source context owns its Promise callbacks. Copied source and native
// results cross runners; no JSI handles leave their owning runtime.
class HostScriptThreadBindings final
    : public std::enable_shared_from_this<HostScriptThreadBindings> {
 public:
  using Domain = ProcessRuntime::Domain;
  using Result = ProcessRuntime::Result;
  using Completion = ProcessRuntime::Completion;
  using Executor = std::function<void(Domain, std::string, std::string,
                                      Completion, std::function<bool()>)>;

  static std::shared_ptr<HostScriptThreadBindings> Create(
      runtime::js::Runtime& runtime, fml::RefPtr<fml::TaskRunner> runner,
      Executor executor, ProcessRuntime::Guard admission_guard = {});

  // Installs runOn* and __hostScriptThread function/source bindings.
  // Same-domain functions keep their closure; cross-domain functions must use
  // target globals or local variables. Only copied source crosses runners.
  // Install and Detach must run on the owning runner, before engine teardown.
  bool Install(const std::string& domain = "bts");
  void Detach();

 private:
  using Runtime = runtime::js::Runtime;
  using Value = runtime::js::Value;
  using Function = runtime::js::Function;
  using CallResult = base::expected<Value, runtime::js::JSINativeException>;
  struct Pending {
    std::optional<Function> resolve;
    std::optional<Function> reject;
  };

  HostScriptThreadBindings(Runtime& runtime,
                           fml::RefPtr<fml::TaskRunner> runner,
                           Executor executor,
                           ProcessRuntime::Guard admission_guard);
  CallResult Run(Domain domain, const Value* args, size_t count);
  bool Admitted() const;
  bool DefineProperty(runtime::js::Object& target, const char* name,
                      Value value, bool mutable_export = true);
  void Complete(uint64_t request, const Result& result);
  void Reject(Pending pending, const char* code, const char* message);

  Runtime& runtime_;
  fml::RefPtr<fml::TaskRunner> runner_;
  Executor executor_;
  const ProcessRuntime::Guard admission_guard_;
  std::atomic<bool> attached_{false};
  // Owner-thread state; Detach clears every engine reference explicitly.
  std::optional<Function> promise_;
  std::optional<Function> error_;
  std::optional<Function> parse_json_;
  std::optional<Function> define_property_;
  std::optional<Function> create_object_;
  std::optional<Function> get_prototype_;
  std::optional<Function> function_source_;
  std::optional<Function> invoke_sync_;
  std::optional<runtime::js::Object> function_prototype_;
  std::string domain_;
  uint64_t next_request_ = 0;
  std::unordered_map<uint64_t, Pending> pending_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_THREAD_BINDINGS_H_
