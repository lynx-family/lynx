// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"

#include <atomic>
#include <string>
#include <utility>

#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/android/lynx_view/host_script_view_hooks_android.h"
#include "core/shell/host_script/android/runtime/process_runtime_android.h"
#include "core/shell/host_script/runtime/host_script_interceptor_bindings.h"
#include "core/shell/host_script/runtime/host_script_module_interceptor.h"
#include "core/shell/host_script/runtime/interceptor.h"

namespace lynx {
namespace shell {
namespace {
std::atomic<bool> ui_initialized{false};
std::atomic<bool> view_created{false};
// UI setup is published once and stays valid for the process lifetime.
bool CheckUIInitialized(const ProcessRuntime::Completion& completion) {
  if (ui_initialized.load(std::memory_order_acquire)) return true;
  ProcessRuntime::Result result;
  result.error = "RUNTIME_NOT_RUNNING";
  if (completion) completion(std::move(result));
  return false;
}
}  // namespace

void LoadHostScriptRuntime(std::string source, std::string url,
                           ProcessRuntime::Completion completion) {
  // GetRunner waits until UI setup; reject before asking for that runner.
  if (!CheckUIInitialized(completion)) return;
  fml::TaskRunner::RunNowOrPostTask(
      base::UIThread::GetRunner(),
      [source = std::move(source), url = std::move(url),
       completion = std::move(completion)]() mutable {
        PrepareHostScriptRuntime();
        ProcessRuntime::GetInstance().LoadScript(
            std::move(source), std::move(url), std::move(completion));
      });
}

void EvaluateHostScriptRuntime(ProcessRuntime::Domain domain,
                               std::string source, std::string url,
                               ProcessRuntime::Completion completion) {
  if (!CheckUIInitialized(completion)) return;
  ProcessRuntime::GetInstance().Evaluate(domain, std::move(source),
                                         std::move(url), std::move(completion));
}

void PrepareHostScriptRuntime() {
  ui_initialized.store(true, std::memory_order_release);
  // Precedes every JS runtime, so DevTool may be enabled later. Each call
  // checks admission first.
  HostScriptModuleInterceptor::Register();
  base::TaskRunnerManufactor runners(base::MOST_ON_TASM, false, false);
  DCHECK(runners.GetUITaskRunner()->RunsTasksOnCurrentThread());
  InstallHostScriptViewHooks();
  auto& runtime = ProcessRuntime::GetInstance();
  runtime.Initialize({runners.GetJSTaskRunner(), runners.GetTASMTaskRunner(),
                      runners.GetUITaskRunner()},
                     {}, &HostScriptInterceptorBindings::Create, {},
                     ProcessRuntime::InitializationMode::kLazy);
  if (view_created.load(std::memory_order_acquire))
    runtime.InitializeBindings();
}

void OnHostScriptViewCreated() {
  view_created.store(true, std::memory_order_release);
  if (!ui_initialized.load(std::memory_order_acquire)) return;
  fml::TaskRunner::RunNowOrPostTask(base::UIThread::GetRunner(),
                                    [] { PrepareHostScriptRuntime(); });
}

void RegisterHostScriptView(base::LynxEntityId view) {
  Interceptor::RegisterView(view);
}

void UnregisterHostScriptView(base::LynxEntityId view) {
  Interceptor::DestroyView(view);
}

void UpdateHostScriptDebugState(bool) {
  if (!ui_initialized.load(std::memory_order_acquire)) return;
  fml::TaskRunner::RunNowOrPostTask(base::UIThread::GetRunner(), [] {
    if (tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
      PrepareHostScriptRuntime();
    } else {
      UninstallHostScriptViewHooks();
    }
  });
}

}  // namespace shell
}  // namespace lynx
