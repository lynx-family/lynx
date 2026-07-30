// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/trace/native/hook_systrace/memory_info_trace.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_controller.h"
#include "base/trace/native/trace_event.h"

namespace lynx {
namespace trace {

MemoryInfoTrace::MemoryInfoTrace(TraceController& owner)
    : owner_(owner), thread_("memory_trace_thread") {}

void MemoryInfoTrace::DispatchBegin() {
  if (auto config = owner_.GetLastSessionTraceConfig();
      config && config->enable_memory_trace) {
    // 500ms is a balance between time-cost and accuracy.
    const static uint32_t sDelayTimeForMemoryTrace = 100;  // Ms
    auto record_memory_task = [&] {
      if (auto delegate = owner_.GetDelegate(); delegate) {
        auto data_array = delegate->GetMemoryStats();
        auto timestamp = GetTraceTimeNs();
        for (size_t i = 0; i + 1 < data_array.size(); i += 2) {
          TRACE_COUNTER(INTERNAL_TRACE_CATEGORY_VITALS, data_array[i].c_str(),
                        timestamp, std::stoull(data_array[i + 1]));
        }
      }
    };

    thread_.GetTaskRunner()->PostTask([this, record_memory_task] {
      timer_ = std::make_unique<lynx::base::TimedTaskManager>();
      timer_->SetInterval(std::move(record_memory_task),
                          sDelayTimeForMemoryTrace);
    });

    record_memory_task();
  }
}

void MemoryInfoTrace::DispatchEnd() {
  if (timer_ != nullptr) {
    thread_.GetTaskRunner()->PostTask([&] { timer_.reset(nullptr); });
  }
}

}  // namespace trace
}  // namespace lynx
