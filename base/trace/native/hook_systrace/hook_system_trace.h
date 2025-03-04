// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_TRACE_NATIVE_HOOK_SYSTRACE_HOOK_SYSTEM_TRACE_H_
#define BASE_TRACE_NATIVE_HOOK_SYSTRACE_HOOK_SYSTEM_TRACE_H_

#include <memory>

#include "base/trace/native/hook_systrace/cpu_info_trace.h"

namespace lynx {
namespace trace {

class HookSystemTrace {
 public:
  HookSystemTrace() = default;
  ~HookSystemTrace() = default;

  void Install();

  void Uninstall();

 private:
  static void InstallSystemTraceHooks();
  static void UninstallSystemTraceHooks();
  CpuInfoTrace cpu_info_trace_;
};
}  // namespace trace
}  // namespace lynx
#endif  // BASE_TRACE_NATIVE_HOOK_SYSTRACE_HOOK_SYSTEM_TRACE_H_
