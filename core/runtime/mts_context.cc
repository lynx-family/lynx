// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/mts_context.h"

#include "core/shell/runtime/mts/mts_runtime.h"

namespace lynx {
namespace runtime {

void MTSContext::OnBTSConsoleEvent(const std::string& func_name,
                                   const std::string& args) {
  if (runtime_private_) {
    runtime_private_->OnBTSConsoleEvent(func_name, args);
  }
}

void MTSContext::ReportErrorWithMsg(const std::string& msg, int32_t error_code,
                                    int32_t level) {
  if (runtime_private_) {
    runtime_private_->ReportErrorWithMsg(msg, error_code, level);
  }
}

void MTSContext::ReportError(const std::string& exception_info,
                             int32_t err_code,
                             base::LynxErrorLevel error_level) {
  if (runtime_private_) {
    runtime_private_->ReportError(exception_info, err_code, error_level);
  }
}

void MTSContext::ReportGCTimingEvent(const char* start, const char* end) {
  if (runtime_private_) {
    runtime_private_->ReportGCTimingEvent(start, end);
  }
}

void MTSContext::OnContextGC(std::string mem_info) {
  if (runtime_private_) {
    runtime_private_->OnGC(std::move(mem_info));
  }
}

}  // namespace runtime
}  // namespace lynx
