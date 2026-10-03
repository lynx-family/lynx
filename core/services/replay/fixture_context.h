// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_FIXTURE_CONTEXT_H_
#define CORE_SERVICES_REPLAY_FIXTURE_CONTEXT_H_

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/services/replay/fixture_js_runtime.h"

namespace lynx {
namespace tasm {
namespace replay {

struct FixtureContextLimits : FixtureRuntimeLimits {
  size_t max_script_bytes = kDefaultFixtureMaxScriptBytes;
  size_t max_asset_bytes = kDefaultFixtureMaxAssetBytes;
  size_t max_handlers = 10000;
  // Module/method keys live in native memory, outside the engine heap budget.
  size_t max_handler_name_bytes = 16 * 1024 * 1024;
  size_t max_callbacks = 10000;
  size_t max_result_bytes = 16 * 1024 * 1024;
};

struct FixtureDispatchResult {
  // False for an absent handler, invalid input or failed execution. Failed
  // executions return no partial callback results. Script state is not rolled
  // back. A registered handler that returns undefined is still handled.
  bool handled = false;
  // Empty means undefined or a value that cannot be serialized as JSON.
  std::string return_value_json;

  struct CallbackCall {
    // Index in the ordered callback list, not the original argument array.
    size_t index = 0;
    std::string value_json;
    int64_t delay_ms = 0;
  };
  std::vector<CallbackCall> callbacks;
};

// Keeps fixture handlers and their closure/this state alive across calls.
// Initialize, Dispatch and Destroy (including destruction) must run on the
// same thread, normally the Lynx JS thread. Each instance owns its runtime;
// arguments and results cross to the business runtime through JSON only.
// Lifecycle actions and shared data are handled by EvaluateFixture. Their
// DSL methods are no-ops here, including after (it does not run its task).
class FixtureContext {
 public:
  FixtureContext();
  ~FixtureContext();
  FixtureContext(const FixtureContext&) = delete;
  FixtureContext& operator=(const FixtureContext&) = delete;

  // Starts a fresh session, replacing any prior state. On failure the context
  // is uninitialized. The directory has the same extraction preconditions as
  // ReadFixtureAsset. Runtime limits must be positive; zero byte/count budgets
  // reject the corresponding input/output rather than disabling a limit.
  bool Initialize(const std::string& fixture_directory,
                  const FixtureContextLimits& limits = FixtureContextLimits{});
  void Destroy();
  bool initialized() const { return initialized_; }
  bool HasHandler(const std::string& module, const std::string& method) const;

  // args_json is a JSON array with null in place of function arguments.
  // callback_positions lists those positions in their original order. The
  // handler receives (args, callbacks), with function type placeholders in
  // args and callbacks[i](value, delayMs) collecting replay intents. The caller
  // schedules them in the business runtime. Proxies are valid during this
  // synchronous dispatch only; no event loop or deferred tasks are provided.
  // Each handler has a persistent this object, initially {n: 0}; the script
  // owns its state updates, including any use of n as a call counter.
  FixtureDispatchResult Dispatch(const std::string& module,
                                 const std::string& method,
                                 const std::string& args_json,
                                 const std::vector<size_t>& callback_positions);

 private:
  struct Handler;
  struct CallbackSink;
  bool InitializeCallbackFactory();
  void RegisterCtxObject(const std::string& fixture_directory);

  // Clear all retained JSI values before destroying their runtime.
  std::unique_ptr<FixtureJsRuntime> runtime_;
  std::map<std::pair<std::string, std::string>, std::shared_ptr<Handler>>
      handlers_;
  std::unique_ptr<runtime::js::Function> callback_factory_;
  std::weak_ptr<CallbackSink> active_sink_;
  FixtureContextLimits limits_;
  size_t handler_name_bytes_ = 0;
  bool initialized_ = false;
  bool registration_failed_ = false;
};

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_FIXTURE_CONTEXT_H_
