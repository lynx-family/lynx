// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/long_task_timing/long_task_monitor.h"

#include <utility>
#include <vector>

#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/base/thread/thread_utils.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/services/event_report/event_tracker.h"
#include "core/services/event_report/event_tracker_platform_impl.h"

namespace lynx {
namespace tasm {
namespace timing {

LongTaskMonitor* LongTaskMonitor::Instance() {
  static thread_local LongTaskMonitor instance_;
  return &instance_;
}

LongTaskMonitor::LongTaskMonitor()
    : enable_(LynxEnv::GetInstance().EnableLongTaskTiming()),
      duration_threshold_ms_(50),
      thread_name_(base::GetCurrentThreadName()),
      long_batched_tasks_monitor_(
          LongBatchedTasksMonitor(thread_name_, duration_threshold_ms_)) {}

void LongTaskMonitor::WillProcessTask(const std::string& type,
                                      const std::string& name,
                                      const std::string& task_info,
                                      int32_t instance_id) {
  if (!enable_ || instance_id < 0) {
    return;
  }
  // When atomic tasks are nested, the same time period may report multiple
  // long-duration tasks
  // and task intervals may exceed 16.7 milliseconds, causing premature long
  // batched task judgments. To solve this, we check the number of elements in
  // the stack and whether the elements are active.
  if (timing_stack_.empty()) {
    timing_stack_.push(LongTaskTiming(base::CurrentTimeMicroseconds(),
                                      thread_name_, type, name, task_info,
                                      instance_id, true));
  } else {
    timing_stack_.push(LongTaskTiming(false));
  }
}

void LongTaskMonitor::DidProcessTask() {
  if (!enable_ || timing_stack_.empty()) {
    return;
  }
  LongTaskTiming& timing = timing_stack_.top();
  if (!timing.is_active_) {
    timing_stack_.pop();
    return;
  }
  timing.end_time_us_ = base::CurrentTimeMicroseconds();
  timing.duration_ms_ =
      std::round(timing.end_time_us_ - timing.start_time_us_) / 1000.0;
  if (duration_threshold_ms_ <= timing.duration_ms_) {
    // to report
    TRACE_EVENT(
        LYNX_TRACE_CATEGORY, "LongTaskTiming::ReportLongTask",
        [&timing](lynx::perfetto::EventContext ctx) {
          ctx.event()->add_debug_annotations("thread_name",
                                             timing.thread_name_);
          ctx.event()->add_debug_annotations("task_name", timing.task_name_);
          ctx.event()->add_debug_annotations("task_type", timing.task_type_);
          ctx.event()->add_debug_annotations("task_info", timing.task_info_);
          ctx.event()->add_debug_annotations(
              "duration_ms", std::to_string(timing.duration_ms_));
        });
    tasm::report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
        [timing = std::move(timing),
         duration_threshold_ms = duration_threshold_ms_] {
          tasm::report::MoveOnlyEvent event;
          event.SetName("lynxsdk_long_task_timing");
          event.SetProps("duration_ms", timing.duration_ms_);
          event.SetProps("duration_threshold_ms", duration_threshold_ms);
          event.SetProps("task_type", timing.task_type_);
          event.SetProps("task_name", timing.task_name_);
          event.SetProps("task_info", timing.task_info_);
          event.SetProps("thread_name", timing.thread_name_);
          tasm::report::EventTrackerPlatformImpl::OnEvent(timing.instance_id_,
                                                          std::move(event));
        });
    long_batched_tasks_monitor_.OnLongTaskTiming();
  } else {
    long_batched_tasks_monitor_.OnTaskTiming(std::move(timing));
  }
  timing_stack_.pop();
}

LongTaskTiming* LongTaskMonitor::GetTopTimingPtr() {
  if (!enable_ || timing_stack_.empty()) {
    return nullptr;
  }
  auto& timing = timing_stack_.top();
  if (!timing.is_active_) {
    return nullptr;
  }
  return &(timing);
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
