// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/darwin/trace_controller_darwin.h"
#import <Foundation/Foundation.h>

#include <chrono>
#import <string>
#import <thread>

#include "base/include/memory/process_memory_info.h"

namespace lynx {
namespace trace {

TraceController* GetTraceControllerInstance() {
  static bool should_init_delegate = true;
  if (should_init_delegate) {
    auto delegate = std::make_unique<TraceControllerDelegateDarwin>();
    TraceController::Instance()->SetDelegate(std::move(delegate));
    should_init_delegate = false;
  }
  return TraceController::Instance();
}

std::string TraceControllerDelegateDarwin::GenerateTracingFileDir() {
  return std::string([[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask,
                                                           YES) lastObject] UTF8String]);
}

std::vector<std::string> TraceControllerDelegateDarwin::GetMemoryStats() {
  const int64_t pss_bytes = lynx::base::GetProcessPssBytes();
  return {"summary.total-pss", std::to_string(pss_bytes > 0 ? pss_bytes : 0)};
}

}  // namespace trace
}  // namespace lynx
