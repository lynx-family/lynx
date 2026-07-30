// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/platform/darwin/trace_controller_darwin.h"
#import <Foundation/Foundation.h>

#include <chrono>
#import <string>
#import <thread>

#import <mach/mach.h>
#import <mach/task_info.h>

namespace lynx {
namespace trace {

namespace {
task_vm_info_data_t GetMemoryInfo() {
  task_t task = mach_task_self();

  task_vm_info_data_t info;
  mach_msg_type_number_t count = TASK_VM_INFO_COUNT;

  kern_return_t kr = task_info(task, TASK_VM_INFO, (task_info_t)&info, &count);

  if (kr != KERN_SUCCESS) {
    return task_vm_info_data_t();
  }
  return info;
}
}  // namespace

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
  auto memory_info = GetMemoryInfo();
  return {"summary.total-pss", std::to_string(memory_info.phys_footprint)};
}

}  // namespace trace
}  // namespace lynx
