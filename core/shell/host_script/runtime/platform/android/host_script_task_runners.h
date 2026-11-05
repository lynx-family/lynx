// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_TASK_RUNNERS_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_TASK_RUNNERS_H_

#include <memory>

#include "base/include/fml/task_runner.h"
#include "core/base/lynx_export.h"

namespace lynx {
namespace runtime {
namespace js {
class JSExecutor;
}
}  // namespace runtime
namespace shell {

struct HostScriptRuntimeCallbacks;

struct HostScriptExecutorFactory {
  runtime::js::JSExecutor* (*create)();
  void (*destroy)(runtime::js::JSExecutor*);
};
using HostScriptExecutor = std::unique_ptr<runtime::js::JSExecutor,
                                           void (*)(runtime::js::JSExecutor*)>;

// Arguments are borrowed on the calling thread. Outputs point to caller-
// constructed, same-version objects; no argument is retained after the call.
struct HostScriptEngineOps {
  void (*js_to_lepus)(void* runtime, const void* value, void* output);
  void (*pub_to_lepus)(const void* value, void* output);
  void (*lepus_to_js)(void* runtime, const void* value, void* output);
  void (*callback_thread_switch_start)(void* collector);
};

// Internal, same-version contract. These runners remain owned by the engine.
struct HostScriptTaskRunners {
  fml::RefPtr<fml::TaskRunner> bts;
  fml::RefPtr<fml::TaskRunner> mts;
  fml::RefPtr<fml::TaskRunner> ui;
  bool (*is_debug_enabled)();
  const HostScriptExecutorFactory* executor_factory;
  const HostScriptEngineOps* engine_ops;
};

struct HostScriptEngine {
  bool (*set_provider)(const HostScriptRuntimeCallbacks*, bool);
  // Android UI only, after environment initialization.
  // Does not initialize a script runtime or install observers.
  bool (*get_runners)(HostScriptTaskRunners*);
};

// The same-version engine exposes one bridge to the optional runtime.
LYNX_EXPORT const HostScriptEngine* GetHostScriptEngine();

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_TASK_RUNNERS_H_
