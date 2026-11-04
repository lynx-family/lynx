// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/tracing/platform/memory_trace_plugin_harmony.h"

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include <chrono>
#include <thread>

#include "base/include/fml/message_loop.h"
#include "base/include/memory/memory_pressure_level.h"
#include "base/trace/native/trace_defines.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

namespace lynx {
namespace trace {

MemoryTracePluginHarmony::MemoryTracePluginHarmony()
    : notification_callback_(
          LYNX_TRACE_MEMORY_PLUGIN_GC_NOTIFICATION,
          [this](const std::string&, intptr_t) {
            {
              std::lock_guard<std::mutex> lock(gc_task_mutex_);
              if (!enabled_ || gc_task_scheduled_) {
                return;
              }
              gc_task_scheduled_ = true;
            }
            auto task_runner = fml::MessageLoop::GetCurrent().GetTaskRunner();
            task_runner->PostDelayedTask(
                [this]() {
                  {
                    std::lock_guard<std::mutex> lock(gc_task_mutex_);
                    if (!enabled_) {
                      gc_task_scheduled_ = false;
                      return;
                    }
                  }
                  RunGC();
                  {
                    std::lock_guard<std::mutex> lock(gc_task_mutex_);
                    gc_task_scheduled_ = false;
                  }
                },
                fml::TimeDelta::FromMilliseconds(100));
          }) {}

void MemoryTracePluginHarmony::DispatchSetup(
    const std::shared_ptr<TraceConfig>& config) {
  {
    std::lock_guard<std::mutex> lock(gc_task_mutex_);
    enabled_ = config->enable_memory_trace;
  }
  if (config->enable_memory_trace) {
    RunGC();
  }
}

void MemoryTracePluginHarmony::DispatchEnd() {
  {
    std::lock_guard<std::mutex> lock(gc_task_mutex_);
    if (!enabled_) {
      return;
    }
    enabled_ = false;
  }
  RunGC();
}

std::string MemoryTracePluginHarmony::Name() { return "Memory"; }

void MemoryTracePluginHarmony::RunGC() {
  harmony::LynxTemplateRenderer::TrimMemory(
      base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL);
  // LynxEnv.trimMemory dispatches GC to the engine and runtime task runners.
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

}  // namespace trace
}  // namespace lynx
#endif
