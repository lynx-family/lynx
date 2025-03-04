// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_THREAD_TIMED_TASK_H_
#define BASE_INCLUDE_THREAD_TIMED_TASK_H_

#include <memory>
#include <unordered_map>
#include <utility>

#include "base/include/base_export.h"
#include "base/include/closure.h"
#include "base/include/fml/task_runner.h"

namespace lynx {
namespace base {

// not thread safe, need ensure lifecycle on one thread forever.
class TimedTaskManager {
 public:
  explicit TimedTaskManager(bool need_stop_all_tasks_when_exit = true,
                            fml::RefPtr<fml::TaskRunner> runner = nullptr);
  ~TimedTaskManager();

  uint32_t SetTimeout(closure closure, int64_t delay);

  uint32_t SetInterval(closure closure, int64_t delay);

  void StopTask(uint32_t id);

  void StopAllTasks();

 private:
  struct Controller {
    explicit Controller(closure other) : closure(std::move(other)) {}

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;

    closure closure;
  };

  // need forbid StopTask in execute timed task,
  // or delete this will cause crash.
  // just use a scope to control.
  class Scope {
   public:
    Scope(TimedTaskManager* manager, uint32_t current,
          bool is_interval = false);
    ~Scope();

   private:
    TimedTaskManager* manager_;
    bool is_interval_;
  };

  void SetInterval(std::unique_ptr<TimedTaskManager::Controller> controller,
                   int64_t delay, uint32_t current);

  uint32_t current_ = 0;

  std::unordered_map<uint32_t, Controller*> controllers_;

  // bind to thread which TimedTaskManager created.
  fml::RefPtr<fml::TaskRunner> runner_;

  uint32_t current_executing_task_{0};

  bool has_pending_remove_task_{false};

  // TODO(heshan.pro): Try to avoid crash when the thread is released and needs
  // to be deleted TimedTaskManager after refactor.
  bool need_stop_all_tasks_when_exit_{true};
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_THREAD_TIMED_TASK_H_
