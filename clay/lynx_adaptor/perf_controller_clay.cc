// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/perf_controller_clay.h"

#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace tasm {

namespace {
inline constexpr const char* const kHostPlatformSurface = "Clay";
}  // namespace

PerfControllerClay::PerfControllerClay(
    const std::shared_ptr<shell::PerfControllerProxy>& controller,
    int32_t instance_id)
    : perf_controller_proxy_(controller), instance_id_(instance_id) {
  if (perf_controller_proxy_) {
    perf_controller_proxy_->SetHostPlatformType(
        perf_controller_proxy_->GetPlatform() + kHostPlatformSurface);
  }
}

void PerfControllerClay::SetNeedMarkPaintEndTiming(
    const tasm::PipelineID& pipeline_id) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if (pipeline_id.empty() || !perf_controller_proxy_) {
    return;
  }
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, TIMING_SET_NEED_MARK_DRAW_END,
                      "pipeline_id", pipeline_id);
  pipeline_id_list_.push_back(pipeline_id);
}

void PerfControllerClay::OnPaintEnd() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if (pipeline_id_list_.empty() || !perf_controller_proxy_) {
    return;
  }

  auto timestamp_us = base::CurrentSystemTimeMicroseconds();
  std::vector<tasm::PipelineID> pipeline_id_list;
  pipeline_id_list.swap(pipeline_id_list_);

  std::weak_ptr<PerfControllerClay> weak_self = shared_from_this();
  perf_controller_proxy_->RunTaskInReportThread(
      [weak_self, pipeline_id_list = std::move(pipeline_id_list),
       timestamp_us]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
          return;
        }

        for (const auto& pipeline_id : pipeline_id_list) {
          strong_self->perf_controller_proxy_->SetTiming(
              timing::kPaintEnd, timestamp_us, pipeline_id);
        }
      });
}

void PerfControllerClay::OnPipelineEnd(
    std::vector<std::pair<std::string, uint64_t>> timings,
    std::vector<std::string> pipeline_id_list) {
  if (pipeline_id_list.empty() || !perf_controller_proxy_) {
    return;
  }

  auto timestamp = lynx::base::CurrentSystemTimeMicroseconds();
  std::weak_ptr<PerfControllerClay> weak_self = shared_from_this();
  perf_controller_proxy_->RunTaskInReportThread(
      [weak_self, timings = std::move(timings),
       pipeline_id_list = std::move(pipeline_id_list), timestamp]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
          return;
        }

        for (const auto& pipeline_id : pipeline_id_list) {
          // mark all host-platform-timing
          for (const auto& timing_pair : timings) {
            strong_self->perf_controller_proxy_->SetHostPlatformTiming(
                timing_pair.first, timing_pair.second, pipeline_id);
          }
          // mark pipeline-end
          strong_self->perf_controller_proxy_->SetTiming(
              timing::kPipelineEnd, timestamp, pipeline_id);
        }
      });
}

}  // namespace tasm
}  // namespace lynx
