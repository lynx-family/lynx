// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_GAP_TASK_H_
#define CLAY_UI_COMMON_GAP_TASK_H_

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/memory/weak_ptr.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/list_recycler.h"

namespace clay {

class GapTask {
 public:
  GapTask(fml::WeakPtr<BaseView> host, int32_t id, int32_t estimate_time,
          int32_t priority = -1, bool enable_force_run = false)
      : id_(id),
        estimate_time_(estimate_time),
        priority_(priority),
        host_view_(host),
        enable_force_run_(enable_force_run) {}
  virtual ~GapTask() = default;
  virtual void Run() = 0;
  auto estimate_time() const { return estimate_time_; }
  auto priority() const { return priority_; }
  auto id() const { return id_; }
  auto enable_force_run() const { return enable_force_run_; }

  struct GapTaskComparator {
    bool operator()(const std::unique_ptr<GapTask>& l,
                    const std::unique_ptr<GapTask>& r) const {
      return l->priority() < r->priority();
    }
  };

 protected:
  int32_t id_;
  int32_t estimate_time_ = 0;
  int32_t priority_ = -1;
  fml::WeakPtr<BaseView> host_view_;
  // Indicate that this task should be run even if the idle time is not enough.
  // But the actual behavior will be depended on the actual gap worker
  // implementation.
  bool enable_force_run_;
};

class GapTaskBundle : public fml::RefCountedThreadSafe<GapTaskBundle> {
 public:
  explicit GapTaskBundle(fml::WeakPtr<BaseView> host) : host_view_(host) {}
  void AddTask(std::unique_ptr<GapTask> task) {
    priority_ = std::min(priority_, task->priority());
    tasks_.push_back(std::move(task));
  }
  bool IsEmpty() const { return tasks_.empty(); }
  bool IsValid() const { return static_cast<bool>(host_view_); }
  void Clear() { tasks_.clear(); }
  auto begin() { return tasks_.begin(); }
  auto end() { return tasks_.end(); }
  void sort() {
    std::sort(tasks_.begin(), tasks_.end(), GapTask::GapTaskComparator());
  }
  auto erase(std::vector<std::unique_ptr<GapTask>>::iterator itr) {
    return tasks_.erase(itr);
  }
  fml::WeakPtr<BaseView> host_view() const { return host_view_; }
  auto priority() const { return priority_; }

  struct GapTaskBundleComparator {
    bool operator()(const fml::RefPtr<GapTaskBundle>& l,
                    const fml::RefPtr<GapTaskBundle>& r) const {
      return l->priority() < r->priority();
    }
  };

 private:
  int32_t priority_ = std::numeric_limits<int32_t>::max();
  fml::WeakPtr<BaseView> host_view_;
  std::vector<std::unique_ptr<GapTask>> tasks_;

  FML_FRIEND_MAKE_REF_COUNTED(GapTaskBundle);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(GapTaskBundle);
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_GAP_TASK_H_
