// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"

#include <atomic>
#include <string>
#include <utility>

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/runtime/js/bindings/modules/module_interceptor.h"
#include "core/runtime/js/bindings/modules/module_interceptor_registry.h"
#include "core/runtime/js/js_executor.h"
#include "core/shell/host_script/runtime/platform/android/host_script_runtime_callbacks.h"
#include "core/shell/host_script/runtime/platform/android/host_script_task_runners.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "core/value_wrapper/value_impl_piper.h"

namespace lynx {
namespace shell {
namespace {
std::atomic<bool> ui_initialized{false};
std::atomic<bool> view_created{false};
// All accesses use atomic builtins to avoid an out-of-line atomic template.
const HostScriptRuntimeCallbacks* provider = nullptr;
const HostScriptRuntimeCallbacks* LoadProvider() {
  return __atomic_load_n(&provider, __ATOMIC_ACQUIRE);
}
bool IsDebugEnabled() {
  return tasm::DevToolLifecycle::GetInstance().IsEnabled();
}
const HostScriptExecutorFactory executor_factory{
    [] { return new runtime::js::JSExecutor("-1", nullptr, nullptr, true); },
    [](runtime::js::JSExecutor* executor) { delete executor; }};
const HostScriptEngineOps engine_ops{
    [](void* runtime_ptr, const void* value, void* output) {
      *static_cast<lepus::Value*>(output) =
          pub::ValueUtils::ConvertValueToLepusValue(pub::ValueImplPiper(
              *static_cast<runtime::js::Runtime*>(runtime_ptr),
              *static_cast<const runtime::js::Value*>(value)));
    },
    [](const void* value, void* output) {
      *static_cast<lepus::Value*>(output) =
          pub::ValueUtils::ConvertValueToLepusValue(
              *static_cast<const pub::Value*>(value));
    },
    [](void* runtime_ptr, const void* value, void* output) {
      *static_cast<runtime::js::Value*>(output) =
          pub::ValueUtils::ConvertValueToPiperValue(
              *static_cast<runtime::js::Runtime*>(runtime_ptr),
              pub::ValueImplLepus(*static_cast<const lepus::Value*>(value)));
    },
    [](void* collector) {
      static_cast<runtime::js::NativeModuleInfoCollector*>(collector)
          ->CallbackThreadSwitchStart();
    }};
std::unique_ptr<runtime::js::ModuleInterceptor> CreateModuleInterceptor() {
  const auto* callbacks = LoadProvider();
  return callbacks ? std::unique_ptr<runtime::js::ModuleInterceptor>(
                         callbacks->create_module_interceptor())
                   : nullptr;
}
}  // namespace

bool SetHostScriptRuntimeProvider(const HostScriptRuntimeCallbacks* callbacks,
                                  bool commit) {
  if (!callbacks || callbacks->abi_version != kHostScriptRuntimeAbiVersion ||
      callbacks->executor_features != kHostScriptExecutorFeatures ||
      !callbacks->prepare || !callbacks->update_debug_state ||
      !callbacks->create_module_interceptor || !callbacks->register_view ||
      !callbacks->destroy_view)
    return false;
  if (!commit) {
    const auto* current = LoadProvider();
    return !current || current == callbacks;
  }
  const HostScriptRuntimeCallbacks* expected = nullptr;
  if (!__atomic_compare_exchange_n(&provider, &expected, callbacks, false,
                                   __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE))
    return expected == callbacks;
  if (ui_initialized.load(std::memory_order_acquire))
    PrepareHostScriptRuntime();
  return true;
}

bool GetHostScriptTaskRunners(HostScriptTaskRunners* output) {
  if (!output || !LoadProvider() ||
      !ui_initialized.load(std::memory_order_acquire))
    return false;
  if (!base::UIThread::GetRunner()->RunsTasksOnCurrentThread()) return false;
  base::TaskRunnerManufactor runners(base::MOST_ON_TASM, false, false);
  *output = {runners.GetJSTaskRunner(), runners.GetTASMTaskRunner(),
             runners.GetUITaskRunner(), &IsDebugEnabled,
             &executor_factory,         &engine_ops};
  return true;
}

const HostScriptEngine* GetHostScriptEngine() {
  static const HostScriptEngine engine{&SetHostScriptRuntimeProvider,
                                       &GetHostScriptTaskRunners};
  return &engine;
}

void PrepareHostScriptRuntime() {
  ui_initialized.store(true, std::memory_order_release);
  const auto* callbacks = LoadProvider();
  if (!callbacks) return;
  runtime::js::ModuleInterceptorRegistry::RegisterLeading(
      &CreateModuleInterceptor);
  callbacks->prepare(view_created.load(std::memory_order_acquire));
}

void OnHostScriptViewCreated() {
  view_created.store(true, std::memory_order_release);
  const auto* callbacks = LoadProvider();
  if (!callbacks || !ui_initialized.load(std::memory_order_acquire)) return;
  callbacks->prepare(true);
}

void UpdateHostScriptDebugState(bool enabled) {
  const auto* callbacks = LoadProvider();
  if (!callbacks || !ui_initialized.load(std::memory_order_acquire)) return;
  callbacks->update_debug_state(enabled);
}

void RegisterHostScriptView(base::LynxEntityId view) {
  const auto* callbacks = LoadProvider();
  if (!callbacks) return;
  callbacks->register_view(view);
}

void UnregisterHostScriptView(base::LynxEntityId view) {
  const auto* callbacks = LoadProvider();
  if (!callbacks) return;
  callbacks->destroy_view(view);
}

}  // namespace shell
}  // namespace lynx
