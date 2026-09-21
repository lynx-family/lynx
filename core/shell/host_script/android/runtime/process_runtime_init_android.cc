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
#include "core/shell/host_script/android/lynx_view/host_script_view_observer_android.h"
#include "core/shell/host_script/android/runtime/process_runtime_android.h"

namespace lynx {
namespace shell {
namespace {
std::atomic<bool> ui_initialized{false};
std::atomic<bool> view_created{false};
std::atomic<uint64_t> debug_epoch{0};
}  // namespace

void EvaluateHostScriptRuntime(ProcessRuntime::Domain domain,
                               std::string source, std::string url,
                               ProcessRuntime::Completion completion) {
  ProcessRuntime::GetInstance().Evaluate(domain, std::move(source),
                                         std::move(url), std::move(completion));
}

bool HasHostScriptRuntime() { return true; }

void ShutdownHostScriptRuntime(ProcessRuntime::Completion completion) {
  ProcessRuntime::GetInstance().Shutdown(std::move(completion));
}

bool PrepareHostScriptRuntime() {
  ui_initialized.store(true, std::memory_order_release);
  // Android LynxEnv.isLynxDebugEnabled() delegates to this same lifecycle.
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) return false;
  base::TaskRunnerManufactor runners(base::MOST_ON_TASM, false, false);
  DCHECK(runners.GetUITaskRunner()->RunsTasksOnCurrentThread());
  InstallHostScriptViewObserver();
  auto& runtime = ProcessRuntime::GetInstance();
  const bool prepared = runtime.Initialize(
      {runners.GetJSTaskRunner(), runners.GetTASMTaskRunner(),
       runners.GetUITaskRunner()},
      {}, {}, {}, ProcessRuntime::InitializationMode::kLazy);
  if (view_created.load(std::memory_order_acquire))
    runtime.InitializeBindings();
  return prepared;
}

void OnHostScriptViewCreated() {
  view_created.store(true, std::memory_order_release);
  if (!ui_initialized.load(std::memory_order_acquire) ||
      !tasm::DevToolLifecycle::GetInstance().IsEnabled())
    return;
  fml::TaskRunner::RunNowOrPostTask(base::UIThread::GetRunner(),
                                    [] { PrepareHostScriptRuntime(); });
}

uint64_t HostScriptDebugEpoch() {
  return debug_epoch.load(std::memory_order_acquire);
}

void UpdateHostScriptDebugState(bool enabled) {
  if (!enabled) debug_epoch.fetch_add(1, std::memory_order_acq_rel);
  // GetRunner can wait before UI setup. LynxEnv handles that initial case.
  if (!ui_initialized.load(std::memory_order_acquire)) return;
  auto ui = base::UIThread::GetRunner();
  auto prepare = [ui] { ui->PostTask([] { PrepareHostScriptRuntime(); }); };
  if (enabled) {
    prepare();
  } else {
    fml::TaskRunner::RunNowOrPostTask(
        ui, [] { UninstallHostScriptViewObserver(); });
    ProcessRuntime::GetInstance().Shutdown(
        [prepare](ProcessRuntime::Result) { prepare(); });
  }
}

}  // namespace shell
}  // namespace lynx
