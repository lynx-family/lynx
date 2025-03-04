// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/time/timer.h"

#include <utility>

#include "base/include/fml/macros.h"

namespace lynx {
namespace fml {

void Timer::Start(fml::TimeDelta delay, Task task) {
  if (!task || !task_runner_) {
    LYNX_BASE_DCHECK(false);
    return;
  }

  delay_ = delay;
  LYNX_BASE_DCHECK(delay_ != fml::TimeDelta::Zero());
  user_task_ = std::move(task);
  running_ = true;
  AbandonScheduledTasks();
  ScheduleNewTask();
}

void Timer::AbandonScheduledTasks() {
  // Overflow will be reset to 0.
  ++validator_;
}

void Timer::ScheduleNewTask() {
  task_runner_->PostDelayedTask(
      [validator = validator_, self = weak_factory_.GetWeakPtr()]() {
        if (!self || !self->running_ || validator != self->validator_) {
          return;
        }

        if (self->repeating_) {
          self->ScheduleNewTask();
        } else {
          self->ResetState();
        }

        // Must run after schedule. Otherwise timer maybe destructed.
        self->RunUserTask();
      },
      delay_);
}

// Whence stopped, all scheduled tasks are invalidated.
void Timer::Stop() {
  ResetState();
  user_task_ = nullptr;
}

void Timer::ResetState() {
  running_ = false;
  AbandonScheduledTasks();
}

void Timer::RunUserTask() {
  if (user_task_) {
    user_task_();
  }
}

bool OneshotTimer::FireImmediately() {
  if (!Stopped()) {
    ResetState();
    RunUserTask();
    return true;
  }
  return false;
}

}  // namespace fml
}  // namespace lynx
