// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_FIXTURE_CONTEXT_H_
#define CORE_SERVICES_REPLAY_FIXTURE_CONTEXT_H_

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/runtime/js/jsi/jsi.h"

// Opaque QuickJS runtime type (defined in quickjs.h); forward declared here to
// avoid exposing QuickJS headers from this file.
typedef struct LEPUSRuntime LEPUSRuntime;

namespace lynx {
namespace runtime {
namespace js {

// Fixture replay code avoids std::optional::value(): its throwing path
// references std::bad_optional_access, which breaks compatibility with the
// Lynx iOS 10 deployment target. Use has_value() with operator*/operator->
// instead.

// Resource limits applied to the fixture runtime. Mirrors the protections of
// tasm::replay::FixtureEvaluationLimits (fixture_evaluator.h): fixture scripts
// are developer-authored (and may be downloaded from a fixture hub), so both
// the offline evaluation pass and the live dispatch session must be bounded.
struct FixtureContextLimits {
  // Wall-clock budget for one synchronous JS execution: the fixture.js
  // top-level evaluation or one handler dispatch.
  int64_t timeout_ms = 5000;
  // Total QuickJS heap budget for the fixture runtime. 0 disables the limit.
  size_t memory_limit_bytes = 64 * 1024 * 1024;
  // fixture.js larger than this fails Initialize. 0 disables the limit.
  size_t max_script_bytes = 4 * 1024 * 1024;
  // ctx.readAsset() files larger than this read as undefined. 0 disables the
  // limit.
  size_t max_asset_bytes = 16 * 1024 * 1024;
};

// Result of dispatching one NativeModule call into the fixture runtime.
//
// The fixture runtime is fully decoupled from the business JS runtime: args go
// in as JSON, results come out as JSON. The caller (ModuleFixtureReplay) is
// responsible for converting `return_value_json` into a JSI value in the
// business runtime and for invoking the recorded callbacks asynchronously,
// preserving JSON replay callback semantics.
struct FixtureDispatchResult {
  // Whether a handler was registered for (module, method). When false the
  // caller should treat the call as an unmatched JSB.
  bool handled = false;

  // JSON serialization of the handler's return value. Empty or the literal
  // string "undefined" means the handler returned undefined.
  std::string return_value_json;

  struct CallbackCall {
    // Index into the ordered list of function arguments of the JSB call.
    size_t index = 0;
    // JSON serialization of the value passed to the callback.
    std::string value_json;
    // Recorded request->callback delay in milliseconds (0 = immediate).
    int64_t delay_ms = 0;
  };

  // Callback invocations the handler requested, in the order it made them.
  std::vector<CallbackCall> callbacks;
};

// Owns a dedicated QuickJS runtime that evaluates a fixture.js script and keeps
// the registered ctx.register() handlers alive for the whole replay session.
//
// Threading: the runtime is created lazily on the first call to Initialize and
// must only be used from the thread that created it (QuickJS captures the
// creating thread's stack pointer for its stack-overflow guard). In the replay
// pipeline this is the Lynx_JS thread, which is also where Dispatch is invoked.
class FixtureContext {
 public:
  FixtureContext();
  ~FixtureContext();

  FixtureContext(const FixtureContext&) = delete;
  FixtureContext& operator=(const FixtureContext&) = delete;

  // Creates the runtime, registers the ctx surface, and evaluates
  // <fixture_dir>/fixture.js so that all ctx.register() handlers are recorded.
  // Returns true when the fixture script loaded and at least the ctx object was
  // installed. Failures are logged before returning false.
  bool Initialize(const std::string& fixture_dir,
                  const FixtureContextLimits& limits = FixtureContextLimits{});

  void Destroy();

  bool initialized() const { return initialized_; }

  // Returns true when a handler is registered for (module, method).
  bool HasHandler(const std::string& module, const std::string& method) const;

  // Invokes the registered handler for (module, method).
  //   args_json: JSON array string with null placeholders for functions.
  //   callback_positions: positions of function arguments in that array.
  // The returned result carries the handler return value and the ordered list
  // of callback invocations the handler performed.
  FixtureDispatchResult Dispatch(const std::string& module,
                                 const std::string& method,
                                 const std::string& args_json,
                                 const std::vector<size_t>& callback_positions);

 private:
  void RegisterCtxObject();
  bool LoadFixtureScript();
  std::string ReadAssetFile(const std::string& path);

  // QuickJS interrupt handler (checked in JS loops/backward jumps). Arms the
  // per-execution deadline in interrupt_deadline_; once tripped the current
  // JS execution is aborted.
  static int InterruptHandler(LEPUSRuntime* runtime, void* opaque);

  std::unique_ptr<Runtime> runtime_;
  std::shared_ptr<VMInstance> vm_;
  std::shared_ptr<JSIContext> js_context_;

  std::string fixture_dir_;
  FixtureContextLimits limits_;
  bool initialized_ = false;

  // Interrupt state read/written by InterruptHandler on the runtime thread.
  std::chrono::steady_clock::time_point interrupt_deadline_ =
      std::chrono::steady_clock::time_point::max();
  bool interrupt_timed_out_ = false;

  // key = "module.method"
  std::unordered_map<std::string, Function> handler_map_;
  std::unordered_map<std::string, Object> handler_state_map_;
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_FIXTURE_CONTEXT_H_
