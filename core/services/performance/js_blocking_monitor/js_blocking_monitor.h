// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_JS_BLOCKING_MONITOR_JS_BLOCKING_MONITOR_H_
#define CORE_SERVICES_PERFORMANCE_JS_BLOCKING_MONITOR_JS_BLOCKING_MONITOR_H_

#include <memory>
#include <string>

#include "base/include/timer/time_utils.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/performance/js_blocking_monitor/js_blocking_trace_event_def.h"
#include "core/services/performance/performance_event_sender.h"

namespace lynx {
namespace tasm {
namespace performance {
static constexpr char kJSBlockingStageLoadBundle[] = "loadBundle";
static constexpr char kJSBlockingStageTimer[] = "timer";

struct JSTaskEnqueueRetInfo {
  uint64_t enqueue_time;
};

// @class JSBlockingMonitor
// @brief A class for monitoring JavaScript blocking time and reporting blocking
// information.
//
// The JSBlockingMonitor class provides functionality to track JavaScript
// blocking time and report blocking information with specific event types and
// durations.
class JSBlockingMonitor
    : public std::enable_shared_from_this<JSBlockingMonitor> {
 public:
  explicit JSBlockingMonitor(PerformanceEventSender* observer)
      : sender_(observer) {
    last_report_time_ = GetNowTimeMs();
  };
  ~JSBlockingMonitor() = default;
  JSBlockingMonitor(const JSBlockingMonitor& monitor) = delete;
  JSBlockingMonitor& operator=(const JSBlockingMonitor&) = delete;
  JSBlockingMonitor(JSBlockingMonitor&& other) = delete;
  JSBlockingMonitor& operator=(JSBlockingMonitor&& other) = delete;

  // Adds blocking time with a millisecond timestamp.
  // @param timestamp_ms The millisecond timestamp of the blocking event.
  void AddBlockingTime(int64_t duration_ms);

  // Reports blocking information when setup event.
  // @param event entry
  void OnPerformanceEvent(const std::unique_ptr<pub::Value>& entry_map);

  // Checks if JS blocking monitoring is enabled.
  // Modules can call this before collecting data to avoid unnecessary
  // collection.
  static bool Enable();

  // Returns the threshold in milliseconds for JS blocking monitoring.
  // @return The threshold in milliseconds.
  static int32_t GetThresholdMs();

  // Returns the current time in milliseconds.
  // @return The current time in milliseconds.
  static inline uint64_t GetNowTimeMs() {
    if (!Enable()) {
      return 0;
    }
    return base::CurrentTimeMilliseconds();
  };

  // Returns the interval in milliseconds for JS blocking report timer.
  // @return The interval in milliseconds.
  static int32_t GetReportIntervalMs();

  // Mark start use trace instant
  // @return The trace flow_id.
  static JSTaskEnqueueRetInfo MarkJSTaskEnqueue();

 private:
  // Reports blocking information with stage and total duration.
  // @param stage The stage of event ("fsp" or "timer").
  // @param total_duration_ms The total duration in milliseconds for the current
  // stage.
  void ReportBlockingInfo(const std::string& stage);

  void ReportWithTimer(int8_t index);
  PerformanceEventSender* sender_;
  int64_t total_blocking_time_ = 0;
  int64_t total_blocking_count_ = 0;
  int64_t last_report_time_ = 0;
  int64_t load_bundle_time_ = 0;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_JS_BLOCKING_MONITOR_JS_BLOCKING_MONITOR_H_
