// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"

#include <atomic>
#include <string>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/runtime/js/js_executor.h"
#include "core/shell/host_script/runtime/host_script_module_interceptor.h"
#include "core/shell/host_script/runtime/interceptor.h"
#include "core/shell/host_script/runtime/platform/android/host_script_runtime_callbacks.h"
#include "core/shell/host_script/runtime/platform/android/host_script_task_runners.h"

namespace lynx {
namespace shell {
namespace {
std::atomic<bool> ui_initialized{false};
std::atomic<bool> view_created{false};
std::atomic<const HostScriptRuntimeCallbacks*> provider{nullptr};
bool IsDebugEnabled() {
  return tasm::DevToolLifecycle::GetInstance().IsEnabled();
}
const HostScriptExecutorFactory executor_factory{
    [] { return new runtime::js::JSExecutor("-1", nullptr, nullptr, true); },
    [](runtime::js::JSExecutor* executor) { delete executor; }};
}  // namespace

bool SetHostScriptRuntimeProvider(const HostScriptRuntimeCallbacks* callbacks,
                                  bool commit) {
  if (!callbacks || callbacks->abi_version != kHostScriptRuntimeAbiVersion ||
      callbacks->executor_features != kHostScriptExecutorFeatures ||
      !callbacks->prepare || !callbacks->update_debug_state)
    return false;
  if (!commit) {
    const auto* current = provider.load(std::memory_order_acquire);
    return !current || current == callbacks;
  }
  const HostScriptRuntimeCallbacks* expected = nullptr;
  if (!provider.compare_exchange_strong(expected, callbacks,
                                        std::memory_order_acq_rel))
    return expected == callbacks;
  if (ui_initialized.load(std::memory_order_acquire))
    PrepareHostScriptRuntime();
  return true;
}

bool GetHostScriptTaskRunners(HostScriptTaskRunners* output) {
  if (!output || !provider.load(std::memory_order_acquire) ||
      !ui_initialized.load(std::memory_order_acquire))
    return false;
  if (!base::UIThread::GetRunner()->RunsTasksOnCurrentThread()) return false;
  base::TaskRunnerManufactor runners(base::MOST_ON_TASM, false, false);
  *output = {runners.GetJSTaskRunner(), runners.GetTASMTaskRunner(),
             runners.GetUITaskRunner(), &IsDebugEnabled, &executor_factory};
  return true;
}

void PrepareHostScriptRuntime() {
  ui_initialized.store(true, std::memory_order_release);
  const auto* callbacks = provider.load(std::memory_order_acquire);
  if (!callbacks) return;
  HostScriptModuleInterceptor::Register();
  callbacks->prepare(view_created.load(std::memory_order_acquire));
}

void OnHostScriptViewCreated() {
  view_created.store(true, std::memory_order_release);
  if (!provider.load(std::memory_order_acquire) ||
      !ui_initialized.load(std::memory_order_acquire))
    return;
  fml::TaskRunner::RunNowOrPostTask(base::UIThread::GetRunner(),
                                    [] { PrepareHostScriptRuntime(); });
}

void UpdateHostScriptDebugState(bool enabled) {
  const auto* callbacks = provider.load(std::memory_order_acquire);
  if (!callbacks || !ui_initialized.load(std::memory_order_acquire)) return;
  callbacks->update_debug_state(enabled);
}

void RegisterHostScriptView(base::LynxEntityId view) {
  if (!provider.load(std::memory_order_acquire)) return;
  Interceptor::RegisterView(view);
}

void UnregisterHostScriptView(base::LynxEntityId view) {
  if (!provider.load(std::memory_order_acquire)) return;
  Interceptor::DestroyView(view);
}

}  // namespace shell
}  // namespace lynx
