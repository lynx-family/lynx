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

enum BoolValue : uint8_t { Unset = 0, False = 1, True = 2 };
// Highest priority setting
static std::atomic<BoolValue> g_force_enable_{Unset};
// Environment-based setting
static std::atomic<BoolValue> g_env_enable_{Unset};

static std::atomic<int32_t> g_threshold_ms_{0};

static std::atomic<int32_t> g_interval_ms_{0};

bool JSBlockingMonitor::Enable() {
  // [Priority 1] Check force-enable setting first (explicit override)
  const auto force_val = g_force_enable_.load(std::memory_order_acquire);
  if (force_val != Unset) {
    return force_val == True;  // Force setting takes absolute precedence
  }

  // [Priority 2] Check cached environment setting
  const auto env_val = g_env_enable_.load(std::memory_order_acquire);
  if (env_val != Unset) {
    return env_val == True;  // Use cached environment value if available
  }

  // [Initialization] Both unset - fetch from environment (once)
  static std::once_flag init_flag;
  bool ret = false;
  std::call_once(init_flag, [&] {
    // For now, we default to true, but this could be configured via settings
    ret = LynxEnv::GetInstance().EnableJSBlockingMonitor();

    // Cache environment result for future calls
    g_env_enable_.store(ret ? True : False, std::memory_order_release);
  });
  return ret;
}

// External control interface (sets highest priority flag)
void JSBlockingMonitor::SetForceEnable(bool enable) {
  // This override takes precedence over environment settings
  g_force_enable_.store(enable ? True : False, std::memory_order_release);
}

int32_t JSBlockingMonitor::GetThresholdMs() {
  // [Priority 1] Check cached environment setting
  const auto env_val = g_threshold_ms_.load(std::memory_order_acquire);
  if (env_val != 0) {
    return env_val;  // Use cached environment value if available
  }

  // [Initialization] Both unset - fetch from environment (once)
  static std::once_flag threshold_init_flag;
  int32_t ret = 5;
  std::call_once(threshold_init_flag, [&] {
    // For now, we default to 1ms, but this could be configured via settings
    ret = LynxEnv::GetInstance().GetJSBlockingThresholdMs();

    // Cache environment result for future calls
    g_threshold_ms_.store(ret, std::memory_order_release);
  });
  return ret;
}

int32_t JSBlockingMonitor::GetReportIntervalMs() {
  // [Priority 1] Check cached environment setting
  const auto env_val = g_interval_ms_.load(std::memory_order_acquire);
  if (env_val != 0) {
    return env_val;  // Use cached environment value if available
  }

  // [Initialization] Both unset - fetch from environment (once)
  static std::once_flag interval_init_flag;
  int32_t ret = 5;
  std::call_once(interval_init_flag, [&] {
    // For now, we default to 1ms, but this could be configured via settings
    ret = LynxEnv::GetInstance().GetJSBlockingReportIntervalMs();

    // Cache environment result for future calls
    g_interval_ms_.store(ret, std::memory_order_release);
  });
  return ret;
}

uint64_t JSBlockingMonitor::GetNowTimeMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now().time_since_epoch())
      .count();
}

int64_t JSBlockingMonitor::MarkStartTraceInstant() {
  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY, kJSBlockingStart,
                      [trace_flow_id](lynx::perfetto::EventContext ctx) {
                        ctx.event()->add_flow_ids(trace_flow_id);
                      });
  return trace_flow_id;
}

void JSBlockingMonitor::AddBlockingTime(int64_t duration_ms) {
  if (!Enable()) {
    return;
  }
  int32_t threshold_ms = GetThresholdMs();
  if (duration_ms >= threshold_ms) {
    total_blocking_time_ += duration_ms;
    total_blocking_count_++;
  }
}

void JSBlockingMonitor::OnSetupEvent(const std::unique_ptr<pub::Value>& entry) {
  if (!Enable() || entry == nullptr) {
    return;
  }

  auto entryType = entry->GetValueForKey(kPerformanceEventType)->str();
  auto name = entry->GetValueForKey(kPerformanceEventName)->str();
  if (!entryType.empty() && entryType == timing::kEntryTypeMetric &&
      !name.empty() && name == timing::kFCP) {
    auto totalFcpMap = entry->GetValueForKey(timing::kTotalFCP);
    auto fcpMap = entry->GetValueForKey(timing::kFCP);
    auto lynxFcpMap = entry->GetValueForKey(timing::kLynxFCP);

    std::unique_ptr<pub::Value> selectedFcpMap;
    if (totalFcpMap && totalFcpMap->IsMap()) {
      selectedFcpMap = std::move(totalFcpMap);
    } else if (fcpMap && fcpMap->IsMap()) {
      selectedFcpMap = std::move(fcpMap);
    } else if (lynxFcpMap && lynxFcpMap->IsMap()) {
      selectedFcpMap = std::move(lynxFcpMap);
    }
    if (selectedFcpMap) {
      selectedFcpMap->ForeachMap(
          [this](const pub::Value& k, const pub::Value& v) {
            if (!k.str().empty() && k.str() == timing::kDuration) {
              auto duration = v.Double();
              ReportBlockingInfo(kJSBlockingStageFCP, duration, 0);
            }
          });
      report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostDelayedTask(
          [this]() { ReportWithTimer(0); },
          fml::TimeDelta::FromMilliseconds(GetReportIntervalMs()));
    }
  }
}

void JSBlockingMonitor::ReportWithTimer(int8_t index) {
  if (sender_ == nullptr || timer_stopped_) {
    return;
  }
  auto report_interval = GetReportIntervalMs();
  auto total_duration_ms = report_interval;
  auto time_after_fcp = (index + 1) * report_interval;

  ReportBlockingInfo("timer", total_duration_ms, time_after_fcp);
  report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostDelayedTask(
      [this, index]() {
        if (!timer_stopped_) {
          ReportWithTimer(index + 1);
        }
      },
      fml::TimeDelta::FromSeconds(report_interval));
}

void JSBlockingMonitor::ReportBlockingInfo(const std::string& stage,
                                           int64_t total_duration_ms,
                                           int64_t time_after_fcp) {
  if (!Enable() || sender_ == nullptr || total_duration_ms == 0 ||
      total_blocking_time_ == 0 || total_blocking_count_ == 0) {
    return;
  }
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
    entry_map->PushInt64ToMap("time_after_fcp", time_after_fcp);
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

void JSBlockingMonitor::StopTimerReporting() { timer_stopped_ = true; }

}  // namespace performance
}  // namespace tasm
}  // namespace lynx
