// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing.h"

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace tasm {

TimingCollector* TimingCollector::Instance() {
  static thread_local TimingCollector instance_;
  return &instance_;
}

void TimingCollector::Mark(const TimingKey& key, uint64_t timestamp) {
  // If timing_stack_ is empty, no processing is required
  if (timing_stack_.empty()) {
    return;
  }
  Timing& top_timings = timing_stack_.top();
  timestamp = timestamp > 0 ? timestamp : base::CurrentSystemTimeMicroseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, nullptr,
      [&top_timings, &key, timestamp](lynx::perfetto::EventContext ctx) {
        ctx.event()->set_name("Timing::Mark." + key);
        ctx.event()->add_debug_annotations("timing_key", key);
        ctx.event()->add_debug_annotations("pipeline_id",
                                           top_timings.pipeline_id_);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(timestamp));
      });
  top_timings.timings_[key] = timestamp;
}

void TimingCollector::MarkFrameworkTiming(const lynx::tasm::TimingKey& key,
                                          uint64_t timestamp) {
  // If timing_stack_ is empty, no processing is required
  if (timing_stack_.empty()) {
    return;
  }
  Timing& top_timings = timing_stack_.top();
  timestamp = timestamp > 0 ? timestamp : base::CurrentSystemTimeMicroseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, nullptr,
      [&top_timings, &key, timestamp](lynx::perfetto::EventContext ctx) {
        ctx.event()->set_name("Timing::MarkFrameWorkTiming." + key);
        ctx.event()->add_debug_annotations("timing_key", key);
        ctx.event()->add_debug_annotations("pipeline_id",
                                           top_timings.pipeline_id_);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(timestamp));
      });
  top_timings.framework_timings_[key] = timestamp;
}

PipelineID TimingCollector::GetTopPipelineID() {
  if (timing_stack_.empty()) {
    return "";
  }

  Timing& top_timings = timing_stack_.top();
  return top_timings.pipeline_id_;
}

}  // namespace tasm
}  // namespace lynx
