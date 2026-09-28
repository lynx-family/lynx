// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/string/string_utils.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/android/runtime/process_runtime_android.h"
#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

namespace lynx {
namespace devtool {
namespace {
using shell::ProcessRuntime;
using Callback = GlobalDevToolPlatformFacade::HSRScriptCallback;

// Accessed only on the DevTool runner. IDs make late resource callbacks safe
// after timeout, without passing native callback pointers through Java.
struct PendingLoad {
  uint64_t id = 0;
  uint64_t debug_epoch = 0;
  std::shared_ptr<std::atomic<bool>> cancelled;
  bool fetching = false;
  std::string url;
  Callback callback;
};

PendingLoad& Load() {
  static base::NoDestructor<PendingLoad> load;
  return *load;
}

auto DevToolRunner() {
  return LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
}

// Control state and pending responses stay on the existing DevTool runner.
struct RuntimeControl {
  bool loaded = false;
  bool stopped = false;
  bool stopping = false;
  uint64_t loaded_epoch = 0;
  // Only this cancellation token is read from execution owners.
  std::atomic<uint64_t> stop_epoch{0};
  size_t evaluations = 0;
  std::vector<Callback> stop_callbacks;
};

RuntimeControl& Control() {
  static base::NoDestructor<RuntimeControl> control;
  return *control;
}

Json::Value Status() {
  Json::Value result(Json::objectValue);
  const auto& control = Control();
  result["available"] = shell::HasHostScriptRuntime();
  result["enabled"] = tasm::DevToolLifecycle::GetInstance().IsEnabled();
  for (auto domain :
       {ProcessRuntime::Domain::kBTS, ProcessRuntime::Domain::kMTS,
        ProcessRuntime::Domain::kUI}) {
    constexpr const char* names[] = {"bts", "mts", "ui"};
    result["ready"][names[static_cast<size_t>(domain)]] =
        shell::IsHostScriptRuntimeReady(domain);
  }
  result["loaded"] =
      control.loaded && control.loaded_epoch == shell::HostScriptDebugEpoch();
  result["stopping"] = control.stopping;
  result["pending"] = static_cast<Json::UInt64>(control.evaluations +
                                                (Load().callback ? 1 : 0));
  return result;
}

void FinishLoad(uint64_t id, std::string error) {
  auto& load = Load();
  if (load.id != id || !load.callback) return;
  if (load.debug_epoch != shell::HostScriptDebugEpoch())
    error = "HSR_DEBUG_DISABLED";
  if (error.empty()) {
    Control().loaded = true;
    Control().stopped = false;
    Control().loaded_epoch = load.debug_epoch;
  }
  // UI may still hold a source task after this request has been cancelled.
  load.cancelled->store(true, std::memory_order_release);
  auto callback = std::move(load.callback);
  load.fetching = false;
  load.url.clear();
  std::move(callback)(Json::Value(Json::objectValue), error);
}

void SourceLoaded(uint64_t id, std::string source, const std::string& error) {
  auto& load = Load();
  if (load.id != id || !load.callback || !load.fetching) return;
  load.fetching = false;
  if (load.debug_epoch != shell::HostScriptDebugEpoch()) {
    FinishLoad(id, "HSR_DEBUG_DISABLED");
    return;
  }
  if (!error.empty()) {
    FinishLoad(id, error);
    return;
  }
  Control().loaded = false;
  // Restart and stop share UI ordering. No DevTool-owned state is read on UI.
  base::UIThread::GetRunner()->PostTask(
      [id, source = std::move(source), url = load.url, epoch = load.debug_epoch,
       restart = Control().stopped, cancelled = load.cancelled]() mutable {
        if (cancelled->load(std::memory_order_acquire)) return;
        if (epoch != shell::HostScriptDebugEpoch()) {
          DevToolRunner()->PostTask(
              [id] { FinishLoad(id, "HSR_DEBUG_DISABLED"); });
          return;
        }
        if (restart) shell::PrepareHostScriptRuntime();
        shell::LoadHostScriptRuntime(
            std::move(source), std::move(url),
            [id](const ProcessRuntime::Result& result) {
              std::string error = result.error;
              if (!result.success && error.empty()) error = "HSR_LOAD_FAILED";
              DevToolRunner()->PostTask(
                  [id, error = std::move(error)] { FinishLoad(id, error); });
            });
      });
}

void StartLoad(HSRScriptRequest request, Callback callback) {
  auto& load = Load();
  if (load.callback) {
    std::move(callback)(Json::Value(), "HSR_LOAD_IN_PROGRESS");
    return;
  }
  const auto id = ++load.id;
  load.cancelled = std::make_shared<std::atomic<bool>>(false);
  load.debug_epoch = shell::HostScriptDebugEpoch();
  load.fetching = true;
  load.callback = std::move(callback);
  load.url = request.source_type == HSRScriptRequest::SourceType::kUrl
                 ? request.source
                 : "host-script://cdp-main.js";
  if (request.source_type == HSRScriptRequest::SourceType::kInline) {
    SourceLoaded(id, std::move(request.source), "");
    return;
  }
  if (!base::IsValidUtf8(reinterpret_cast<const uint8_t*>(load.url.data()),
                         load.url.size())) {
    FinishLoad(id, "INVALID_SCRIPT_URL");
    return;
  }
  // A broken resource provider must not retain the global load slot forever.
  // This only expires fetching; it never cancels executing JavaScript.
  DevToolRunner()->PostDelayedTask(
      [id] {
        if (Load().id == id && Load().fetching)
          FinishLoad(id, "HSR_SOURCE_LOAD_TIMEOUT");
      },
      fml::TimeDelta::FromSeconds(30));
  GlobalDevToolPlatformAndroid::FetchHSRScript(load.url, id);
}

ProcessRuntime::Domain RuntimeDomain(HSRScriptRequest::Thread thread) {
  switch (thread) {
    case HSRScriptRequest::Thread::kBTS:
      return ProcessRuntime::Domain::kBTS;
    case HSRScriptRequest::Thread::kMTS:
      return ProcessRuntime::Domain::kMTS;
    case HSRScriptRequest::Thread::kUI:
      return ProcessRuntime::Domain::kUI;
  }
  return ProcessRuntime::Domain::kBTS;
}

void Evaluate(HSRScriptRequest request, Callback callback) {
  const auto domain = RuntimeDomain(request.thread);
  constexpr const char* kSourceUrls[] = {"host-script://cdp-bts.js",
                                         "host-script://cdp-mts.js",
                                         "host-script://cdp-ui.js"};
  ++Control().evaluations;
  const auto stop_epoch = Control().stop_epoch.load(std::memory_order_acquire);
  auto completion = std::make_shared<Callback>(std::move(callback));
  shell::EvaluateHostScriptRuntime(
      domain, request.source.empty() ? "void 0;" : std::move(request.source),
      kSourceUrls[static_cast<size_t>(domain)],
      [completion, stop_epoch](ProcessRuntime::Result result) {
        DevToolRunner()->PostTask([completion, stop_epoch,
                                   result = std::move(result)]() mutable {
          if (!*completion) return;
          --Control().evaluations;
          std::string error =
              stop_epoch == Control().stop_epoch ? result.error : "HSR_STOPPED";
          Json::Value response(Json::objectValue);
          if (result.success && error.empty()) {
            response["valueType"] = result.has_value ? "json" : "undefined";
            if (result.has_value &&
                !Json::Reader().parse(result.value_json, response["value"],
                                      false)) {
              error = "Cannot decode Host Script result JSON";
            }
          }
          auto callback = std::move(*completion);
          std::move(callback)(std::move(response), error);
        });
      },
      [stop_epoch] {
        return stop_epoch ==
               Control().stop_epoch.load(std::memory_order_acquire);
      });
}

void Stop(Callback callback) {
  auto& control = Control();
  control.stop_callbacks.push_back(std::move(callback));
  if (control.stopping) return;
  control.stopping = true;
  control.stopped = true;
  control.loaded = false;
  control.stop_epoch.fetch_add(1, std::memory_order_acq_rel);
  if (Load().callback) FinishLoad(Load().id, "HSR_STOPPED");
  // Serialize teardown behind any source already posted to UI for execution.
  base::UIThread::GetRunner()->PostTask([] {
    shell::ShutdownHostScriptRuntime([](ProcessRuntime::Result result) {
      DevToolRunner()->PostTask([error = std::move(result.error)] {
        auto& control = Control();
        control.stopping = false;
        auto callbacks = std::move(control.stop_callbacks);
        control.stop_callbacks.clear();
        for (auto& callback : callbacks)
          std::move(callback)(Json::Value(Json::objectValue), error);
      });
    });
  });
}
}  // namespace

void GlobalDevToolPlatformAndroid::HandleHSRScript(HSRScriptRequest request,
                                                   HSRScriptCallback callback) {
  if (!callback) return;
  const auto epoch = shell::HostScriptDebugEpoch();
  // Schema callers may arrive from a platform thread. CDP already runs here.
  fml::TaskRunner::RunNowOrPostTask(
      DevToolRunner(), [request = std::move(request),
                        callback = std::move(callback), epoch]() mutable {
        using Operation = HSRScriptRequest::Operation;
        if (request.operation == Operation::kGetStatus) {
          std::move(callback)(Status(), "");
        } else if (!shell::HasHostScriptRuntime()) {
          std::move(callback)(Json::Value(), "HSR_DEBUG_LIBRARY_REQUIRED");
        } else if (!shell::IsHostScriptUIInitialized()) {
          // GetRunner would block the DevTool thread before UI publication.
          std::move(callback)(Json::Value(), "RUNTIME_NOT_RUNNING");
        } else if (request.operation == Operation::kStop) {
          Stop(std::move(callback));
        } else if (epoch != shell::HostScriptDebugEpoch() ||
                   !tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
          std::move(callback)(Json::Value(), "HSR_DEBUG_DISABLED");
        } else if (Control().stopping) {
          std::move(callback)(Json::Value(), "HSR_STOPPING");
        } else if (request.operation == Operation::kLoadScript) {
          StartLoad(std::move(request), std::move(callback));
        } else if (Control().stopped) {
          std::move(callback)(Json::Value(), "HSR_STOPPED");
        } else {
          Evaluate(std::move(request), std::move(callback));
        }
      });
}

void GlobalDevToolPlatformAndroid::OnHSRScriptFetched(
    JNIEnv* env, jlong request_id, jbyteArray bytes, jbyteArray error_message) {
  using base::android::JNIConvertHelper;
  std::string error;
  if (error_message) {
    error = JNIConvertHelper::ConvertToString(env, error_message);
  }
  std::string source;
  if (error.empty()) {
    if (!bytes) {
      error = "HSR_SOURCE_LOAD_FAILED";
    } else if (env->GetArrayLength(bytes) > 512 * 1024) {
      error = "INVALID_SCRIPT_SOURCE";
    } else {
      source = JNIConvertHelper::ConvertToString(env, bytes);
    }
  }
  DevToolRunner()->PostTask([request_id, source = std::move(source),
                             error = std::move(error)]() mutable {
    SourceLoaded(request_id, std::move(source), error);
  });
}
}  // namespace devtool
}  // namespace lynx
