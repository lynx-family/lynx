// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/shell/host_script/android/runtime/process_runtime_android.h"
#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"

namespace lynx {
namespace shell {

bool PrepareHostScriptRuntime() { return false; }
void OnHostScriptViewCreated() {}
void UpdateHostScriptDebugState(bool enabled) {}
uint64_t HostScriptDebugEpoch() { return 0; }

bool HasHostScriptRuntime() { return false; }

void ShutdownHostScriptRuntime(ProcessRuntime::Completion completion) {
  ProcessRuntime::Result result;
  result.error = "HSR_DEBUG_LIBRARY_REQUIRED";
  if (completion) completion(std::move(result));
}

void EvaluateHostScriptRuntime(ProcessRuntime::Domain, std::string, std::string,
                               ProcessRuntime::Completion completion) {
  ProcessRuntime::Result result;
  result.error = "HSR_DEBUG_LIBRARY_REQUIRED";
  if (completion) completion(std::move(result));
}

}  // namespace shell
}  // namespace lynx
