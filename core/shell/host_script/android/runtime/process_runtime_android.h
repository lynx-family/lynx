// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_PROCESS_RUNTIME_ANDROID_H_
#define CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_PROCESS_RUNTIME_ANDROID_H_

#include <string>

#include "core/shell/host_script/runtime/process_runtime.h"

namespace lynx {
namespace shell {

// Called on Android UI after Lynx initialization. Reuses the process runners;
// readiness is reported on each owner after its bootstrap has been installed.
LYNX_EXPORT_FOR_DEVTOOL bool InitializeHostScriptRuntime(
    ProcessRuntime::Completion ready, std::string bootstrap);

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_PROCESS_RUNTIME_ANDROID_H_
