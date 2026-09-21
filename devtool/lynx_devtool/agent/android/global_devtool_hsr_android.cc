// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/shell/host_script/android/runtime/process_runtime_android.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"

namespace lynx {
namespace devtool {
namespace {
using shell::ProcessRuntime;
using Callback = GlobalDevToolPlatformFacade::HSRScriptCallback;

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
  constexpr const char* kDomainNames[] = {"bts", "mts", "ui"};
  auto completion = std::make_shared<Callback>(std::move(callback));
  shell::EvaluateHostScriptRuntime(
      domain, request.source.empty() ? "void 0;" : std::move(request.source),
      std::string("host-script://cdp-") +
          kDomainNames[static_cast<size_t>(domain)] + ".js",
      [completion](ProcessRuntime::Result result) mutable {
        Json::Value response(Json::objectValue);
        if (result.success) {
          response["valueType"] = result.has_value ? "json" : "undefined";
          if (result.has_value &&
              !Json::Reader().parse(result.value_json, response["value"],
                                    false)) {
            result.error = "Cannot decode Host Script result JSON";
          }
        }
        if (*completion) {
          auto callback = std::move(*completion);
          std::move(callback)(std::move(response), result.error);
        }
      });
}
}  // namespace

void GlobalDevToolPlatformAndroid::HandleHSRScript(HSRScriptRequest request,
                                                   HSRScriptCallback callback) {
  if (!callback) return;
  if (!tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    std::move(callback)(Json::Value(), "HSR_DEBUG_DISABLED");
    return;
  }
  if (request.operation != HSRScriptRequest::Operation::kEvaluate) {
    std::move(callback)(Json::Value(), "HSR loadScript is not connected");
    return;
  }
  // LynxEnv prepares routing; Evaluate creates bindings on the selected owner.
  // The shared CDP responder posts the completed result back to DevTool.
  Evaluate(std::move(request), std::move(callback));
}
}  // namespace devtool
}  // namespace lynx
