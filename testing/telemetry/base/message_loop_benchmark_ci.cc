// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
#include "base/include/lynx_actor.h"
#include "base/include/no_destructor.h"
#include "third_party/benchmark/include/benchmark/benchmark.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"

namespace lynx {
namespace base {

class MockActorObject {
 public:
  int x;
};

static fml::RefPtr<fml::TaskRunner> GetTaskRunner() {
  static base::NoDestructor<fml::Thread> thread;
  return thread->GetTaskRunner();
}

static void heavyWork(int nanoseconds) {
  auto start = std::chrono::high_resolution_clock::now();
  while (std::chrono::high_resolution_clock::now() - start <
         std::chrono::nanoseconds(nanoseconds)) {
    // spin wait
  }
}

static void BM_TaskScheduler_Throughput(benchmark::State& state) {
  auto actor_object = std::make_unique<MockActorObject>();
  auto actor = std::make_shared<shell::LynxActor<MockActorObject>>(
      std::move(actor_object), GetTaskRunner(), 0, true);

  const auto total_tasks = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    std::atomic<int> completed_count{0};
    std::promise<void> all_done_promise;
    auto all_done_future = all_done_promise.get_future();

    auto task_wrapper = [&](std::function<void()> real_task) {
      real_task();
      if (completed_count.fetch_add(1, std::memory_order_release) ==
          total_tasks - 1) {
        all_done_promise.set_value();
      }
    };

    auto tasks_per_thread = total_tasks / 2;
    auto producer_logic = [&](int start_index) {
      std::string heavy_payload(128, 'X');
      int simple_payload = 42;
      for (auto i = 0; i < tasks_per_thread; ++i) {
        auto task_type = (start_index + i) % 3;
        if (task_type == 0) {
          actor->ActAsync([wrapper = task_wrapper, simple_payload](auto&) {
            wrapper([simple_payload]() {
              benchmark::DoNotOptimize(simple_payload + 1);
            });
          });
        } else if (task_type == 1) {
          actor->ActAsync([wrapper = task_wrapper, heavy_payload](auto&) {
            wrapper([heavy_payload]() {
              benchmark::DoNotOptimize(heavy_payload.size());
              heavyWork(50);  // 50ns
            });
          });
        } else {
          actor->ActAsync([wrapper = task_wrapper](auto&) {
            wrapper([]() {
              heavyWork(200);  // 200ns
            });
          });
        }
      }
    };
    state.ResumeTiming();
    // start producer threads
    std::thread threadB(producer_logic, 0);
    std::thread threadC(producer_logic, tasks_per_thread);
    all_done_future.wait();
    threadB.join();
    threadC.join();
  }
  // state.SetItemsProcessed(state.iterations() * total_tasks);
}

BENCHMARK(BM_TaskScheduler_Throughput)->Arg(20000)->UseRealTime();

}  // namespace base
}  // namespace lynx

#pragma clang diagnostic pop
