// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include <chrono>
#include <thread>
#include <vector>

#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#define TIMESENSITIVE(x) TimeSensitiveTest_##x

namespace lynx {
class MockVSyncMonitor {
 public:
  explicit MockVSyncMonitor(int64_t frame_duration_ns = 16000000)
      : frame_duration_ns_(frame_duration_ns) {}
  ~MockVSyncMonitor() = default;

  void RequestVSync(fml::VSyncCallback callback) {
    request_count_++;
    callback_ = std::move(callback);
  }

  void TriggerVSync() {
    if (callback_) {
      auto callback = std::move(callback_);
      const auto frame_start = current_;
      current_ += frame_duration_ns_;
      callback(frame_start, current_);
    }
  }

  void SetFrameDuration(int64_t frame_duration_ns) {
    frame_duration_ns_ = frame_duration_ns;
  }

  bool HasPendingRequest() const { return callback_ != nullptr; }

  int request_count() const { return request_count_; }

 private:
  int64_t current_ = 0;
  int64_t frame_duration_ns_;
  fml::VSyncCallback callback_;
  int request_count_ = 0;
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

class RecordingMessageLoop : public fml::MessageLoopImpl {
 public:
  RecordingMessageLoop() = default;
  ~RecordingMessageLoop() override = default;

  void Run() override {}
  void Terminate() override {}
  void WakeUp(fml::TimePoint time_point) override {
    wake_times.push_back(time_point);
  }

  std::vector<fml::TimePoint> wake_times;
};

class MessageLoopImplVSyncTest : public ::testing::Test {
 protected:
  void SetUp() override {
    loop_impl_ = fml::MakeRefCounted<RecordingMessageLoop>();
    task_queues_ = fml::MessageLoopTaskQueues::GetInstance();
    loop_impl_->SetVSyncRequest([this](fml::VSyncCallback callback) {
      vsync_monitor_.RequestVSync(std::move(callback));
    });
    vsync_queue_id_ = CreateAlignedQueue();
  }

  void TearDown() override {
    for (auto queue_id : aligned_queues_) {
      loop_impl_->UnBind(queue_id);
      task_queues_->DisposeTasks(queue_id);
      task_queues_->Dispose(queue_id);
    }
    loop_impl_ = nullptr;
  }

  fml::TaskQueueId CreateAlignedQueue() {
    auto queue_id = task_queues_->CreateTaskQueue(true);
    aligned_queues_.push_back(queue_id);
    loop_impl_->Bind(queue_id);
    return queue_id;
  }

  fml::RefPtr<RecordingMessageLoop> loop_impl_;
  fml::MessageLoopTaskQueues* task_queues_ = nullptr;
  fml::TaskQueueId vsync_queue_id_{0};
  std::vector<fml::TaskQueueId> aligned_queues_;
  // A sub-millisecond interval gives a zero budget, deterministically yielding
  // after one task. NonzeroFrameBudgetContinuesOnNextVSync uses a real
  // interval.
  MockVSyncMonitor vsync_monitor_{1};
};

TEST_F(MessageLoopImplVSyncTest, ReadyTasksContinueOnNextVSync) {
  int executed_task_count = 0;
  const auto now = fml::TimePoint::Now();
  task_queues_->RegisterTask(
      vsync_queue_id_, [&]() { executed_task_count++; }, now);
  task_queues_->RegisterTask(
      vsync_queue_id_, [&]() { executed_task_count++; }, now);

  EXPECT_EQ(vsync_monitor_.request_count(), 1);
  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 1);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  EXPECT_EQ(task_queues_->GetNumPendingTasks(vsync_queue_id_), 1u);
  vsync_monitor_.TriggerVSync();
  ASSERT_EQ(executed_task_count, 2);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
  EXPECT_TRUE(loop_impl_->wake_times.empty());
}

TEST_F(MessageLoopImplVSyncTest, EmptyQueuePreservesOrdinaryWakeUp) {
  const auto target = fml::TimePoint::Now() + fml::TimeDelta::FromSeconds(10);
  loop_impl_->PostTask([]() {}, target);
  task_queues_->RegisterTask(
      vsync_queue_id_, []() {}, fml::TimePoint::Now());

  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(task_queues_->GetNumPendingTasks(vsync_queue_id_), 0u);
  EXPECT_EQ(vsync_monitor_.request_count(), 1);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
  ASSERT_EQ(loop_impl_->wake_times.size(), 1u);
  EXPECT_EQ(loop_impl_->wake_times.back(), target);
}

TEST_F(MessageLoopImplVSyncTest, FutureTasksPreserveOrdinaryWakeUp) {
  const auto now = fml::TimePoint::Now();
  const auto ordinary_target = now + fml::TimeDelta::FromSeconds(10);
  loop_impl_->PostTask([]() {}, ordinary_target);
  task_queues_->RegisterTask(
      vsync_queue_id_, []() {}, now);
  task_queues_->RegisterTask(
      vsync_queue_id_, []() {}, now + fml::TimeDelta::FromSeconds(20));

  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(task_queues_->GetNumPendingTasks(vsync_queue_id_), 1u);
  EXPECT_EQ(vsync_monitor_.request_count(), 1);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
  ASSERT_EQ(loop_impl_->wake_times.size(), 1u);
  EXPECT_EQ(loop_impl_->wake_times.back(), ordinary_target);
}

TEST_F(MessageLoopImplVSyncTest, PostingDuringFlushDoesNotDuplicateVSync) {
  int executed_task_count = 0;
  task_queues_->RegisterTask(
      vsync_queue_id_,
      [&]() {
        executed_task_count++;
        task_queues_->RegisterTask(
            vsync_queue_id_, [&]() { executed_task_count++; },
            fml::TimePoint::Now());
      },
      fml::TimePoint::Now());

  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 1);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 2);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
}

TEST_F(MessageLoopImplVSyncTest, MultipleQueuesContinueOnNextVSync) {
  const auto second_queue_id = CreateAlignedQueue();
  const auto now = fml::TimePoint::Now();
  int executed_task_count = 0;
  task_queues_->RegisterTask(
      vsync_queue_id_, [&]() { executed_task_count++; }, now);
  task_queues_->RegisterTask(
      second_queue_id, [&]() { executed_task_count++; }, now);

  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 1);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 2);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
}

TEST_F(MessageLoopImplVSyncTest, MergedQueueContinuesOnNextVSync) {
  const auto subsumed_queue_id = task_queues_->CreateTaskQueue();
  const auto now = fml::TimePoint::Now();
  int executed_task_count = 0;
  task_queues_->RegisterTask(
      vsync_queue_id_, [&]() { executed_task_count++; }, now);
  task_queues_->RegisterTask(
      subsumed_queue_id, [&]() { executed_task_count++; }, now);
  ASSERT_TRUE(task_queues_->Merge(vsync_queue_id_, subsumed_queue_id));

  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 1);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 2);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
}

TEST_F(MessageLoopImplVSyncTest, NonzeroFrameBudgetContinuesOnNextVSync) {
  vsync_monitor_.SetFrameDuration(16000000);
  const auto now = fml::TimePoint::Now();
  int executed_task_count = 0;
  task_queues_->RegisterTask(
      vsync_queue_id_,
      [&]() {
        executed_task_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(13));
      },
      now);
  task_queues_->RegisterTask(
      vsync_queue_id_, [&]() { executed_task_count++; }, now);

  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 1);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(executed_task_count, 2);
  EXPECT_EQ(vsync_monitor_.request_count(), 2);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
}

TEST_F(MessageLoopImplVSyncTest, DrainedFrameDoesNotRequestAnotherVSync) {
  vsync_monitor_.SetFrameDuration(16000000);
  task_queues_->RegisterTask(
      vsync_queue_id_, []() {}, fml::TimePoint::Now());
  vsync_monitor_.TriggerVSync();
  EXPECT_EQ(vsync_monitor_.request_count(), 1);
  EXPECT_FALSE(vsync_monitor_.HasPendingRequest());
  EXPECT_TRUE(loop_impl_->wake_times.empty());
}

TEST_F(MessageLoopImplVSyncTest, ReadinessCheckDoesNotConsumeOrWakeTasks) {
  EXPECT_FALSE(task_queues_->HasTaskToRun({}, fml::TimePoint::Now()));
  EXPECT_FALSE(
      task_queues_->HasTaskToRun({vsync_queue_id_}, fml::TimePoint::Now()));
  const auto now = fml::TimePoint::Now();
  const auto target = now + fml::TimeDelta::FromSeconds(10);
  task_queues_->RegisterTask(
      vsync_queue_id_, []() {}, target);
  EXPECT_FALSE(task_queues_->HasTaskToRun({vsync_queue_id_}, now));
  EXPECT_TRUE(task_queues_->HasTaskToRun({vsync_queue_id_}, target));
  EXPECT_EQ(task_queues_->GetNumPendingTasks(vsync_queue_id_), 1u);
  EXPECT_EQ(vsync_monitor_.request_count(), 0);
  ASSERT_EQ(loop_impl_->wake_times.size(), 1u);
  EXPECT_EQ(loop_impl_->wake_times.back(), target);
}

}  // namespace lynx
