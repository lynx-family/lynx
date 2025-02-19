// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include <thread>

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define TIMESENSITIVE(x) TimeSensitiveTest_##x

namespace lynx {
class MockVSyncMonitor {
 public:
  MockVSyncMonitor() = default;
  ~MockVSyncMonitor() = default;

  void RequestVSync(fml::VSyncCallback callback) {
    callback_ = std::move(callback);
  }

  void TriggerVSync() {
    if (callback_) {
      callback_(current_, current_ + kFrameDuration);
    }
  }

 private:
  int64_t current_ = kFrameDuration;
  constexpr static int64_t kFrameDuration = 16;  // ms
  fml::VSyncCallback callback_;
};

TEST(MessageLoopImpl, TIMESENSITIVE(WakeUpTimersAreSingletons)) {
  auto loop_impl = fml::MessageLoopImpl::Create();

  const auto t1 = fml::TimeDelta::FromMilliseconds(10);
  const auto t2 = fml::TimeDelta::FromMilliseconds(30);

  const auto begin = fml::TimePoint::Now();

  // Register a task scheduled in the future. This schedules a WakeUp call on
  // the MessageLoopImpl with that fml::TimePoint.
  loop_impl->PostTask(
      [&]() {
        auto delta = fml::TimePoint::Now() - begin;
        auto ms = delta.ToMillisecondsF();
        ASSERT_GE(ms, 20);
        ASSERT_LE(ms, 40);

        loop_impl->Terminate();
      },
      begin + t1);

  // Call WakeUp manually to change the WakeUp time further in the future. If
  // the timer is correctly set up to be rearmed instead of a task being
  // scheduled for each WakeUp, the above task will be executed at t2 instead
  // of t1 now.
  loop_impl->WakeUp(begin + t2);

  loop_impl->Run();
}

TEST(MessageLoopImpl, WakeUpByVSync) {
  fml::AutoResetWaitableEvent latch;
  fml::RefPtr<fml::MessageLoopImpl> loop_impl;
  base::closure setup_thread = [&]() {
    auto& loop = fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop_impl = loop.GetLoopImpl();
    latch.Signal();
    loop.Run();
  };
  auto thread = std::make_unique<std::thread>(std::move(setup_thread));
  latch.Wait();
  latch.Reset();

  auto task_queues = fml::MessageLoopTaskQueues::GetInstance();
  auto vsync_queue_id = task_queues->CreateTaskQueue(true);
  auto vsync_monitor = MockVSyncMonitor();
  loop_impl->SetVSyncRequest([&](fml::VSyncCallback vsync_callback) {
    vsync_monitor.RequestVSync(
        [vsync_callback = std::move(vsync_callback)](
            int64_t frame_start_time_ns, int64_t frame_target_time_ns) {
          vsync_callback(frame_start_time_ns, frame_target_time_ns);
        });
  });

  // Bind vsync queue to the loop.
  loop_impl->PostTask(
      [&]() {
        loop_impl->Bind(vsync_queue_id);
        latch.Signal();
      },
      fml::TimePoint::Now());
  latch.Wait();
  latch.Reset();

  // Register vsync task
  bool vsync_task_executed = false;
  auto vsync_aligned_task = [&]() { vsync_task_executed = true; };
  task_queues->RegisterTask(vsync_queue_id, std::move(vsync_aligned_task),
                            fml::TimePoint::Now());

  loop_impl->PostTask(
      [&]() {
        vsync_monitor.TriggerVSync();
        latch.Signal();
      },
      fml::TimePoint::Now());
  latch.Wait();
  ASSERT_TRUE(vsync_task_executed);

  // Terminate thread safely
  loop_impl->PostTask([&]() { loop_impl->Terminate(); }, fml::TimePoint::Now());
  thread->join();
}

}  // namespace lynx
