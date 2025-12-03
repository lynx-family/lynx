// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/perf_controller_proxy_impl.h"

#include <string>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/tasm/config.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace shell {
void PerfControllerProxyImpl::MarkTiming(tasm::TimingKey timing_key,
                                         const tasm::PipelineID& pipeline_id) {
  auto timestamp_us = base::CurrentSystemTimeMicroseconds();
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, TIMING_MARK + std::string(timing_key),
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
  SetTiming(timing_key, timestamp_us, pipeline_id);
}

void PerfControllerProxyImpl::SetTiming(tasm::TimingKey timing_key,
                                        uint64_t timestamp_us,
                                        const tasm::PipelineID& pipeline_id) {
  perf_actor_->ActAsync(
      [timestamp_us, pipeline_id,
       timing_key = std::move(timing_key)](auto& controller) mutable {
        controller->GetTimingHandler().SetTiming(timing_key, timestamp_us,
                                                 pipeline_id);
      });
}

void PerfControllerProxyImpl::SetHostPlatformTiming(
    tasm::TimingKey timing_key, uint64_t timestamp_us,
    const tasm::PipelineID& pipeline_id) {
  perf_actor_->ActAsync(
      [timestamp_us, pipeline_id,
       timing_key = std::move(timing_key)](auto& controller) mutable {
        controller->GetTimingHandler().SetHostPlatformTiming(
            timing_key, timestamp_us, pipeline_id);
      });
}

void PerfControllerProxyImpl::SetHostPlatformType(const std::string& type) {
  perf_actor_->ActAsync([type](auto& controller) mutable {
    controller->GetTimingHandler().SetHostPlatformType(type);
  });
}

std::string PerfControllerProxyImpl::GetPlatform() const {
  return lynx::tasm::Config::Platform();
}

void PerfControllerProxyImpl::RunTaskInReportThread(base::closure task) {
  perf_actor_->ActAsync([task = std::move(task)](auto& controller) mutable {
    if (task) {
      task();
    }
  });
}

void PerfControllerProxyImpl::OnEvent(int32_t instance_id, ReportEvent& event) {
  perf_actor_->ActAsync(
      [instance_id, event = std::move(event)](auto& controller) mutable {
        lynx::tasm::report::MoveOnlyEvent move_only_event;
        move_only_event.SetName(event.event_name.c_str());
        move_only_event.SetStringProps(event.string_props);
        move_only_event.SetDoubleProps(event.double_props);
        move_only_event.SetIntProps(event.int_props);
        lynx::tasm::report::EventTrackerPlatformImpl::OnEvent(
            instance_id, std::move(move_only_event));
      });
}

}  // namespace shell
}  // namespace lynx
