// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_collector_platform_impl.h"

#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace tasm {
namespace timing {

void TimingCollectorPlatformImpl::MarkTiming(
    const tasm::PipelineID& pipeline_id, const std::string& timing_key) const {
  SetTiming(pipeline_id, timing_key, base::CurrentSystemTimeMicroseconds());
}

void TimingCollectorPlatformImpl::SetTiming(const tasm::PipelineID& pipeline_id,
                                            const std::string& timing_key,
                                            uint64_t us_timestamp) const {
  if (timing_actor_) {
    TRACE_EVENT_INSTANT(
        LYNX_TRACE_CATEGORY, TIMING_MARK + timing_key,
        [&pipeline_id, &timing_key, us_timestamp,
         instance_id =
             timing_actor_->GetInstanceId()](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("timing_key", timing_key);
          ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
          ctx.event()->add_debug_annotations("timestamp",
                                             std::to_string(us_timestamp));
          ctx.event()->add_debug_annotations("instance_id",
                                             std::to_string(instance_id));
        });
    timing_actor_->ActAsync([timing_key, us_timestamp,
                             pipeline_id](auto& timing_handler) {
      std::string mutable_timing_key(timing_key);
      timing_handler->SetTiming(mutable_timing_key, us_timestamp, pipeline_id);
    });
  }
}

void TimingCollectorPlatformImpl::SetNeedMarkDrawEndTiming(
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

void TimingCollectorPlatformImpl::MarkDrawEndTimingIfNeeded() {
  auto us_timestamp = base::CurrentSystemTimeMicroseconds();
  auto paint_end_pipeline_id_list = paint_end_pipeline_id_list_.PopAll();
  for (const auto& pipeline_id : paint_end_pipeline_id_list) {
    SetTiming(pipeline_id, tasm::timing::kPaintEnd, us_timestamp);
  }
}

void TimingCollectorPlatformImpl::OnPipelineStart(
    const tasm::PipelineID& pipeline_id,
    const lynx::tasm::PipelineOrigin& pipeline_origin,
    lynx::tasm::timing::TimestampUs pipeline_start_timestamp) {
  if (timing_actor_) {
    timing_actor_->ActAsync([pipeline_id, pipeline_origin,
                             pipeline_start_timestamp](auto& timing_handler) {
      timing_handler->OnPipelineStart(pipeline_id, pipeline_origin,
                                      pipeline_start_timestamp);
    });
  }
}
void TimingCollectorPlatformImpl::ResetTimingBeforeReload() {
  if (timing_actor_) {
    timing_actor_->Act([](auto& timing_handler) {
      timing_handler->ResetTimingBeforeReload();
    });
  }
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
