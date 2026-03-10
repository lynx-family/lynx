// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_
#define CORE_SERVICES_WATCH_DOG_WATCH_DOG_H_

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/closure.h"
#include "base/include/fml/thread.h"
#include "base/include/log/logging.h"
#include "base/include/lynx_actor.h"
#include "base/include/timer/time_utils.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace shell {

class WatchDog final {
 public:
  using TimeoutErrorHandler =
      base::MoveOnlyClosure<void, std::string,
                            std::unordered_map<std::string, std::string>>;
  struct TaskConfig {
    uint32_t delay = 50 * 1000;
    uint32_t allow_delay = 1.5 * 1000;
    uint32_t step = 2;
    base::MoveOnlyClosure<void> idle_task;
  };

  class JSCallTimeoutGuard {
   public:
    JSCallTimeoutGuard(TimeoutErrorHandler error_dispatcher,
                       uint32_t timeout_ms, std::string page_url)
        : error_dispatcher_(std::move(error_dispatcher)),
          timeout_ms_(timeout_ms),
          page_url_(std::move(page_url)),
          done_flag_(std::make_shared<std::atomic<bool>>(false)) {
      auto done = done_flag_;
      GetWatchDogTaskRunner()->PostDelayedTask(
          [done, dispatcher = std::move(error_dispatcher_),
           timeout_ms = timeout_ms_, page_url = page_url_]() mutable {
            if (!done->load()) {
              LOGE("js call exceeded " << timeout_ms << "ms"
                                       << ", page_url: " << page_url);
              std::string message = std::string("JS call exceeded ") +
                                    std::to_string(timeout_ms) + "ms";
              std::unordered_map<std::string, std::string> info;
              info.emplace("timeout_ms", std::to_string(timeout_ms));
              if (!page_url.empty()) {
                info.emplace("page_url", std::move(page_url));
              }
              if (dispatcher) {
                dispatcher(std::move(message), std::move(info));
              }
            }
          },
          fml::TimeDelta::FromMilliseconds(timeout_ms_));
    }

    ~JSCallTimeoutGuard() { done_flag_->store(true); }

    JSCallTimeoutGuard(const JSCallTimeoutGuard&) = delete;
    JSCallTimeoutGuard& operator=(const JSCallTimeoutGuard&) = delete;

   private:
    TimeoutErrorHandler error_dispatcher_;
    uint32_t timeout_ms_;
    std::string page_url_;
    std::shared_ptr<std::atomic<bool>> done_flag_;
  };

 private:
  static const fml::RefPtr<fml::TaskRunner>& GetWatchDogTaskRunner() {
    static base::NoDestructor<fml::Thread> watch_dog_thread(
        fml::Thread::ThreadConfig("lynx_watch_dog",
                                  fml::Thread::ThreadPriority::NORMAL));
    return watch_dog_thread->GetTaskRunner();
  }

  struct IdleCheckState {
    explicit IdleCheckState(TaskConfig&& config,
                            fml::RefPtr<fml::TaskRunner> runner)
        : delay_us(config.delay),
          allow_delay_us(config.allow_delay),
          step(config.step == 0 ? 1u : config.step),
          idle_task(std::move(config.idle_task)),
          runner(std::move(runner)) {}

    uint32_t delay_us;
    uint32_t allow_delay_us;
    uint32_t step;
    uint32_t counter = 0;
    base::MoveOnlyClosure<void> idle_task;
    fml::RefPtr<fml::TaskRunner> runner;
  };

  static void RunOnIdle(TaskConfig&& config,
                        const fml::RefPtr<fml::TaskRunner>& runner) {
    auto state = std::make_shared<IdleCheckState>(std::move(config), runner);

    auto schedule_next = std::make_shared<base::closure>();
    *schedule_next = base::closure([state, schedule_next]() mutable {
      GetWatchDogTaskRunner()->PostDelayedTask(
          [state, schedule_next]() mutable {
            state->counter++;
            uint64_t start_time = base::CurrentTimeMicroseconds();
            state->runner->PostSyncTask(
                [state, start_time, schedule_next]() mutable {
                  if (base::CurrentTimeMicroseconds() - start_time >
                      state->allow_delay_us) {
                    state->counter = 0u;
                  } else if (state->counter >= state->step) {
                    if (state->idle_task) {
                      state->idle_task();
                    }
                    return;
                  }
                  (*schedule_next)();
                });
          },
          fml::TimeDelta::FromMicroseconds(state->delay_us / state->step));
    });

    (*schedule_next)();
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
