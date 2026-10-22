// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/runtime/platform/android/process_runtime_android.h"

#include <atomic>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"
#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"
#include "core/shell/host_script/runtime/platform/android/host_script_task_runners.h"
#include "core/shell/host_script/runtime/platform/android/host_script_view_hooks_android.h"

namespace lynx {
namespace shell {
namespace {
std::atomic<bool> ui_initialized{false};
std::atomic<bool> view_created{false};
std::atomic<bool (*)()> debug_enabled_reader{nullptr};
std::atomic<const HostScriptExecutorFactory*> executor_factory{nullptr};
// UI setup is published once and stays valid for the process lifetime.
bool CheckUIInitialized(const ProcessRuntime::Completion& completion) {
  if (ui_initialized.load(std::memory_order_acquire)) return true;
  ProcessRuntime::Result result;
  result.error = "RUNTIME_NOT_RUNNING";
  if (completion) completion(std::move(result));
  return false;
}
}  // namespace

bool IsHostScriptDebugEnabled() {
  auto reader = debug_enabled_reader.load(std::memory_order_acquire);
  return reader && reader();
}

HostScriptExecutor CreateHostScriptExecutor() {
  const auto* factory = executor_factory.load(std::memory_order_acquire);
  if (!factory || !factory->create || !factory->destroy)
    return {nullptr, nullptr};
  return {factory->create(), factory->destroy};
}

bool InitializeHostScriptRuntime(ProcessRuntime::Completion ready,
                                 std::string bootstrap) {
  HostScriptTaskRunners runners;
  if (!GetHostScriptTaskRunners(&runners)) return false;
  executor_factory.store(runners.executor_factory, std::memory_order_release);
  debug_enabled_reader.store(runners.is_debug_enabled,
                             std::memory_order_release);
  return ProcessRuntime::GetInstance().Initialize(
      {runners.bts, runners.mts, runners.ui}, std::move(ready), {},
      std::move(bootstrap));
}

void LoadHostScriptRuntime(std::string source, std::string url,
                           ProcessRuntime::Completion completion,
                           ProcessRuntime::Domain domain) {
  // GetRunner waits until UI setup; reject before asking for that runner.
  if (!CheckUIInitialized(completion)) return;
  fml::TaskRunner::RunNowOrPostTask(
      base::UIThread::GetRunner(),
      [domain, source = std::move(source), url = std::move(url),
       completion = std::move(completion)]() mutable {
        PrepareStandaloneHostScriptRuntime(false);
        ProcessRuntime::GetInstance().LoadScript(
            std::move(source), std::move(url), std::move(completion), domain);
      });
}

void EvaluateHostScriptRuntime(ProcessRuntime::Domain domain,
                               std::string source, std::string url,
                               ProcessRuntime::Completion completion) {
  if (!CheckUIInitialized(completion)) return;
  ProcessRuntime::GetInstance().Evaluate(domain, std::move(source),
                                         std::move(url), std::move(completion));
}

void PrepareStandaloneHostScriptRuntime(bool had_view) {
  ui_initialized.store(true, std::memory_order_release);
  if (had_view) view_created.store(true, std::memory_order_release);
  HostScriptTaskRunners runners;
  if (!GetHostScriptTaskRunners(&runners)) return;
  executor_factory.store(runners.executor_factory, std::memory_order_release);
  debug_enabled_reader.store(runners.is_debug_enabled,
                             std::memory_order_release);
  DCHECK(runners.ui->RunsTasksOnCurrentThread());
  InstallHostScriptViewHooks();
  auto& runtime = ProcessRuntime::GetInstance();
  runtime.Initialize({runners.bts, runners.mts, runners.ui}, {},
                     &HostScriptInterceptorBindings::Create, {},
                     ProcessRuntime::InitializationMode::kLazy);
  if (view_created.load(std::memory_order_acquire))
    runtime.InitializeBindings();
}

void UpdateStandaloneHostScriptDebugState(bool) {
  if (!ui_initialized.load(std::memory_order_acquire)) return;
  fml::TaskRunner::RunNowOrPostTask(base::UIThread::GetRunner(), [] {
    if (IsHostScriptDebugEnabled()) {
      PrepareStandaloneHostScriptRuntime(false);
    } else {
      UninstallHostScriptViewHooks();
    }
  });
}

}  // namespace shell
}  // namespace lynx
