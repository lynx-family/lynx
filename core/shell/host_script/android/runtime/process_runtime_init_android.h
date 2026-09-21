// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_PROCESS_RUNTIME_INIT_ANDROID_H_
#define CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_PROCESS_RUNTIME_INIT_ANDROID_H_

namespace lynx {
namespace shell {

// Called by LynxEnv on Android UI. Prepares fixed process routing without
// creating JS contexts or bindings until a View is created. Repeated calls
// leave the generation intact.
void PrepareHostScriptRuntime();

// Reuses the native View renderer creation path. Initializes each domain's
// bindings on its owner, including after debugging is enabled later.
void OnHostScriptViewCreated();

// Reuses the existing DevTool lifecycle notification, including late enabling.
// Disabling closes admission immediately; disposal stays on each owner.
void UpdateHostScriptDebugState(bool enabled);

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_ANDROID_RUNTIME_PROCESS_RUNTIME_INIT_ANDROID_H_
