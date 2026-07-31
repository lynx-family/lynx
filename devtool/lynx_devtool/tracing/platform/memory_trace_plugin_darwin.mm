// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/platform/memory_trace_plugin_darwin.h"

#include <chrono>
#include <thread>

#import <Lynx/LynxEnv.h>

#include "base/include/fml/message_loop.h"
#include "base/trace/native/trace_defines.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
namespace lynx {
namespace trace {

MemoryTracePluginDarwin::MemoryTracePluginDarwin()
    : notification_callback_(LYNX_TRACE_MEMORY_PLUGIN_GC_NOTIFICATION,
                             [&](const std::string& tag, [[maybe_unused]] intptr_t data) {
                               auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
                               {
                                 std::lock_guard<std::mutex> lock(gc_task_mutex_);
                                 if (gc_task_scheduled_) {
                                   return;
                                 }
                                 gc_task_scheduled_ = true;
                               }
                               task_runner->PostDelayedTask(
                                   [this]() {
                                     this->RunGC();
                                     {
                                       std::lock_guard<std::mutex> lock(gc_task_mutex_);
                                       gc_task_scheduled_ = false;
                                     }
                                   },
                                   fml::TimeDelta::FromMilliseconds(100));
                             }) {}

void MemoryTracePluginDarwin::DispatchSetup(const std::shared_ptr<TraceConfig>& config) {
  force_gc_ = config->memory_trace_force_gc;
  if (force_gc_) {
    RunGC();
  }
}

void MemoryTracePluginDarwin::DispatchEnd() {
  if (force_gc_) {
    RunGC();
  }
}

std::string MemoryTracePluginDarwin::Name() { return "Memory"; }

void MemoryTracePluginDarwin::RunGC() {
  [[LynxEnv sharedInstance] trimMemory:LynxMemoryPressureLevelCritical];
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

}  // namespace trace
}  // namespace lynx
#endif
