// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_VIEW_HOOKS_ANDROID_H_
#define CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_VIEW_HOOKS_ANDROID_H_

#include <string>

#include "base/include/fml/task_runner.h"

namespace lynx {
namespace shell {

// Returns the platform-owned local resource root for process Host Scripts.
std::string GetHostScriptResourceRoot();

// Lets LynxViews register as operable Views, independent of DevTool. Commands
// run on `ui_runner`, the platform UI thread.
void EnableHostScriptViewRegistry(fml::RefPtr<fml::TaskRunner> ui_runner);
// Enables or disables UI-domain interception on the UI thread.
void InstallHostScriptViewHooks();
void UninstallHostScriptViewHooks();

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_RUNTIME_PLATFORM_ANDROID_HOST_SCRIPT_VIEW_HOOKS_ANDROID_H_
