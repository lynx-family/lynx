// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/timing_handler/timing_collector_platform_impl.h"

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
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, nullptr,
      [&pipeline_id, &timing_key,
       us_timestamp](lynx::perfetto::EventContext ctx) {
        ctx.event()->set_name("Timing::Mark." + timing_key);
        ctx.event()->add_debug_annotations("timing_key", timing_key);
        ctx.event()->add_debug_annotations("pipeline_id", pipeline_id);
        ctx.event()->add_debug_annotations("timestamp",
                                           std::to_string(us_timestamp));
      });
  if (timing_actor_) {
    timing_actor_->Act([timing_key, us_timestamp,
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
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, "Timing::SetNeedMarkDrawEndTiming",
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

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
