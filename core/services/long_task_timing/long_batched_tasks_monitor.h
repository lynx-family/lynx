// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SERVICES_LONG_TASK_TIMING_LONG_BATCHED_TASKS_MONITOR_H_
#define CORE_SERVICES_LONG_TASK_TIMING_LONG_BATCHED_TASKS_MONITOR_H_

#include <stdlib.h>

#include <atomic>
#include <cmath>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include "base/include/thread/timed_task.h"
#include "core/services/long_task_timing/long_task_timing.h"

namespace lynx {
namespace tasm {
namespace timing {
/**
 * @class LongBatchedTasksMonitor
 *
 * @brief A monitor class that tracks and reports long batched tasks.
 *
 * This class is designed to monitor long batched tasks that consist of multiple
 * atomic tasks. It evaluates whether the aggregated duration and intervals
 * between tasks satisfy certain conditions, and then reports or clears the task
 * stack accordingly.
 *
 * Key functionalities of this class include:
 * - Receiving new atomic task timings.
 * - Aggregating tasks within a specified detection window.
 * - Reporting when the total duration exceeds a threshold.
 * - Clearing the task stack and stopping the timer when necessary.
 * - Managing intervals between tasks to ensure they do not exceed a maximum
 * allowed interval.
 *
 * The class uses a time window to group tasks and a timer to control the
 * maximum interval between tasks. If the conditions for a long batched task are
 * met, it initiates reporting. Otherwise, it continues evaluating the time
 * window until the criteria are satisfied.
 *
 * @note The thresholds and detection window size can be controlled via
 * settings.
 *
 * Example usage:
 * @code
 * LongBatchedTasksMonitor monitor("thread_name", 50, 1000, 0.8, 16.7);
 * LongTaskTiming timing = {start_time, end_time};
 * monitor.OnTaskTiming(timing);
 * @endcode
 */
class LongBatchedTasksMonitor
    : public std::enable_shared_from_this<LongBatchedTasksMonitor> {
 public:
  /**
   * @brief Constructor to initialize the monitor with various thresholds and
   * settings.
   *
   * @param thread_name The name of the thread this monitor operates on.
   * @param duration_threshold_ms The minimum duration threshold in
   * milliseconds.
   * @param detection_window_size_threshold_ms The size of the detection window
   * in milliseconds, default is 1000 ms.
   * @param duration_threshold_percentage The threshold percentage for
   * determining a long batched task, default is 0.8.
   * @param max_task_interval_ms The maximum interval between two adjacent tasks
   * in milliseconds, default is 16.7 ms.Since the timer's maximum precision is
   * milliseconds, the 0.7 milliseconds is discarded.
   */
  LongBatchedTasksMonitor(std::string thread_name, double duration_threshold_ms,
                          double detection_window_size_threshold_ms = 1000,
                          double duration_threshold_percentage = 0.8,
                          double max_task_interval_ms = 16.7)
      : detection_window_size_threshold_ms_(detection_window_size_threshold_ms),
        duration_threshold_percentage_(duration_threshold_percentage),
        duration_threshold_ms_(duration_threshold_ms),
        max_task_interval_ms_(max_task_interval_ms),
        all_tasks_duration_ms_(0.0),
        thread_name_(thread_name) {}
  /** Copy constructor is deleted to prevent copying */
  LongBatchedTasksMonitor(const LongBatchedTasksMonitor& timing) = delete;
  /** Assignment operator is deleted to prevent copying */
  LongBatchedTasksMonitor& operator=(const LongBatchedTasksMonitor&) = delete;
  /** Move constructor is deleted to prevent moving */
  LongBatchedTasksMonitor(LongBatchedTasksMonitor&&) = default;
  /** Move assignment operator is deleted to prevent moving */
  LongBatchedTasksMonitor& operator=(LongBatchedTasksMonitor&&) = default;

  /**
   * @brief Receives a new atomic task.
   *
   * @param timing The timing information of the atomic task.
   */
  void OnTaskTiming(LongTaskTiming timing);

  /**
   * @brief Receives a new long atomic task.
   */
  void OnLongTaskTiming();

 private:
  /**
   * @brief Checks if reporting conditions are met and reports if necessary.
   *
   * @return True if the conditions are met and reporting is performed,
   * otherwise false.
   */
  bool ReportIfNeed();

  /**
   * @brief Reports the long batched tasks event.
   */
  void ReportLongBatchedTasksEvent(double actual_window_size_ms);

  /**
   * @brief Clears the task stack and resets the total duration.
   */
  void ClearTaskStack();

  /**
   * @brief Evaluates the current time window and reports if the conditions are
   * met.
   *
   * If the conditions for a long batched task are not met, this function will
   * continue evaluating the time window until the criteria are satisfied or the
   * window size is less than the threshold.
   */
  void EvaluateTimeWindowAndReport();

  /**
   * @brief Checks if the interval between the last task in the stack and the
   * given task exceeds the maximum allowed interval.
   *
   * This method calculates the interval between the end time of the last task
   * in the stack and the start time of the given task. It then checks if this
   * interval exceeds the maximum allowed task interval.
   *
   * @param timing The timing information of the current task.
   * @return true if the interval exceeds the maximum allowed task interval,
   * false otherwise.
   */
  bool IsTaskIntervalExceeded(const LongTaskTiming& timing);

  /**
   * @brief Adds a new task to the stack and accumulates its duration to the
   * total duration.
   *
   * This method temporarily stores the new task in the stack and adds its
   * duration to the total duration of all tasks.
   *
   * @param current_task The new task to be added.
   */
  void AddTask(LongTaskTiming current_task);

  /**
   * @brief Returns the size of the current time window in milliseconds.
   *
   * @return The size of the current time window in milliseconds.
   */
  double TimeWindowSizeMs();

  // Storage for atomic tasks
  std::list<LongTaskTiming> timing_stack_;
  // Time window N, where N can be any value greater than 50. Default is 1
  // second and can be controlled via settings.
  double detection_window_size_threshold_ms_;
  // Threshold for determining a long batched task, with a range of (0, 1].
  // Default is 0.8 and can be controlled via settings.
  double duration_threshold_percentage_;
  // Minimum duration threshold
  double duration_threshold_ms_;
  // Maximum interval between two adjacent tasks. Default is 16.7 milliseconds
  // and can be controlled via settings.
  double max_task_interval_ms_;
  // Total duration of all tasks
  double all_tasks_duration_ms_;
  // The name of the thread this monitor operates on
  std::string thread_name_;
};

}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_LONG_TASK_TIMING_LONG_BATCHED_TASKS_MONITOR_H_
