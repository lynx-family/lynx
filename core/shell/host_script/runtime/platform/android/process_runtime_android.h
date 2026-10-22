// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_PROCESS_RUNTIME_ANDROID_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_PROCESS_RUNTIME_ANDROID_H_

#include <jni.h>

#include <string>

#include "base/include/closure.h"
#include "core/shell/host_script/runtime/platform/android/host_script_runtime_callbacks.h"
#include "core/shell/host_script/runtime/platform/android/host_script_task_runners.h"
#include "core/shell/host_script/runtime/process_runtime.h"

namespace Json {
class Value;
}
namespace lynx {
namespace devtool {
struct HSRScriptRequest;
}
namespace shell {

// Called on Android UI after Lynx initialization. Reuses the process runners;
// readiness is reported on each owner after its bootstrap has been installed.
LYNX_EXPORT_FOR_DEVTOOL bool InitializeHostScriptRuntime(
    ProcessRuntime::Completion ready, std::string bootstrap);

// The optional library executes requests on the engine-owned runners.
LYNX_EXPORT_FOR_DEVTOOL void EvaluateHostScriptRuntime(
    ProcessRuntime::Domain domain, std::string source, std::string url,
    ProcessRuntime::Completion completion);

LYNX_EXPORT_FOR_DEVTOOL void LoadHostScriptRuntime(
    std::string source, std::string url, ProcessRuntime::Completion completion,
    ProcessRuntime::Domain domain = ProcessRuntime::Domain::kBTS);

struct HostScriptTransport {
  using Callback = base::MoveOnlyClosure<void, Json::Value, const std::string&>;
  using Fetcher = void (*)(const std::string&, uint64_t);
  void (*handle)(devtool::HSRScriptRequest, Callback,
                 fml::RefPtr<fml::TaskRunner>, Fetcher);
  void (*fetched)(JNIEnv*, jlong, jbyteArray, jbyteArray,
                  fml::RefPtr<fml::TaskRunner>);
};

bool PublishHostScriptRuntimeCallbacks(
    JNIEnv* env, const HostScriptRuntimeCallbacks* callbacks, bool commit);
void PrepareStandaloneHostScriptRuntime(bool view_created);
void UpdateStandaloneHostScriptDebugState(bool enabled);
bool IsHostScriptDebugEnabled();
HostScriptExecutor CreateHostScriptExecutor();
const HostScriptTransport* GetStandaloneHostScriptTransport();

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_PROCESS_RUNTIME_ANDROID_H_
