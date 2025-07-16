// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/perf_controller_proxy_impl.h"

#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace shell {
void PerfControllerProxyImpl::MarkTiming(tasm::TimingKey timing_key,
                                         const tasm::PipelineID& pipeline_id) {
  auto timestamp_us = base::CurrentSystemTimeMicroseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, TIMING_MARK + std::string(tasm::timing::kPaintEnd),
      [timestamp_us, &pipeline_id, &timing_key,
       instance_id =
           perf_actor_->GetInstanceId()](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("timing_key", timing_key);
        ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(timestamp_us));
        ctx.event()->add_debug_annotations("instance_id",
                                           std::to_string(instance_id));
      });
  perf_actor_->ActAsync([timestamp_us, pipeline_id,
                         timing_key =
                             std::move(timing_key)](auto& controller) mutable {
    auto key = static_cast<tasm::timing::TimestampKey&>(timing_key);
    controller->GetTimingHandler().SetTiming(
        key, static_cast<tasm::timing::TimestampUs>(timestamp_us), pipeline_id);
  });
}
}  // namespace shell
}  // namespace lynx
