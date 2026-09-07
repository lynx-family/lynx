// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_JS_DISPATCHER_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_JS_DISPATCHER_H_

#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>

#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace shell {

// Owns the only thread-safe dispatch channel for one Host Script JS runtime.
class HostScriptJsDispatcher final {
 public:
  using Task = std::function<void(Napi::Env)>;

  static std::shared_ptr<HostScriptJsDispatcher> Create(napi_env env);

  ~HostScriptJsDispatcher();

  HostScriptJsDispatcher(const HostScriptJsDispatcher&) = delete;
  HostScriptJsDispatcher& operator=(const HostScriptJsDispatcher&) = delete;

  bool Post(Task task);
  void Detach();
  bool IsAttached() const;

 private:
  static void InvokeTask(Napi::Env env, std::nullptr_t*, Task task);
  static void FinalizeDispatcher(Napi::Env, std::nullptr_t*, std::nullptr_t*);

  using RuntimeDispatcher =
      Napi::ThreadSafeFunction<std::nullptr_t, Task, InvokeTask>;

  explicit HostScriptJsDispatcher(
      std::unique_ptr<RuntimeDispatcher> dispatcher);

  mutable std::mutex mutex_;
  std::unique_ptr<RuntimeDispatcher> dispatcher_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_HOST_SCRIPT_JS_DISPATCHER_H_
