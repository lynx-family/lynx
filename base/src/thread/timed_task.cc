// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/thread/timed_task.h"

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/message_loop.h"

namespace lynx {
namespace base {

TimedTaskManager::TimedTaskManager(bool need_stop_all_tasks_when_exit,
                                   fml::RefPtr<fml::TaskRunner> runner)
    : runner_(runner ? runner : fml::MessageLoop::GetCurrent().GetTaskRunner()),
      need_stop_all_tasks_when_exit_(need_stop_all_tasks_when_exit) {}

TimedTaskManager::~TimedTaskManager() {
  if (need_stop_all_tasks_when_exit_) {
    StopAllTasks();
  }
}

uint32_t TimedTaskManager::SetTimeout(closure closure, int64_t delay) {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  auto controller = std::make_unique<Controller>(std::move(closure));
  controllers_.emplace(++current_, controller.get());

  runner_->PostDelayedTask(
      fml::MakeCopyable([this, current = current_,
                         controller = std::move(controller)]() mutable {
        if (controller->closure) {
          Scope scope(this, current);
          controller->closure();
          controllers_.erase(current);
        }
      }),
      fml::TimeDelta::FromMilliseconds(delay));

  return current_;
}

uint32_t TimedTaskManager::SetInterval(closure closure, int64_t delay) {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  auto controller = std::make_unique<Controller>(std::move(closure));
  controllers_.emplace(++current_, controller.get());
  SetInterval(std::move(controller), delay, current_);
  return current_;
}

void TimedTaskManager::SetInterval(
    std::unique_ptr<TimedTaskManager::Controller> controller, int64_t delay,
    uint32_t current) {
  runner_->PostDelayedTask(
      fml::MakeCopyable(
          [this, controller = std::move(controller), delay, current]() mutable {
            if (controller->closure) {
              Scope scope(this, current, true);
              auto& closure = controller->closure;
              SetInterval(std::move(controller), delay, current);
              closure();
            }
          }),
      fml::TimeDelta::FromMilliseconds(delay));
}

void TimedTaskManager::StopTask(uint32_t id) {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  // the return value of setTimeout and setInterval begins with 1,
  // which means "id == 0" here is invalid
  if (id == 0) {
    return;
  }

  // need pending remove task when it is executing
  if (current_executing_task_ == id) {
    has_pending_remove_task_ = true;
    return;
  }

  auto iter = controllers_.find(id);
  if (iter == controllers_.end()) {
    return;
  }

  auto* controller = iter->second;
  controller->closure = nullptr;

  controllers_.erase(iter);
}

void TimedTaskManager::StopAllTasks() {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  for (auto& [id, controller] : controllers_) {
    controller->closure = nullptr;
  }

  controllers_.clear();
}

TimedTaskManager::Scope::Scope(TimedTaskManager* manager, uint32_t current,
                               bool is_interval)
    : manager_(manager), is_interval_(is_interval) {
  manager_->current_executing_task_ = current;
}

TimedTaskManager::Scope::~Scope() {
  uint32_t current = manager_->current_executing_task_;
  manager_->current_executing_task_ = 0;

  // for interval task, need delay stop here.
  // for non-interval, do nothing, because it has been removed.
  if (is_interval_ && manager_->has_pending_remove_task_) {
    manager_->StopTask(current);
  }

  manager_->has_pending_remove_task_ = false;
}

}  // namespace base
}  // namespace lynx
