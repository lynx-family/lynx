// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HOST_SCRIPT_ANDROID_LYNX_VIEW_HOST_SCRIPT_VIEW_HOOKS_ANDROID_H_
#define CORE_SHELL_HOST_SCRIPT_ANDROID_LYNX_VIEW_HOST_SCRIPT_VIEW_HOOKS_ANDROID_H_

namespace lynx {
namespace shell {

// Enables or disables LynxView creation notifications on the UI thread.
void InstallHostScriptViewHooks();
void UninstallHostScriptViewHooks();

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HOST_SCRIPT_ANDROID_LYNX_VIEW_HOST_SCRIPT_VIEW_HOOKS_ANDROID_H_
