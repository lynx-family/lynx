// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_FIXTURE_JS_RUNTIME_H_
#define CORE_SERVICES_REPLAY_FIXTURE_JS_RUNTIME_H_

#include <cstddef>
#include <memory>
#include <string>

#include "core/runtime/js/jsi/jsi.h"
#include "core/services/replay/fixture_common.h"

namespace lynx {
namespace tasm {
namespace replay {

// The caller chooses the fallback for values that cannot be serialized. This
// may invoke JavaScript (e.g. toJSON), so keep it inside an execution scope.
std::string FixtureValueToJson(runtime::js::Runtime& runtime,
                               const runtime::js::Value& value,
                               const char* fallback);

// Owns a copy of the directory and budget. JSON assets are parsed; malformed
// JSON falls back to text. Unreadable/empty/oversized assets return undefined.
runtime::js::Function CreateFixtureReadAssetFunction(
    runtime::js::Runtime& runtime, const std::string& fixture_directory,
    size_t max_bytes = kDefaultFixtureMaxAssetBytes);

// Owns an isolated JSI runtime and its VM/context. Create, use and destroy it
// on the same thread. All externally held JSI values must be released first.
// Engine implementations supply execution limits; fixture bindings use JSI.
class FixtureJsRuntime {
 public:
  virtual ~FixtureJsRuntime();

  FixtureJsRuntime(const FixtureJsRuntime&) = delete;
  FixtureJsRuntime& operator=(const FixtureJsRuntime&) = delete;

  runtime::js::Runtime& runtime() { return *runtime_; }

  // Executes a plain fixture body or the writer's "export default function"
  // form against the caller-installed global ctx. This is not an ESM loader.
  base::expected<runtime::js::Value, runtime::js::JSINativeException>
  EvaluateScript(std::string source);

  // Retained until the next outermost execution scope begins.
  virtual bool timed_out() const = 0;

 protected:
  // The backend initializes the VM/context first so engine-specific creation
  // options can be applied before the runtime is exposed to fixture code.
  explicit FixtureJsRuntime(std::unique_ptr<runtime::js::Runtime> runtime);

 private:
  friend class ScopedFixtureExecution;
  void BeginExecution();
  void EndExecution();
  virtual bool IsHeapWithinLimit() = 0;
  virtual void ArmExecutionDeadline() = 0;
  virtual void DisarmExecutionDeadline() = 0;

  // The initialized JSI runtime retains its context and VM.
  std::unique_ptr<runtime::js::Runtime> runtime_;
  size_t execution_depth_ = 0;
};

// Enters JSI and bounds a complete synchronous operation. Keep conversions and
// serialization inside this scope too. Nested scopes share the outer budget.
class ScopedFixtureExecution {
 public:
  explicit ScopedFixtureExecution(FixtureJsRuntime& runtime);
  ~ScopedFixtureExecution();

  ScopedFixtureExecution(const ScopedFixtureExecution&) = delete;
  ScopedFixtureExecution& operator=(const ScopedFixtureExecution&) = delete;

 private:
  FixtureJsRuntime& runtime_;
  runtime::js::Scope scope_;
};

// Currently the only backend. Further engines can implement FixtureJsRuntime
// without changing the resource helpers, DSL bindings or evaluation logic.
// Returns null for invalid limits or a budget below engine startup usage.
// Neither limit can be disabled with zero.
std::unique_ptr<FixtureJsRuntime> CreateQuickJsFixtureRuntime(
    const FixtureRuntimeLimits& limits = FixtureRuntimeLimits{});

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_FIXTURE_JS_RUNTIME_H_
