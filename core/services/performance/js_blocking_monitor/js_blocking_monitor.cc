// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/performance/js_blocking_monitor/js_blocking_monitor.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/public/pub_value.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {
namespace performance {

inline constexpr char kJSBlockingEntryType[] = "js_blocking";
inline constexpr char kReportJSBlocking[] = "ReportJSBlocking";

struct JSBlockingMonitorArgs {
  JSBlockingMonitorArgs() {
    enabled = LynxEnv::GetInstance().EnableJSBlockingMonitor();
    threshold_ms = LynxEnv::GetInstance().GetJSBlockingThresholdMs();
    report_interval_ms = LynxEnv::GetInstance().GetJSBlockingReportIntervalMs();
  }

  bool enabled;
  int32_t threshold_ms;
  int32_t report_interval_ms;
};

const JSBlockingMonitorArgs& GetJSBlockingMonitorArgs() {
  static JSBlockingMonitorArgs args;
  return args;
}

bool JSBlockingMonitor::Enable() { return GetJSBlockingMonitorArgs().enabled; }

int32_t JSBlockingMonitor::GetThresholdMs() {
  return GetJSBlockingMonitorArgs().threshold_ms;
}

int32_t JSBlockingMonitor::GetReportIntervalMs() {
  return GetJSBlockingMonitorArgs().report_interval_ms;
}

JSTaskEnqueueRetInfo JSBlockingMonitor::MarkJSTaskEnqueue() {
  uint64_t time_now = GetNowTimeMs();
  return JSTaskEnqueueRetInfo{time_now};
}

void JSBlockingMonitor::AddBlockingTime(int64_t duration_ms) {
  int32_t threshold_ms = GetThresholdMs();
  if (duration_ms >= threshold_ms) {
    total_blocking_time_ += duration_ms;
    total_blocking_count_++;
  }
}

void JSBlockingMonitor::OnPerformanceEvent(
    const std::unique_ptr<pub::Value>& entry) {
  if (!Enable() || entry == nullptr) {
    return;
  }

  auto entryType = entry->GetValueForKey(kPerformanceEventType)->str();
  auto name = entry->GetValueForKey(kPerformanceEventName)->str();
  if (!entryType.empty() && entryType == timing::kEntryTypePipeline &&
      !name.empty() && name == timing::kEntryNameLoadBundle) {
    load_bundle_time_ = GetNowTimeMs();
    ReportBlockingInfo(kJSBlockingStageLoadBundle);
    std::weak_ptr<JSBlockingMonitor> weak_self = shared_from_this();
    report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostDelayedTask(
        [weak_self]() {
          auto self = weak_self.lock();
          if (self) {
            self->ReportWithTimer(1);
          }
        },
        fml::TimeDelta::FromMilliseconds(GetReportIntervalMs()));
  }
}

void JSBlockingMonitor::ReportWithTimer(int8_t index) {
  if (sender_ == nullptr) {
    return;
  }
  auto interval = GetReportIntervalMs();
  auto report_interval = (index + 1) * interval;

  ReportBlockingInfo(kJSBlockingStageTimer);
  std::weak_ptr<JSBlockingMonitor> weak_self = shared_from_this();
  report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostDelayedTask(
      [weak_self, index]() {
        auto self = weak_self.lock();
        if (self) {
          self->ReportWithTimer(index + 1);
        }
      },
      fml::TimeDelta::FromMilliseconds(report_interval));
}

void JSBlockingMonitor::ReportBlockingInfo(const std::string& stage) {
  LOGI("ReportBlockingInfo stage:" << stage);
  if (!Enable() || sender_ == nullptr || total_blocking_time_ == 0 ||
      total_blocking_count_ == 0) {
    return;
  }
  auto now_time = GetNowTimeMs();
  auto total_duration_ms = now_time - last_report_time_;
  auto time_after_load_bundle = now_time - load_bundle_time_;
  // reset last_report_time_
  last_report_time_ = now_time;
  auto blocking_ratio =
      static_cast<double>(total_blocking_time_) / total_duration_ms;
  auto avg_blocking_time =
      static_cast<double>(total_blocking_time_) / total_blocking_count_;

  auto& value_factory = sender_->GetValueFactory();
  if (value_factory == nullptr) {
    return;
  }

  auto entry_map = value_factory->CreateMap();
  entry_map->PushStringToMap(kPerformanceEventType, kJSBlockingEntryType);
  entry_map->PushStringToMap(kPerformanceEventName, kJSBlockingEntryType);
  entry_map->PushStringToMap("stage", stage);
  entry_map->PushInt64ToMap("total_blocking_time", total_blocking_time_);
  entry_map->PushInt64ToMap("total_blocking_count", total_blocking_count_);
  entry_map->PushInt64ToMap("total_duration", total_duration_ms);
  entry_map->PushDoubleToMap("blocking_ratio", blocking_ratio);
  entry_map->PushDoubleToMap("avg_blocking_time", avg_blocking_time);
  if (stage == kJSBlockingStageTimer) {
    entry_map->PushInt64ToMap("time_after_load_bundle", time_after_load_bundle);
  }

  TRACE_EVENT(
      INTERNAL_TRACE_CATEGORY, kReportJSBlocking,
      [&entry_map](lynx::perfetto::EventContext ctx) {
        entry_map->ForeachMap([debug = ctx.event()](const pub::Value& key,
                                                    const pub::Value& val) {
          if (key.IsString() && key.str() != timing::kEntryName &&
              key.str() != timing::kEntryType) {
            if (val.IsString()) {
              debug->add_debug_annotations(key.str(), val.str());
            } else if (val.IsNumber()) {
              debug->add_debug_annotations(key.str(),
                                           std::to_string(val.Number()));
            }
          }
        });
      });
  sender_->OnPerformanceEvent(std::move(entry_map), kEventTypePlatform);

  // Reset after reporting
  total_blocking_time_ = 0;
  total_blocking_count_ = 0;
}
}  // namespace performance
}  // namespace tasm
}  // namespace lynx
