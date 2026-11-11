// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/harmony/trace_controller_delegate_harmony.h"

#include <utility>

#include "base/include/memory/process_memory_info.h"

namespace lynx {
namespace trace {

TraceController* GetTraceControllerInstance() {
  static bool should_init_delegate = true;
  if (should_init_delegate) {
    auto delegate = std::make_unique<TraceControllerDelegateHarmony>();
    TraceController::Instance()->SetDelegate(std::move(delegate));
    should_init_delegate = false;
  }
  return TraceController::Instance();
}

std::vector<std::string> TraceControllerDelegateHarmony::GetMemoryStats() {
  const int64_t pss_bytes = lynx::base::GetProcessPssBytes();
  return {"summary.total-pss", std::to_string(pss_bytes > 0 ? pss_bytes : 0)};
}

}  // namespace trace
}  // namespace lynx
