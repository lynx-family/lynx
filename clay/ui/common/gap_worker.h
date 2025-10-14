// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_GAP_WORKER_H_
#define CLAY_UI_COMMON_GAP_WORKER_H_

#include <functional>
#include <unordered_map>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "clay/ui/common/gap_task.h"
#include "clay/ui/component/css_property.h"

namespace clay {

using GapTaskCollector = std::function<void()>;
class BaseView;

class GapWorker {
 public:
  explicit GapWorker(fml::RefPtr<fml::TaskRunner> ui_task_runner)
      : ui_task_runner_(ui_task_runner) {
    max_estimate_duration_ =
        fml::TimeDelta::FromMilliseconds(8).ToNanoseconds();
  }

  void CollectTaskIfNeeded();
  void RegisterTaskCollector(BaseView* view, GapTaskCollector collector);
  void UnregisterTaskCollector(BaseView* view);
  void SubmitTask(fml::RefPtr<GapTaskBundle> tasks);
  void CancelTask(BaseView* host);
  void FlushTask(const fml::TimePoint& end_time);
  void SetRefreshRate(uint32_t refresh_rate) {
    max_estimate_duration_ = 1.0E9F / refresh_rate / 2;
  }
  bool HasGapTask() const { return !collectors_.empty(); }

 private:
  std::unordered_map<BaseView*, GapTaskCollector> collectors_;
  std::unordered_map<BaseView*, fml::RefPtr<GapTaskBundle>> task_map_;
  std::vector<fml::RefPtr<GapTaskBundle>> last_task_list_;
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
  int64_t max_estimate_duration_;
  bool data_changed_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_GAP_WORKER_H_
