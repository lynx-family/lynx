// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_LONG_TASK_TIMING_LONG_TASK_TIMING_H_
#define CORE_SERVICES_LONG_TASK_TIMING_LONG_TASK_TIMING_H_

#include <stdlib.h>

#include <atomic>
#include <string>
#include <utility>
#include <vector>

#include "core/services/event_report/event_tracker.h"

namespace lynx {
namespace tasm {
namespace timing {
struct LongTaskTiming {
  int64_t start_time_us_;
  int64_t end_time_us_;
  double duration_ms_;
  std::string thread_name_;
  std::string task_type_;
  std::string task_name_;
  std::string task_info_;
  int32_t instance_id_;
  bool is_active_;

  explicit LongTaskTiming(bool is_active = false) : is_active_(is_active) {}
  LongTaskTiming(int64_t start_time_us, const std::string& thread_name,
                 std::string task_type, std::string task_name,
                 std::string task_info = "",
                 int32_t instance_id = report::kUninitializedInstanceId,
                 bool is_active = true)
      : start_time_us_(start_time_us),
        thread_name_(thread_name),
        task_type_(std::move(task_type)),
        task_name_(std::move(task_name)),
        task_info_(std::move(task_info)),
        instance_id_(instance_id),
        is_active_(is_active) {}

  LongTaskTiming(const LongTaskTiming& s) = delete;
  LongTaskTiming& operator=(const LongTaskTiming&) = delete;
  LongTaskTiming(LongTaskTiming&&) = default;
  LongTaskTiming& operator=(LongTaskTiming&&) = default;
};
}  // namespace timing
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_LONG_TASK_TIMING_LONG_TASK_TIMING_H_
