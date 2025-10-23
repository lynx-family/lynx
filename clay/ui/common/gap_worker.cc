// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/common/gap_worker.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/rect.h"
#include "clay/gfx/style/color.h"
#include "clay/ui/common/gap_task.h"

namespace clay {

void GapWorker::CollectTaskIfNeeded() {
  FML_DCHECK(ui_task_runner_->RunsTasksOnCurrentThread());
  TRACE_EVENT("clay", "GapWorker::CollectTaskIfNeeded");
  for (const auto& kv : collectors_) {
    kv.second();
  }
}

void GapWorker::RegisterTaskCollector(BaseView* view,
                                      GapTaskCollector collector) {
  FML_DCHECK(view);
  collectors_[view] = collector;
}

void GapWorker::UnregisterTaskCollector(BaseView* view) {
  collectors_.erase(view);
  CancelTask(view);
}

void GapWorker::SubmitTask(fml::RefPtr<GapTaskBundle> tasks) {
  if (tasks->IsValid() && !tasks->IsEmpty()) {
    task_map_[tasks->host_view().get()] = tasks;
    data_changed_ = true;
  }
}

void GapWorker::CancelTask(BaseView* host) {
  task_map_.erase(host);
  data_changed_ = true;
}

void GapWorker::FlushTask(const fml::TimePoint& end_time) {
  CollectTaskIfNeeded();

  if (task_map_.empty()) {
    return;
  }
  auto start_time = fml::TimePoint::Now();
  if (start_time > end_time) {
    return;
  }
  TRACE_EVENT("clay", "FlushTask");
  if (data_changed_) {
    data_changed_ = false;
    last_task_list_.clear();
    for (auto itr = task_map_.begin(); itr != task_map_.end();) {
      if (!itr->second || !itr->second->IsValid() || itr->second->IsEmpty()) {
        itr = task_map_.erase(itr);
        continue;
      }
      last_task_list_.emplace_back(itr->second);
      itr++;
    }
    if (last_task_list_.size() > 1) {
      std::sort(last_task_list_.begin(), last_task_list_.end(),
                GapTaskBundle::GapTaskBundleComparator());
    }
    // change data might cost a little time, so we need to update start time.
    start_time = fml::TimePoint::Now();
  }

  int64_t current_interval_ns = (end_time - start_time).ToNanoseconds();
  for (auto itr = last_task_list_.begin(); itr != last_task_list_.end();) {
    TRACE_EVENT("clay", "HandleTaskBundle");
    if (current_interval_ns < 0) {
      return;
    }
    if (!(*itr)->IsValid() || (*itr)->IsEmpty()) {
      itr = last_task_list_.erase(itr);
      continue;
    }
    for (auto task = (*itr)->begin(); task != (*itr)->end();) {
      auto item_duration = (*task)->estimate_time();
      // Some task is very heavy and its estimate time might longer than
      // frame interval (e.x. 25ms > 16.7ms), for these tasks, we will check
      // if we can run it forcibly .
      //
      // First check if estimate duration is less than our current idle interval
      if (item_duration > current_interval_ns) {
        // Then check if the task enable force run.
        if (!(*task)->enable_force_run() ||
            current_interval_ns < max_estimate_duration_) {
          // We can't run this task queue, try another one.
          break;
        }
      }
      (*task)->Run();
      task = (*itr)->erase(task);
      current_interval_ns = (end_time - fml::TimePoint::Now()).ToNanoseconds();
    }
    itr++;
  }
}

}  // namespace clay
