// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/harmony/trace_controller_delegate_harmony.h"

#include <utility>

#include "base/include/log/logging.h"

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

std::string TraceControllerDelegateHarmony::trace_dir_path_ = "";

std::string TraceControllerDelegateHarmony::GenerateTracingFileDir() {
  DCHECK(trace_dir_path_.size());
  return trace_dir_path_;
}

// static
void TraceControllerDelegateHarmony::SetTraceDirPath(const std::string& dir) {
  DCHECK(dir.size());
  trace_dir_path_ = dir;
}

}  // namespace trace
}  // namespace lynx
