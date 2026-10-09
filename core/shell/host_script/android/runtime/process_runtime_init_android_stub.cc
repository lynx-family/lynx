// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/shell/host_script/android/runtime/process_runtime_android.h"
#include "core/shell/host_script/android/runtime/process_runtime_init_android.h"

namespace lynx {
namespace shell {

void PrepareHostScriptRuntime() {}
void OnHostScriptViewCreated() {}
void UpdateHostScriptDebugState(bool enabled) {}

void LoadHostScriptRuntime(std::string, std::string,
                           ProcessRuntime::Completion completion) {
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

uint64_t HostScriptDebugEpoch() { return 0; }

void LoadHostScriptRuntime(std::string source, std::string url, uint64_t,
                           ProcessRuntime::Completion completion) {
  LoadHostScriptRuntime(std::move(source), std::move(url),
                        std::move(completion));
}

void EvaluateHostScriptRuntime(ProcessRuntime::Domain domain,
                               std::string source, std::string url, uint64_t,
                               ProcessRuntime::Completion completion) {
  EvaluateHostScriptRuntime(domain, std::move(source), std::move(url),
                            std::move(completion));
}

}  // namespace shell
}  // namespace lynx
