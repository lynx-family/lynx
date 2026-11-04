// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_MEMORY_TRACE_PLUGIN_HARMONY_H_
#define DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_MEMORY_TRACE_PLUGIN_HARMONY_H_

#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
#include <mutex>

#include "base/include/notification_center.h"
#include "base/trace/native/trace_controller.h"

namespace lynx {
namespace trace {

class MemoryTracePluginHarmony : public TracePlugin {
 public:
  MemoryTracePluginHarmony();
  ~MemoryTracePluginHarmony() override = default;
  void DispatchSetup(const std::shared_ptr<TraceConfig>& config) override;
  void DispatchBegin() override {}
  void DispatchEnd() override;
  std::string Name() override;

 private:
  void RunGC();

  bool enabled_{false};
  bool gc_task_scheduled_{false};
  std::mutex gc_task_mutex_;
  base::NotificationCallback notification_callback_;
};

}  // namespace trace
}  // namespace lynx
#endif
#endif  // DEVTOOL_LYNX_DEVTOOL_TRACING_PLATFORM_MEMORY_TRACE_PLUGIN_HARMONY_H_
