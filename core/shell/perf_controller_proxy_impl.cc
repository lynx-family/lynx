// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/perf_controller_proxy_impl.h"

#include <string>

#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace shell {
void PerfControllerProxyImpl::MarkPaintEndTimingIfNeeded() {
  if (paint_end_pipeline_id_list_.Empty()) {
    return;
  }
  auto us_timestamp = base::CurrentSystemTimeMicroseconds();
  auto paint_end_pipeline_id_list = paint_end_pipeline_id_list_.PopAll();
  for (const auto& pipeline_id : paint_end_pipeline_id_list) {
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY, TIMING_MARK + std::string(tasm::timing::kPaintEnd),
        [us_timestamp, &pipeline_id,
         instance_id =
             perf_actor_->GetInstanceId()](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("timing_key",
                                             tasm::timing::kPaintEnd);
          ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
          ctx.event()->add_debug_annotations("timestamp",
                                             std::to_string(us_timestamp));
          ctx.event()->add_debug_annotations("instance_id",
                                             std::to_string(instance_id));
        });
    perf_actor_->ActAsync(
        [us_timestamp, pipeline_id](auto& controller) mutable {
          tasm::TimingKey key(tasm::timing::kPaintEnd);
          controller->GetTimingHandler().SetTiming(
              key, static_cast<lynx::tasm::timing::TimestampUs>(us_timestamp),
              pipeline_id);
        });
  }
}

void PerfControllerProxyImpl::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  if (pipeline_id.empty()) {
    return;
  }
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, TIMING_SET_NEED_MARK_DRAW_END,
                      [&pipeline_id](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_debug_annotations("pipeline_id",
                                                           pipeline_id);
                      });
  paint_end_pipeline_id_list_.Push(pipeline_id);
}

}  // namespace shell
}  // namespace lynx
