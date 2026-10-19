// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/thread/timed_task.h"

#include <utility>

#include "base/include/fml/make_copyable.h"
#include "base/include/fml/memory/js_memory_track_scope.h"
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
  PostTimeout(std::move(controller), delay, current_,
              runner_->GetCurrentAllocSlot());
  return current_;
}

void TimedTaskManager::PostTimeout(
    std::unique_ptr<TimedTaskManager::Controller> controller, int64_t delay,
    uint32_t current, int32_t alloc_slot) {
  runner_->PostDelayedTask(
      fml::MakeCopyable([this, current, alloc_slot,
                         controller = std::move(controller)]() mutable {
        if (controller->closure) {
          if (paused_) {
            deferred_tasks_.push_back(
                {current, std::move(controller), false, 0, alloc_slot});
            return;
          }
          Scope scope(this, current);
          fml::JSMemoryTrackSlot alloc_slot_scope(this->runner_.get(),
                                                  alloc_slot);
          controller->closure();
          controllers_.erase(current);
        }
      }),
      fml::TimeDelta::FromMilliseconds(delay));
}

uint32_t TimedTaskManager::SetInterval(closure closure, int64_t delay) {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  auto controller = std::make_unique<Controller>(std::move(closure));
  controllers_.emplace(++current_, controller.get());
  SetInterval(std::move(controller), delay, current_,
              runner_->GetCurrentAllocSlot(), delay);
  return current_;
}

void TimedTaskManager::SetInterval(
    std::unique_ptr<TimedTaskManager::Controller> controller, int64_t delay,
    uint32_t current, int32_t alloc_slot, int64_t first_delay) {
  runner_->PostDelayedTask(
      fml::MakeCopyable([this, alloc_slot, controller = std::move(controller),
                         delay, current]() mutable {
        if (controller->closure) {
          if (paused_) {
            // Stop rescheduling while paused, the interval restarts on Resume.
            deferred_tasks_.push_back(
                {current, std::move(controller), true, delay, alloc_slot});
            return;
          }
          Scope scope(this, current, true);
          fml::JSMemoryTrackSlot alloc_slot_scope(this->runner_.get(),
                                                  alloc_slot);
          auto& closure = controller->closure;
          SetInterval(std::move(controller), delay, current, alloc_slot, delay);
          closure();
        }
      }),
      fml::TimeDelta::FromMilliseconds(first_delay));
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
  deferred_tasks_.clear();
}

void TimedTaskManager::Pause() {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  paused_ = true;
}

void TimedTaskManager::Resume() {
  // DCHECK(runner_->RunsTasksOnCurrentThread());
  if (!paused_) {
    return;
  }
  paused_ = false;

  // Post instead of running them here to keep the callbacks out of the caller's
  // stack. Tasks stopped while deferred have a null closure and are dropped
  // when they are posted.
  auto deferred_tasks = std::move(deferred_tasks_);
  deferred_tasks_.clear();
  for (auto& task : deferred_tasks) {
    if (task.is_interval) {
      SetInterval(std::move(task.controller), task.interval, task.id,
                  task.alloc_slot, 0);
    } else {
      PostTimeout(std::move(task.controller), 0, task.id, task.alloc_slot);
    }
  }
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
