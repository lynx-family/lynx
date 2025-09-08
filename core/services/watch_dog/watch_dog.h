// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_
#define CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_

#include <unistd.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/thread.h"
#include "base/include/lynx_actor.h"
#include "base/include/timer/time_utils.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace shell {

class WatchDog final {
 public:
  struct TaskConfig {
    uint32_t delay = 50 * 1000;
    uint32_t allow_delay = 1.5 * 1000;
    uint32_t step = 2;
    base::MoveOnlyClosure<void> idle_task;
  };

 private:
  template <typename T>
  static void RunOnIdle(TaskConfig&& config,
                        const std::shared_ptr<T>& target_actor) {
    uint32_t delay = config.delay;
    uint32_t allow_delay = config.allow_delay;
    uint32_t step = config.step;

    uint32_t counter = 0;
    while (counter < step) {
      usleep(delay / step);
      counter++;
      uint64_t start_time = base::CurrentTimeMicroseconds();
      uint64_t end_time = target_actor->ActSync(
          [](auto& t) { return base::CurrentTimeMicroseconds(); });

      if (end_time - start_time > allow_delay) {
        counter = 0;
      }
    }

    target_actor->Act(
        [task = std::move(config.idle_task)](auto& t) { task(); });
  }

  fml::RefPtr<fml::TaskRunner> watch_dog_runner_;

 public:
  WatchDog() {
    static base::NoDestructor<fml::Thread> watch_dog_thread(
        fml::Thread::ThreadConfig("lynx_watch_dog",
                                  fml::Thread::ThreadPriority::NORMAL));

    watch_dog_runner_ = watch_dog_thread->GetTaskRunner();
  }
  ~WatchDog() = default;

  template <typename T>
  void RunOnActorThreadIdle(TaskConfig&& config,
                            const std::shared_ptr<T>& mts_actor) {
    watch_dog_runner_->PostTask(
        [config = std::move(config), actor = mts_actor]() mutable {
          RunOnIdle(std::move(config), actor);
        });
  }
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_
