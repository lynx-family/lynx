// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_
#define CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/thread.h"
#include "base/include/lynx_actor.h"
#include "base/include/timer/time_utils.h"
#include "core/base/lynx_trace_categories.h"

#if OS_WIN
#include <Windows.h>

inline void lynx_shell_watch_dog_usleep(uint32_t num) {
  SleepEx(num < 500 ? 1 : (num + 500) / 1000, true);
}
#else
#include <unistd.h>

inline void lynx_shell_watch_dog_usleep(uint32_t num) { usleep(num); }
#endif

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
  static const fml::RefPtr<fml::TaskRunner>& GetWatchDogTaskRunner() {
    static base::NoDestructor<fml::Thread> watch_dog_thread(
        fml::Thread::ThreadConfig("lynx_watch_dog",
                                  fml::Thread::ThreadPriority::NORMAL));
    return watch_dog_thread->GetTaskRunner();
  }

  static void RunOnIdle(TaskConfig&& config,
                        const fml::RefPtr<fml::TaskRunner>& runner) {
    const uint32_t delay = config.delay;
    const uint32_t step = config.step;

    uint32_t counter = 0;
    while (counter < step) {
      lynx_shell_watch_dog_usleep(delay / step);
      counter++;
      uint64_t start_time = base::CurrentTimeMicroseconds();
      runner->PostSyncTask([start_time, &counter, &config]() mutable {
        if (base::CurrentTimeMicroseconds() - start_time > config.allow_delay) {
          counter = 0u;  // target runner is busy, reset counter
        } else if (counter == config.step) {
          config.idle_task();
        }
      });
    }
  }

 public:
  template <typename T>
  static void RunOnActorThreadIdle(TaskConfig&& config,
                                   const std::shared_ptr<LynxActor<T>>& actor) {
    GetWatchDogTaskRunner()->PostTask(
        [config = std::move(config), actor]() mutable {
          // Passing the runner parameter can avoid making the RunOnIdle method
          // also a template function, which can significantly reduce binary
          // size.
          RunOnIdle(std::move(config), actor->GetRunner());
        });
  }
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_
