// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/long_task_timing/long_batched_tasks_monitor.h"

#include <algorithm>
#include <list>
#include <unordered_map>
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

bool LongBatchedTasksMonitor::ReportIfNeed() {
  if (timing_stack_.size() <= 1) {
    // [break] When the number of tasks is less than two, the conditions for
    // long batched tasks cannot be met.
    return false;
  }
  double actual_window_size_ms = TimeWindowSizeMs();
  if (all_tasks_duration_ms_ < duration_threshold_ms_ ||
      all_tasks_duration_ms_ <
          actual_window_size_ms * duration_threshold_percentage_) {
    // [break] When the time taken is less than 'duration_threshold_ms_' or less
    // than 'actual_window_size_ms * duration_threshold_percentage_',
    // the conditions for long batched tasks cannot be met.
    return false;
  }
  ReportLongBatchedTasksEvent(actual_window_size_ms);
  return true;
}

void LongBatchedTasksMonitor::ReportLongBatchedTasksEvent(
    double actual_window_size_ms) {
  report::EventTrackerPlatformImpl::GetReportTaskRunner()->PostTask(
      [timing_stack = std::move(timing_stack_), actual_window_size_ms,
       all_tasks_duration_ms = all_tasks_duration_ms_,
       detection_window_size_threshold_ms = detection_window_size_threshold_ms_,
       duration_threshold_percentage = duration_threshold_percentage_,
       duration_threshold_ms = duration_threshold_ms_,
       max_task_interval_ms = max_task_interval_ms_,
       thread_name = thread_name_]() mutable {
        // 2. Find the top 3 longest task timings
        LongTaskTiming* top_1_timing = nullptr;
        LongTaskTiming* top_2_timing = nullptr;
        LongTaskTiming* top_3_timing = nullptr;

        std::unordered_map<int32_t, double> instance_id_duration_map;
        int32_t tasks_count = 0;

        for (auto& timing : timing_stack) {
          if (!top_1_timing ||
              timing.duration_ms_ > top_1_timing->duration_ms_) {
            top_3_timing = top_2_timing;
            top_2_timing = top_1_timing;
            top_1_timing = &timing;
          } else if (!top_2_timing ||
                     timing.duration_ms_ > top_2_timing->duration_ms_) {
            top_3_timing = top_2_timing;
            top_2_timing = &timing;
          } else if (!top_3_timing ||
                     timing.duration_ms_ > top_3_timing->duration_ms_) {
            top_3_timing = &timing;
          }
          // Accumulate the durations for each instance_id
          instance_id_duration_map[timing.instance_id_] += timing.duration_ms_;
          // Accumulate task count
          tasks_count++;
        }

        // 3. Calculate average duration
        double average_duration_ms = all_tasks_duration_ms / tasks_count;

        // 4. Find the instance_id with the maximum total duration
        int32_t max_duration_instance_id =
            tasm::report::kUninitializedInstanceId;
        double max_duration = 0.0;
        for (const auto [instance_id, duration] : instance_id_duration_map) {
          if (duration > max_duration) {
            max_duration = duration;
            max_duration_instance_id = instance_id;
          }
        }

        // 5. get count of instance id.
        uint64_t instance_id_count =
            static_cast<uint64_t>(instance_id_duration_map.size());

        // Report the event with calculated statistics
        report::MoveOnlyEvent event;
        event.SetName("lynxsdk_long_batched_tasks_timing");
        event.SetProps("all_tasks_duration_ms", all_tasks_duration_ms);
        event.SetProps("time_window_size_threshold_ms",
                       detection_window_size_threshold_ms);
        event.SetProps("actual_time_window_size_ms", actual_window_size_ms);
        event.SetProps("duration_threshold_percentage",
                       duration_threshold_percentage);
        event.SetProps("minimum_all_tasks_duration_threshold_ms",
                       duration_threshold_ms);
        event.SetProps("max_task_interval_ms", max_task_interval_ms);
        event.SetProps("thread_name", thread_name.c_str());

        event.SetProps("tasks_count", tasks_count);
        event.SetProps("lynx_view_instance_count", instance_id_count);
        event.SetProps("average_duration_ms", average_duration_ms);
        // Set the properties for the top 3 longest tasks
        if (top_1_timing) {
          event.SetProps("top_1_task_type", top_1_timing->task_type_.c_str());
          event.SetProps("top_1_task_name", top_1_timing->task_name_.c_str());
          event.SetProps("top_1_task_info", top_1_timing->task_info_.c_str());
          event.SetProps("top_1_task_duration_ms", top_1_timing->duration_ms_);
        }
        if (top_2_timing) {
          event.SetProps("top_2_task_type", top_2_timing->task_type_.c_str());
          event.SetProps("top_2_task_name", top_2_timing->task_name_.c_str());
          event.SetProps("top_2_task_info", top_2_timing->task_info_.c_str());
          event.SetProps("top_2_task_duration_ms", top_2_timing->duration_ms_);
        }
        if (top_3_timing) {
          event.SetProps("top_3_task_type", top_3_timing->task_type_.c_str());
          event.SetProps("top_3_task_name", top_3_timing->task_name_.c_str());
          event.SetProps("top_3_task_info", top_3_timing->task_info_.c_str());
          event.SetProps("top_3_task_duration_ms", top_3_timing->duration_ms_);
        }
        report::EventTrackerPlatformImpl::OnEvent(max_duration_instance_id,
                                                  std::move(event));
      });
}

void LongBatchedTasksMonitor::ClearTaskStack() {
  timing_stack_.clear();
  all_tasks_duration_ms_ = 0.0;
}

void LongBatchedTasksMonitor::OnTaskTiming(LongTaskTiming current_task) {
  if (IsTaskIntervalExceeded(current_task)) {
    // 1.1 Attempt to report
    ReportIfNeed();
    // 1.2 Clear stack
    ClearTaskStack();
    // 1.3 Adds a new task to the stack and accumulates its duration.
    AddTask(std::move(current_task));
    return;
  }
  // 2. Adds a new task to the stack and accumulates its duration.
  AddTask(std::move(current_task));
  // 3. Evaluate the time window and report if necessary
  EvaluateTimeWindowAndReport();
}

void LongBatchedTasksMonitor::OnLongTaskTiming() {
  ReportIfNeed();
  ClearTaskStack();
}

void LongBatchedTasksMonitor::EvaluateTimeWindowAndReport() {
  // 1. Check if the time window threshold is met
  if (TimeWindowSizeMs() < detection_window_size_threshold_ms_) {
    return;
  }
  // 2. Check if reporting conditions are met
  if (ReportIfNeed()) {  // If reporting conditions are met
    ClearTaskStack();
    return;
  }
  // 3.0 If reporting conditions are not met
  auto& front_task = timing_stack_.front();
  all_tasks_duration_ms_ -= front_task.duration_ms_;
  // 3.1 Remove the first task
  timing_stack_.pop_front();
  // 3.2 Continue to evaluate until reporting conditions are met or the time
  // window is less than the threshold
  EvaluateTimeWindowAndReport();
}

bool LongBatchedTasksMonitor::IsTaskIntervalExceeded(
    const LongTaskTiming& timing) {
  if (timing_stack_.empty()) {
    return false;
  }
  auto& last_task = timing_stack_.back();
  double interval = (timing.start_time_us_ - last_task.end_time_us_) / 1000.0;
  return interval >= max_task_interval_ms_;
}

void LongBatchedTasksMonitor::AddTask(LongTaskTiming current_task) {
  timing_stack_.push_back(std::move(current_task));
  all_tasks_duration_ms_ += current_task.duration_ms_;
}

double LongBatchedTasksMonitor::TimeWindowSizeMs() {
  if (timing_stack_.size() > 0) {
    auto& first_task = timing_stack_.front();
    auto& last_task = timing_stack_.back();
    return (last_task.end_time_us_ - first_task.start_time_us_) / 1000.0;
  }
  return 0;
}

}  // namespace timing
}  // namespace tasm
}  // namespace lynx
