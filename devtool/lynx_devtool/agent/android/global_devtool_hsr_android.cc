// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "base/include/string/string_utils.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/android/runtime/process_runtime_android.h"
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
  uint64_t epoch = 0;
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

void FinishLoad(uint64_t id, std::string error) {
  auto& load = Load();
  if (load.id != id || !load.callback) return;
  if (load.epoch != shell::HostScriptDebugEpoch()) error = "HSR_DEBUG_DISABLED";
  auto callback = std::move(load.callback);
  load.fetching = false;
  load.url.clear();
  std::move(callback)(Json::Value(Json::objectValue), error);
}

void SourceLoaded(uint64_t id, std::string source, const std::string& error) {
  auto& load = Load();
  if (load.id != id || !load.callback || !load.fetching) return;
  load.fetching = false;
  if (!error.empty()) {
    FinishLoad(id, error);
    return;
  }
  shell::LoadHostScriptRuntime(
      std::move(source), load.url, load.epoch,
      [id](const ProcessRuntime::Result& result) {
        std::string error = result.error;
        if (!result.success && error.empty()) error = "HSR_LOAD_FAILED";
        DevToolRunner()->PostTask(
            [id, error = std::move(error)] { FinishLoad(id, error); });
      });
}

void StartLoad(HSRScriptRequest request, Callback callback, uint64_t epoch) {
  auto& load = Load();
  if (load.callback && load.epoch != epoch)
    FinishLoad(load.id, "HSR_DEBUG_DISABLED");
  if (load.callback) {
    std::move(callback)(Json::Value(), "HSR_LOAD_IN_PROGRESS");
    return;
  }
  const auto id = ++load.id;
  load.epoch = epoch;
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

void Evaluate(HSRScriptRequest request, Callback callback, uint64_t epoch) {
  const auto domain = RuntimeDomain(request.thread);
  constexpr const char* kSourceUrls[] = {"host-script://cdp-bts.js",
                                         "host-script://cdp-mts.js",
                                         "host-script://cdp-ui.js"};
  auto completion = std::make_shared<Callback>(std::move(callback));
  shell::EvaluateHostScriptRuntime(
      domain, request.source.empty() ? "void 0;" : std::move(request.source),
      kSourceUrls[static_cast<size_t>(domain)], epoch,
      [completion](const ProcessRuntime::Result& result) mutable {
        std::string error = result.error;
        Json::Value response(Json::objectValue);
        if (result.success) {
          response["valueType"] = result.has_value ? "json" : "undefined";
          if (result.has_value &&
              !Json::Reader().parse(result.value_json, response["value"],
                                    false)) {
            error = "Cannot decode Host Script result JSON";
          }
        }
        if (*completion) {
          auto callback = std::move(*completion);
          std::move(callback)(std::move(response), error);
        }
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
        if (epoch != shell::HostScriptDebugEpoch() ||
            !tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
          std::move(callback)(Json::Value(), "HSR_DEBUG_DISABLED");
        } else if (request.operation ==
                   HSRScriptRequest::Operation::kLoadScript) {
          StartLoad(std::move(request), std::move(callback), epoch);
        } else {
          Evaluate(std::move(request), std::move(callback), epoch);
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
