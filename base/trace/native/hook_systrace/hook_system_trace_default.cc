// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/hook_systrace/hook_system_trace.h"

namespace lynx {
namespace trace {

void HookSystemTrace::InstallSystemTraceHooks() {}

void HookSystemTrace::UninstallSystemTraceHooks() {}

void HookSystemTrace::Install(const SetupConfig& config) {
  if (config.cpu_trace_enabled) {
    cpu_info_trace_.DispatchBegin();
  }
  if (config.memory_info_trace_enabled) {
    memory_info_trace_.DispatchBegin();
  }
}

void HookSystemTrace::Uninstall() {
  cpu_info_trace_.DispatchEnd();
  memory_info_trace_.DispatchEnd();
}

}  // namespace trace
}  // namespace lynx
