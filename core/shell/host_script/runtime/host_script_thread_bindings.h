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
      Executor executor);

  // Installs the low-level __hostScriptThread source-execution binding.
  // Promises resolve to copied JS values and reject on execution errors.
  // Install and Detach must run on the owning runner, before engine teardown.
  bool Install();
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
                           Executor executor);
  CallResult Run(Domain domain, const Value* args, size_t count);
  bool DefineProperty(runtime::js::Object& target, const char* name,
                      Value value);
  void Complete(uint64_t request, const Result& result);
  void Reject(Pending pending, const char* code, const char* message);

  Runtime& runtime_;
  fml::RefPtr<fml::TaskRunner> runner_;
  Executor executor_;
  std::atomic<bool> attached_{false};
  // Owner-thread state; Detach clears every engine reference explicitly.
  std::optional<Function> promise_;
  std::optional<Function> error_;
  std::optional<Function> parse_json_;
  std::optional<Function> define_property_;
  std::optional<Function> create_object_;
  uint64_t next_request_ = 0;
  std::unordered_map<uint64_t, Pending> pending_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_THREAD_BINDINGS_H_
