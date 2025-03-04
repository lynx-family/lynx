// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_TIME_TIMER_H_
#define BASE_INCLUDE_FML_TIME_TIMER_H_

#include <functional>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/time_delta.h"

namespace lynx {
namespace fml {

// Timer util based to fml::TaskRunner. NOT thread-safe.
// As most functionality of oneshot timer can be substituted by PostDelayTask,
// but repeating tasks or cancelable oneshot task are not so convenient to do.
// So here provide a simple repeating / oneshot cancalable timer for caret
// twinkling or swiper autoplay and so on.
// NOTE: The actual time delay may giant than expected, and it won't be fixed.
class Timer {
 public:
  // Task runner should be same with calling thread.
  Timer(fml::RefPtr<fml::TaskRunner> task_runner, bool repeat)
      : task_runner_(std::move(task_runner)), repeating_(repeat) {}

  virtual ~Timer() { Stop(); }

  using Task = std::function<void()>;

  // Start a timer with delay.
  // First task will be fired after |delay| rather than right now.
  // It can be called multiple times. Once called, the previous scheduled task
  // will be invalidated and delay time will be reset.
  void Start(fml::TimeDelta delay, Task task);

  // Whence stopped, all scheduled tasks are invalidated.
  void Stop();

  bool Stopped() const { return !running_; }

 protected:
  void RunUserTask();
  void ResetState();

 private:
  void AbandonScheduledTasks();

  void ScheduleNewTask();

 private:
  fml::RefPtr<fml::TaskRunner> task_runner_;
  const bool repeating_ = true;
  Task user_task_;
  fml::TimeDelta delay_;

  // Used for invalidate scheduled tasks.
  // Not 100% safe if there're more than numeric_limits<uint64_t>::max() tasks
  // scheduled within one period of delay.
  uint64_t validator_ = 0;
  bool running_ = false;

  fml::WeakPtrFactory<Timer> weak_factory_{this};
};

class RepeatingTimer : public Timer {
 public:
  explicit RepeatingTimer(fml::RefPtr<fml::TaskRunner> task_runner)
      : Timer(task_runner, true) {}
};

class OneshotTimer : public Timer {
 public:
  explicit OneshotTimer(fml::RefPtr<fml::TaskRunner> task_runner)
      : Timer(task_runner, false) {}

  // Fire now if task has not been fired ever. After fire, timer will be reset.
  bool FireImmediately();
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::OneshotTimer;
using lynx::fml::RepeatingTimer;
using lynx::fml::Timer;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_TIME_TIMER_H_
