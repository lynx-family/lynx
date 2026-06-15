// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/lynx_adaptor/perf_controller_clay.h"

#include <random>
#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/trace/native/trace_event.h"
#if defined(OS_IOS)
#include "core/renderer/utils/lynx_env.h"
#endif
#include "core/services/timing_handler/timing_constants.h"
#include "core/services/trace/service_trace_event_def.h"

namespace lynx {
namespace tasm {

namespace {
inline constexpr const char* const kHostPlatformSurface = "Clay";
constexpr int32_t kMinSessionDurationInMs = 200;  // 200ms
constexpr int32_t kMaxSessionDurationInSec = 10;  // 10s
constexpr std::string_view kLynxFluencyEvent = "lynxsdk_fluency_event";
// iOS native renderer posts paintEnd from SetNeedMarkPaintEndTiming. Keep Clay
// iOS aligned with that timing point; other platforms still report at draw-end.
#if defined(OS_IOS)
constexpr bool kPostPaintEndOnSetNeedMarkPaintEndTiming = true;
#else
constexpr bool kPostPaintEndOnSetNeedMarkPaintEndTiming = false;
#endif

uint64_t GenerateSessionID() {
  static uint64_t session_id = 0;
  ++session_id;
  if (session_id == 0) {
    ++session_id;
  }
  return session_id;
}

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
  if constexpr (kPostPaintEndOnSetNeedMarkPaintEndTiming) {
    // iOS needs draw_end to follow the native renderer's dispatch_async
    // polyfill timing, so post paintEnd when the timing request is recorded.
    PostPaintEndOnUIThread(pipeline_id);
  }
}

void PerfControllerClay::SetPageConfigProbability(double probability) {
  page_config_probability_ = probability;
  if (page_config_probability_ >= 0.0) {
    // Generate a random number between 0.0 and 1.0
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<double> dis(0.0, 1.0);
    double random_value = dis(gen);

    // Enable fluency monitor if random value is less than or equal to the
    // probability
    if (random_value <= page_config_probability_) {
      force_fluency_monitor_status_ = ForceFluencyMonitorStatus::FORCED_ON;
    } else {
      force_fluency_monitor_status_ = ForceFluencyMonitorStatus::FORCED_OFF;
    }
  } else {
    // TODO(zuojinglong): This is a temporary split handling for Android/iOS.
    // Unify the default fluency monitor strategy across all Clay platforms.
#if defined(OS_IOS)
    default_enable_fluency_monitor_ =
        lynx::tasm::LynxEnv::GetInstance().GetBoolEnv(
            lynx::tasm::LynxEnv::Key::ENABLE_FLUENCY_TRACE, false);
#endif
  }
  UpdateEnableFluencyMonitor();
}

void PerfControllerClay::SetEnableFluencyMonitor(bool enable) {
  default_enable_fluency_monitor_ = enable;
  UpdateEnableFluencyMonitor();
}

void PerfControllerClay::PostPaintEndOnUIThread(tasm::PipelineID pipeline_id) {
  std::weak_ptr<PerfControllerClay> weak_self = shared_from_this();
  // Match Darwin non-Clay behavior: each pipeline id owns one posted paintEnd
  // task instead of being batched with later timing requests.
  ui_task_runner_->PostTask(
      [weak_self, pipeline_id = std::move(pipeline_id)]() mutable {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
          return;
        }
        std::vector<tasm::PipelineID> pipeline_ids;
        pipeline_ids.push_back(std::move(pipeline_id));
        strong_self->MarkPaintEndForPipelineIds(std::move(pipeline_ids));
      });
}

void PerfControllerClay::UpdateEnableFluencyMonitor() {
  if (force_fluency_monitor_status_ == ForceFluencyMonitorStatus::FORCED_ON) {
    enable_fluency_monitor_ = true;
  } else if (force_fluency_monitor_status_ ==
             ForceFluencyMonitorStatus::FORCED_OFF) {
    enable_fluency_monitor_ = false;
  } else {
    enable_fluency_monitor_ = default_enable_fluency_monitor_;
  }
}

void PerfControllerClay::OnPaintEnd() {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if constexpr (kPostPaintEndOnSetNeedMarkPaintEndTiming) {
    // iOS has reported paintEnd from SetNeedMarkPaintEndTiming. Keep draw-end
    // responsible only for clearing ids retained for Clay frame timing.
    pipeline_id_list_.clear();
  } else {
    // Other Clay platforms keep the original behavior: report paintEnd when
    // PageView sends the draw-end event.
    if (pipeline_id_list_.empty() || !perf_controller_proxy_) {
      return;
    }
    std::vector<tasm::PipelineID> pipeline_id_list;
    pipeline_id_list.swap(pipeline_id_list_);
    MarkPaintEndForPipelineIds(std::move(pipeline_id_list));
  }
}

void PerfControllerClay::MarkPaintEndForPipelineIds(
    std::vector<tasm::PipelineID> pipeline_id_list) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if (pipeline_id_list.empty() || !perf_controller_proxy_) {
    return;
  }
  auto timestamp_us = base::CurrentSystemTimeMicroseconds();

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

void PerfControllerClay::StartFluencyMonitor(
    int id, const std::string& scene, const std::string& scroll_monitor_tag,
    int max_refresh_rate) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if (!enable_fluency_monitor_ || !perf_controller_proxy_) {
    return;
  }

  if (fps_tracers_.find(id) != fps_tracers_.end()) {
    return;
  }

  // Generate a unique session ID for this monitor
  uint64_t session_id = GenerateSessionID();
  fluency_monitor_session_ids_[id] = session_id;

  fps_tracers_[id] = std::make_unique<clay::FpsTracer>(
      scene, scroll_monitor_tag, max_refresh_rate, kMaxSessionDurationInSec);
  fps_tracers_[id]->Start();

  std::weak_ptr<PerfControllerClay> weak_self = shared_from_this();
  // If one scroll event does not stop after 10 seconds, we stop it manually.
  ui_task_runner_->PostDelayedTask(
      [weak_self, id, session_id]() {
        auto strong_self = weak_self.lock();
        if (!strong_self) {
          return;
        }

        // Check if this session is still active
        auto it = strong_self->fluency_monitor_session_ids_.find(id);
        if (it != strong_self->fluency_monitor_session_ids_.end() &&
            it->second == session_id) {
          // Only stop if the session ID matches (it's the same session)
          strong_self->EndFluencyMonitor(id);
        }
      },
      fml::TimeDelta::FromSeconds(kMaxSessionDurationInSec));
}

void PerfControllerClay::EndFluencyMonitor(int id) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if (!enable_fluency_monitor_ || !perf_controller_proxy_) {
    return;
  }

  // Remove the session ID from the map
  auto it = fluency_monitor_session_ids_.find(id);
  if (it != fluency_monitor_session_ids_.end()) {
    // Remove the session ID from the map
    fluency_monitor_session_ids_.erase(it);
  }

  if (fps_tracers_.find(id) != fps_tracers_.end()) {
    auto fps_tracer = std::move(fps_tracers_[id]);
    fps_tracers_.erase(id);
    if (!fps_tracer) {
      return;
    }

    fps_tracer->Stop();
    std::weak_ptr<PerfControllerClay> weak_self = shared_from_this();
    perf_controller_proxy_->RunTaskInReportThread(
        [weak_self, fps_tracer = std::move(fps_tracer)] {
          auto strong_self = weak_self.lock();
          if (!strong_self) {
            return;
          }

          clay::FpsRawMetrics metrics;
          fps_tracer->GetFpsMetrics(metrics);
          // just ignore scroll event with duration less than 200ms.
          if (metrics.duration_ms < kMinSessionDurationInMs) {
            return;
          }

          shell::ReportEvent event;
          event.event_name = std::string(kLynxFluencyEvent);

          // string props
          event.string_props.insert_or_assign("lynxsdk_fluency_scene",
                                              fps_tracer->GetConfig().scene);
          event.string_props.insert_or_assign("lynxsdk_fluency_tag",
                                              fps_tracer->GetConfig().tag);

          // int props
          event.int_props.insert_or_assign("lynxsdk_fluency_maximum_frames",
                                           metrics.max_refresh_rate);
          event.int_props.insert_or_assign("lynxsdk_fluency_frames_number",
                                           metrics.frames);
          event.int_props.insert_or_assign("lynxsdk_fluency_fps", metrics.fps);
          event.int_props.insert_or_assign("lynxsdk_fluency_dur",
                                           metrics.duration_ms);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop1_count",
                                           metrics.drop1_count);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop1_duration",
                                           metrics.drop1_duration_ms);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop3_count",
                                           metrics.drop3_count);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop3_duration",
                                           metrics.drop3_duration_ms);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop7_count",
                                           metrics.drop7_count);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop7_duration",
                                           metrics.drop7_duration_ms);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop25_count",
                                           metrics.drop25_count);
          event.int_props.insert_or_assign("lynxsdk_fluency_drop25_duration",
                                           metrics.drop25_duration_ms);

          // double props
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop1_count_per_second",
              1000.0 * metrics.drop1_count / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop3_count_per_second",
              1000.0 * metrics.drop3_count / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop7_count_per_second",
              1000.0 * metrics.drop7_count / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop25_count_per_second",
              1000.0 * metrics.drop25_count / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop1_ratio",
              1000.0 * metrics.drop1_duration_ms / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop3_ratio",
              1000.0 * metrics.drop3_duration_ms / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop7_ratio",
              1000.0 * metrics.drop7_duration_ms / metrics.duration_ms);
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_drop25_ratio",
              1000.0 * metrics.drop25_duration_ms / metrics.duration_ms);

          // Use real page config probability
          event.double_props.insert_or_assign(
              "lynxsdk_fluency_pageconfig_probability",
              strong_self->page_config_probability_);
          event.int_props.insert_or_assign(
              "lynxsdk_fluency_enabled_by_sampling", 0);

          strong_self->perf_controller_proxy_->OnEvent(
              strong_self->instance_id_, event);
        });
  }
}

void PerfControllerClay::OnFrameTiming(int64_t frame_start_time_nanos,
                                       int64_t frame_end_time_nanos) {
  DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());

  if (!enable_fluency_monitor_ || !perf_controller_proxy_) {
    return;
  }

  for (auto& fps_tracer : fps_tracers_) {
    fps_tracer.second->AddFrameTimeSample(frame_start_time_nanos);
  }
}

}  // namespace tasm
}  // namespace lynx
