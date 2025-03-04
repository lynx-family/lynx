// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include <iostream>
#include <thread>

#include "base/include/fml/concurrent_message_loop.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/chrono_timestamp_provider.h"
#include "build/build_config.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define TIMESENSITIVE(x) TimeSensitiveTest_##x
#if OS_WIN
#define PLATFORM_SPECIFIC_CAPTURE(...) [ __VA_ARGS__, count ]
#else
#define PLATFORM_SPECIFIC_CAPTURE(...) [__VA_ARGS__]
#endif

namespace lynx {

TEST(MessageLoop, GetCurrent) {
  std::thread thread([]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    ASSERT_TRUE(fml::MessageLoop::GetCurrent().GetTaskRunner());
  });
  thread.join();
}

TEST(MessageLoop, DifferentThreadsHaveDifferentLoops) {
  fml::MessageLoop* loop1 = nullptr;
  fml::AutoResetWaitableEvent latch1;
  fml::AutoResetWaitableEvent term1;
  std::thread thread1([&loop1, &latch1, &term1]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop1 = &fml::MessageLoop::GetCurrent();
    latch1.Signal();
    term1.Wait();
  });

  fml::MessageLoop* loop2 = nullptr;
  fml::AutoResetWaitableEvent latch2;
  fml::AutoResetWaitableEvent term2;
  std::thread thread2([&loop2, &latch2, &term2]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop2 = &fml::MessageLoop::GetCurrent();
    latch2.Signal();
    term2.Wait();
  });
  latch1.Wait();
  latch2.Wait();
  ASSERT_FALSE(loop1 == loop2);
  term1.Signal();
  term2.Signal();
  thread1.join();
  thread2.join();
}

TEST(MessageLoop, CanRunAndTerminate) {
  bool started = false;
  bool terminated = false;
  std::thread thread([&started, &terminated]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    ASSERT_TRUE(loop.GetTaskRunner());
    loop.GetTaskRunner()->PostTask([&terminated]() {
      fml::MessageLoop::GetCurrent().Terminate();
      terminated = true;
    });
    loop.Run();
    started = true;
  });
  thread.join();
  ASSERT_TRUE(started);
  ASSERT_TRUE(terminated);
}

TEST(MessageLoop, NonDelayedTasksAreRunInOrder) {
  const size_t count = 100;
  bool started = false;
  bool terminated = false;
  std::thread thread([&started, &terminated, count]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    size_t current = 0;
    for (size_t i = 0; i < count; i++) {
      loop.GetTaskRunner()->PostTask(
          PLATFORM_SPECIFIC_CAPTURE(&terminated, i, &current)() {
            ASSERT_EQ(current, i);
            current++;
            if (count == i + 1) {
              fml::MessageLoop::GetCurrent().Terminate();
              terminated = true;
            }
          });
    }
    loop.Run();
    ASSERT_EQ(current, count);
    started = true;
  });
  thread.join();
  ASSERT_TRUE(started);
  ASSERT_TRUE(terminated);
}

TEST(MessageLoop, DelayedTasksAtSameTimeAreRunInOrder) {
  const size_t count = 100;
  bool started = false;
  bool terminated = false;
  std::thread thread([&started, &terminated, count]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    size_t current = 0;
    const auto now_plus_some =
        fml::ChronoTicksSinceEpoch() + fml::TimeDelta::FromMilliseconds(2);
    for (size_t i = 0; i < count; i++) {
      loop.GetTaskRunner()->PostTaskForTime(
          PLATFORM_SPECIFIC_CAPTURE(&terminated, i, &current)() {
            ASSERT_EQ(current, i);
            current++;
            if (count == i + 1) {
              fml::MessageLoop::GetCurrent().Terminate();
              terminated = true;
            }
          },
          now_plus_some);
    }
    loop.Run();
    ASSERT_EQ(current, count);
    started = true;
  });
  thread.join();
  ASSERT_TRUE(started);
  ASSERT_TRUE(terminated);
}

TEST(MessageLoop, CheckRunsTaskOnCurrentThread) {
  fml::RefPtr<fml::TaskRunner> runner;
  fml::AutoResetWaitableEvent latch;
  std::thread thread([&runner, &latch]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    runner = loop.GetTaskRunner();
    latch.Signal();
    ASSERT_TRUE(loop.GetTaskRunner()->RunsTasksOnCurrentThread());
  });
  latch.Wait();
  ASSERT_TRUE(runner);
  ASSERT_FALSE(runner->RunsTasksOnCurrentThread());
  thread.join();
}

TEST(MessageLoop, TIMESENSITIVE(SingleDelayedTaskByDelta)) {
  bool checked = false;
  std::thread thread([&checked]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    auto begin = fml::ChronoTicksSinceEpoch();
    loop.GetTaskRunner()->PostDelayedTask(
        [begin, &checked]() {
          auto delta = fml::ChronoTicksSinceEpoch() - begin;
          auto ms = delta.ToMillisecondsF();
          ASSERT_GE(ms, 3);
          ASSERT_LE(ms, 7);
          checked = true;
          fml::MessageLoop::GetCurrent().Terminate();
        },
        fml::TimeDelta::FromMilliseconds(5));
    loop.Run();
  });
  thread.join();
  ASSERT_TRUE(checked);
}

TEST(MessageLoop, TIMESENSITIVE(SingleDelayedTaskForTime)) {
  bool checked = false;
  std::thread thread([&checked]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    auto begin = fml::ChronoTicksSinceEpoch();
    loop.GetTaskRunner()->PostTaskForTime(
        [begin, &checked]() {
          auto delta = fml::ChronoTicksSinceEpoch() - begin;
          auto ms = delta.ToMillisecondsF();
          ASSERT_GE(ms, 3);
          ASSERT_LE(ms, 7);
          checked = true;
          fml::MessageLoop::GetCurrent().Terminate();
        },
        fml::ChronoTicksSinceEpoch() + fml::TimeDelta::FromMilliseconds(5));
    loop.Run();
  });
  thread.join();
  ASSERT_TRUE(checked);
}

TEST(MessageLoop, TIMESENSITIVE(MultipleDelayedTasksWithIncreasingDeltas)) {
  const auto count = 10;
  int checked = false;
  std::thread thread(PLATFORM_SPECIFIC_CAPTURE(&checked)() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    for (int target_ms = 0 + 2; target_ms < count + 2; target_ms++) {
      auto begin = fml::ChronoTicksSinceEpoch();
      loop.GetTaskRunner()->PostDelayedTask(
          PLATFORM_SPECIFIC_CAPTURE(begin, target_ms, &checked)() {
            auto delta = fml::ChronoTicksSinceEpoch() - begin;
            auto ms = delta.ToMillisecondsF();
            ASSERT_GE(ms, target_ms - 2);
            ASSERT_LE(ms, target_ms + 2);
            checked++;
            if (checked == count) {
              fml::MessageLoop::GetCurrent().Terminate();
            }
          },
          fml::TimeDelta::FromMilliseconds(target_ms));
    }
    loop.Run();
  });
  thread.join();
  ASSERT_EQ(checked, count);
}

TEST(MessageLoop, TIMESENSITIVE(MultipleDelayedTasksWithDecreasingDeltas)) {
  const auto count = 10;
  int checked = false;
  std::thread thread(PLATFORM_SPECIFIC_CAPTURE(&checked)() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    for (int target_ms = count + 2; target_ms > 0 + 2; target_ms--) {
      auto begin = fml::ChronoTicksSinceEpoch();
      loop.GetTaskRunner()->PostDelayedTask(
          PLATFORM_SPECIFIC_CAPTURE(begin, target_ms, &checked)() {
            auto delta = fml::ChronoTicksSinceEpoch() - begin;
            auto ms = delta.ToMillisecondsF();
            ASSERT_GE(ms, target_ms - 2);
            ASSERT_LE(ms, target_ms + 2);
            checked++;
            if (checked == count) {
              fml::MessageLoop::GetCurrent().Terminate();
            }
          },
          fml::TimeDelta::FromMilliseconds(target_ms));
    }
    loop.Run();
  });
  thread.join();
  ASSERT_EQ(checked, count);
}

TEST(MessageLoop, TaskObserverFire) {
  bool started = false;
  bool terminated = false;
  std::thread thread([&started, &terminated]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    const size_t count = 25;
    auto& loop = fml::MessageLoop::GetCurrent();
    size_t task_count = 0;
    size_t obs_count = 0;
    auto obs = PLATFORM_SPECIFIC_CAPTURE(&obs_count)() { obs_count++; };
    for (size_t i = 0; i < count; i++) {
      loop.GetTaskRunner()->PostTask(
          PLATFORM_SPECIFIC_CAPTURE(&terminated, i, &task_count)() {
            ASSERT_EQ(task_count, i);
            task_count++;
            if (count == i + 1) {
              fml::MessageLoop::GetCurrent().Terminate();
              terminated = true;
            }
          });
    }
    loop.GetTaskRunner()->AddTaskObserver(0, std::move(obs));
    loop.Run();
    ASSERT_EQ(task_count, count);
    ASSERT_EQ(obs_count, count);
    started = true;
  });
  thread.join();
  ASSERT_TRUE(started);
  ASSERT_TRUE(terminated);
}

TEST(MessageLoop, ConcurrentMessageLoopHasNonZeroWorkers) {
  auto loop =
      fml::ConcurrentMessageLoop("", fml::Thread::ThreadPriority::NORMAL,
                                 0u /* explicitly specify zero workers */);
  ASSERT_GT(loop.GetWorkerCount(), 0u);
}

TEST(MessageLoop,
     DISABLED_CanCreateAndShutdownConcurrentMessageLoopsOverAndOver) {
  for (size_t i = 0; i < 10; ++i) {
    auto loop = fml::ConcurrentMessageLoop(
        "", fml::Thread::ThreadPriority::NORMAL, i + 1);
    ASSERT_EQ(loop.GetWorkerCount(), i + 1);
  }
}

TEST(MessageLoop, CanCreateConcurrentMessageLoop) {
  auto loop = fml::ConcurrentMessageLoop("");
  const size_t kCount = 10;
  fml::CountDownLatch latch(kCount);
  std::mutex thread_ids_mutex;
  std::set<std::thread::id> thread_ids;
  for (size_t i = 0; i < kCount; ++i) {
    loop.PostTask([&]() {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      std::cout << "Ran on thread: " << std::this_thread::get_id() << std::endl;
      std::scoped_lock lock(thread_ids_mutex);
      thread_ids.insert(std::this_thread::get_id());
      latch.CountDown();
    });
  }
  latch.Wait();
  ASSERT_GE(thread_ids.size(), 1u);
}

#if !defined(OS_WIN)
static void MockThreadConfigSetter(const fml::Thread::ThreadConfig& config) {
  // set thread name
  fml::Thread::SetCurrentThreadName(config);

  pthread_t tid = pthread_self();
  struct sched_param param;
  int policy = SCHED_OTHER;
  switch (config.priority) {
    case fml::Thread::ThreadPriority::HIGH:
      param.sched_priority = 10;
      break;
    default:
      param.sched_priority = 1;
  }
    // In linx system, sched_priority can only be set to 0 with SCHED_OTHER
    // policy.
#if defined(OS_LINUX)
  param.sched_priority = 0;
#endif
  pthread_setschedparam(tid, policy, &param);
}

TEST(MessageLoop, CreateConcurrentMessageLoopWithThreadConfigSetter) {
  auto loop = fml::ConcurrentMessageLoop("test", MockThreadConfigSetter,
                                         fml::Thread::ThreadPriority::HIGH, 1);
  const size_t kCount = 1;
  fml::CountDownLatch latch(kCount);
  for (size_t i = 0; i < kCount; ++i) {
    loop.PostTask([&]() {
      struct sched_param param;
      char thread_name[16];
      int policy = SCHED_OTHER;
      pthread_t current_thread = pthread_self();
      pthread_getname_np(current_thread, thread_name, sizeof(thread_name));
      pthread_getschedparam(current_thread, &policy, &param);
      const std::string name = "test1";
      ASSERT_EQ(thread_name, name);
      ASSERT_EQ(policy, SCHED_OTHER);
      // In linx system, sched_priority can only be set to 0 with SCHED_OTHER
      // policy.
#if !defined(OS_LINUX)
      ASSERT_EQ(param.sched_priority, 10);
#else
      ASSERT_EQ(param.sched_priority, 0);
#endif
      latch.CountDown();
    });
  }
  latch.Wait();
}
#endif

TEST(MessageLoop, PostEmergencyTask) {
  constexpr uint32_t count = 10;
  std::string str;
  std::thread thread([&str]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    for (uint32_t i = 0; i < count; ++i) {
      loop.GetTaskRunner()->PostTask([&str, i]() {
        str.push_back('0' + i);
        if (i + 1 == count) {
          fml::MessageLoop::GetCurrent().Terminate();
        }
      });
    }
    loop.GetTaskRunner()->PostEmergencyTask([&str]() { str.push_back('a'); });
    loop.Run();
  });
  thread.join();
  ASSERT_EQ(str, "a0123456789");
}

TEST(MessageLoop, PostIdleTaskNotInIdlePeriod) {
  constexpr uint32_t count = 10;
  std::string str;
  std::thread thread([&str]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    loop.GetTaskRunner()->PostIdleTask([&str]() { str.push_back('a'); });
    for (uint32_t i = 0; i < count; ++i) {
      loop.GetTaskRunner()->PostTask([&str, i]() {
        str.push_back('0' + i);
        if (i + 1 == count) {
          fml::MessageLoop::GetCurrent().Terminate();
        }
      });
    }
    loop.GetTaskRunner()->PostIdleTask([&str]() { str.push_back('b'); });
    loop.Run();
  });
  thread.join();
  ASSERT_EQ(str, "0123456789ab");
}

TEST(MessageLoop, TIMESENSITIVE(PostIdleTaskInIdlePeriod)) {
  constexpr uint32_t count = 10;
  std::string str;
  std::thread thread([&str]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    for (uint32_t i = 0; i < count; ++i) {
      loop.GetTaskRunner()->PostDelayedTask(
          [&str, i]() {
            str.push_back('0' + i);
            if (i + 1 == count) {
              fml::MessageLoop::GetCurrent().Terminate();
            }
          },
          fml::TimeDelta::FromMilliseconds(100));
    }
    loop.GetTaskRunner()->PostIdleTask([&str]() { str.push_back('a'); });
    loop.Run();
  });
  thread.join();
  ASSERT_EQ(str, "a0123456789");
}

TEST(MessageLoop, PostSyncTask) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  fml::MessageLoop* async = nullptr;
  fml::AutoResetWaitableEvent latch;

  std::thread thread([&async, &latch]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    async = &loop;
    latch.Signal();

    // on current loop, need run immediately
    bool is_run_immediately = false;
    loop.GetTaskRunner()->PostSyncTask(
        [&is_run_immediately]() { is_run_immediately = true; });
    ASSERT_TRUE(is_run_immediately);

    loop.Run();
  });

  latch.Wait();
  bool ret = false;
  async->GetTaskRunner()->PostSyncTask([&ret, async]() {
    ret = async == &(fml::MessageLoop::GetCurrent());
    fml::MessageLoop::GetCurrent().Terminate();
  });
  ASSERT_TRUE(ret);
  thread.join();
}

TEST(MessageLoop, PostMicroTask) {
  constexpr uint32_t count = 10;
  std::string str;
  std::thread thread([&str]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    auto& loop = fml::MessageLoop::GetCurrent();
    for (uint32_t i = 0; i < count; ++i) {
      loop.GetTaskRunner()->PostTask([&str, i]() {
        str.push_back('0' + i);
        if (i + 1 == count) {
          fml::MessageLoop::GetCurrent().Terminate();
        }
      });
    }
    loop.GetTaskRunner()->PostMicroTask([&str]() { str.push_back('a'); });
    loop.GetTaskRunner()->PostEmergencyTask([&str]() { str.push_back('b'); });
    loop.GetTaskRunner()->PostMicroTask([&str]() { str.push_back('c'); });
    loop.Run();
  });
  thread.join();
  ASSERT_EQ(str, "acb0123456789");
}

}  // namespace lynx
