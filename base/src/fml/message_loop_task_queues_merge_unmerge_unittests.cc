// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include <thread>

#include "base/include/fml/message_loop_task_queues.h"
#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/time/chrono_timestamp_provider.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {
namespace testing {

class TestWakeable : public fml::Wakeable {
 public:
  using WakeUpCall = std::function<void(const fml::TimePoint)>;

  explicit TestWakeable(WakeUpCall call) : wake_up_call_(call) {}

  void WakeUp(fml::TimePoint time_point) override { wake_up_call_(time_point); }

 private:
  WakeUpCall wake_up_call_;
};

static int CountRemainingTasks(MessageLoopTaskQueues* task_queue,
                               const TaskQueueId& queue_id,
                               bool run_invocation = false) {
  const auto now = ChronoTicksSinceEpoch();
  int count = 0;
  std::vector<TaskQueueId> queue_ids = {queue_id};
  base::closure invocation;
  do {
    auto task = task_queue->GetNextTaskToRun(queue_ids, now);
    if (!task || !task.has_value()) {
      break;
    }
    invocation = std::move((*task).task);
    if (!invocation) {
      break;
    }
    count++;
    if (run_invocation) {
      invocation();
    }
  } while (invocation);
  return count;
}

TEST(MessageLoopTaskQueueMergeUnmerge,
     AfterMergePrimaryTasksServicedOnPrimary) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  task_queue->RegisterTask(
      queue_id_1, []() {}, ChronoTicksSinceEpoch());
  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_1));

  task_queue->Merge(queue_id_1, queue_id_2);
  task_queue->RegisterTask(
      queue_id_1, []() {}, ChronoTicksSinceEpoch());

  ASSERT_EQ(2u, task_queue->GetNumPendingTasks(queue_id_1));
  ASSERT_EQ(0u, task_queue->GetNumPendingTasks(queue_id_2));
}

TEST(MessageLoopTaskQueueMergeUnmerge,
     AfterMergeSecondaryTasksAlsoServicedOnPrimary) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  task_queue->RegisterTask(
      queue_id_2, []() {}, ChronoTicksSinceEpoch());
  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_2));

  task_queue->Merge(queue_id_1, queue_id_2);
  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_1));
  ASSERT_EQ(0u, task_queue->GetNumPendingTasks(queue_id_2));
}

TEST(MessageLoopTaskQueueMergeUnmerge, MergeUnmergeTasksPreserved) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  task_queue->RegisterTask(
      queue_id_1, []() {}, ChronoTicksSinceEpoch());
  task_queue->RegisterTask(
      queue_id_2, []() {}, ChronoTicksSinceEpoch());

  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_1));
  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_2));

  task_queue->Merge(queue_id_1, queue_id_2);

  ASSERT_EQ(2u, task_queue->GetNumPendingTasks(queue_id_1));
  ASSERT_EQ(0u, task_queue->GetNumPendingTasks(queue_id_2));

  task_queue->Unmerge(queue_id_1, queue_id_2);

  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_1));
  ASSERT_EQ(1u, task_queue->GetNumPendingTasks(queue_id_2));
}

/// Multiple standalone engines scene
TEST(MessageLoopTaskQueueMergeUnmerge,
     OneCanOwnMultipleQueuesAndUnmergeIndependently) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();
  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();
  auto queue_id_3 = task_queue->CreateTaskQueue();

  // merge
  ASSERT_TRUE(task_queue->Merge(queue_id_1, queue_id_2));
  ASSERT_TRUE(task_queue->Owns(queue_id_1, queue_id_2));
  ASSERT_FALSE(task_queue->Owns(queue_id_1, queue_id_3));

  ASSERT_TRUE(task_queue->Merge(queue_id_1, queue_id_3));
  ASSERT_TRUE(task_queue->Owns(queue_id_1, queue_id_2));
  ASSERT_TRUE(task_queue->Owns(queue_id_1, queue_id_3));

  // unmerge
  ASSERT_TRUE(task_queue->Unmerge(queue_id_1, queue_id_2));
  ASSERT_FALSE(task_queue->Owns(queue_id_1, queue_id_2));
  ASSERT_TRUE(task_queue->Owns(queue_id_1, queue_id_3));

  ASSERT_TRUE(task_queue->Unmerge(queue_id_1, queue_id_3));
  ASSERT_FALSE(task_queue->Owns(queue_id_1, queue_id_2));
  ASSERT_FALSE(task_queue->Owns(queue_id_1, queue_id_3));
}

TEST(MessageLoopTaskQueueMergeUnmerge,
     CannotMergeSameQueueToTwoDifferentOwners) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();
  auto queue = task_queue->CreateTaskQueue();
  auto owner_1 = task_queue->CreateTaskQueue();
  auto owner_2 = task_queue->CreateTaskQueue();

  ASSERT_TRUE(task_queue->Merge(owner_1, queue));
  ASSERT_FALSE(task_queue->Merge(owner_2, queue));
}

TEST(MessageLoopTaskQueueMergeUnmerge, MergeFailIfAlreadySubsumed) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();
  auto queue_id_3 = task_queue->CreateTaskQueue();

  ASSERT_TRUE(task_queue->Merge(queue_id_1, queue_id_2));
  ASSERT_FALSE(task_queue->Merge(queue_id_2, queue_id_3));
  ASSERT_FALSE(task_queue->Merge(queue_id_2, queue_id_1));
}

TEST(MessageLoopTaskQueueMergeUnmerge,
     MergeFailIfAlreadyOwnsButTryToBeSubsumed) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();
  auto queue_id_3 = task_queue->CreateTaskQueue();

  task_queue->Merge(queue_id_1, queue_id_2);
  // A recursively linked merging will fail
  ASSERT_FALSE(task_queue->Merge(queue_id_3, queue_id_1));
}

TEST(MessageLoopTaskQueueMergeUnmerge, UnmergeFailsOnSubsumedOrNeverMerged) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();
  auto queue_id_3 = task_queue->CreateTaskQueue();

  task_queue->Merge(queue_id_1, queue_id_2);
  ASSERT_FALSE(task_queue->Unmerge(queue_id_2, queue_id_3));
  ASSERT_FALSE(task_queue->Unmerge(queue_id_1, queue_id_3));
  ASSERT_FALSE(task_queue->Unmerge(queue_id_3, queue_id_1));
  ASSERT_FALSE(task_queue->Unmerge(queue_id_2, queue_id_1));
}

TEST(MessageLoopTaskQueueMergeUnmerge, MergeInvokesBothWakeables) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  fml::CountDownLatch latch(2);

  task_queue->SetWakeable(
      queue_id_1,
      new TestWakeable([&](fml::TimePoint wake_time) { latch.CountDown(); }));
  task_queue->SetWakeable(
      queue_id_2,
      new TestWakeable([&](fml::TimePoint wake_time) { latch.CountDown(); }));

  task_queue->RegisterTask(
      queue_id_1, []() {}, ChronoTicksSinceEpoch());

  task_queue->Merge(queue_id_1, queue_id_2);

  CountRemainingTasks(task_queue, queue_id_1);

  latch.Wait();
}

TEST(MessageLoopTaskQueueMergeUnmerge,
     MergeUnmergeInvokesBothWakeablesSeparately) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  fml::AutoResetWaitableEvent latch_1, latch_2;

  task_queue->SetWakeable(
      queue_id_1,
      new TestWakeable([&](fml::TimePoint wake_time) { latch_1.Signal(); }));
  task_queue->SetWakeable(
      queue_id_2,
      new TestWakeable([&](fml::TimePoint wake_time) { latch_2.Signal(); }));

  task_queue->RegisterTask(
      queue_id_1, []() {}, ChronoTicksSinceEpoch());
  task_queue->RegisterTask(
      queue_id_2, []() {}, ChronoTicksSinceEpoch());

  task_queue->Merge(queue_id_1, queue_id_2);
  task_queue->Unmerge(queue_id_1, queue_id_2);

  CountRemainingTasks(task_queue, queue_id_1);

  latch_1.Wait();

  CountRemainingTasks(task_queue, queue_id_2);

  latch_2.Wait();
}

TEST(MessageLoopTaskQueueMergeUnmerge, GetTasksToRunNowBlocksMerge) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  fml::AutoResetWaitableEvent wake_up_start, wake_up_end, merge_start,
      merge_end;

  task_queue->RegisterTask(
      queue_id_1, []() {}, ChronoTicksSinceEpoch());
  task_queue->SetWakeable(queue_id_1,
                          new TestWakeable([&](fml::TimePoint wake_time) {
                            wake_up_start.Signal();
                            wake_up_end.Wait();
                          }));

  std::thread tasks_to_run_now_thread([&]() {
    // Fetch task previously added with task_queue's queue_mutex_.
    CountRemainingTasks(task_queue, queue_id_1);
  });

  wake_up_start.Wait();
  bool merge_done = false;

  std::thread merge_thread([&]() {
    merge_start.Signal();
    /*
     * Merge requires task_queue's queue_mutex_.
     * Before wake_up_end.Signal() called, queue_mutex_
     * is held by task_queue_->WakeUp.
     */
    task_queue->Merge(queue_id_1, queue_id_2);
    merge_done = true;
    merge_end.Signal();
  });

  merge_start.Wait();
  ASSERT_FALSE(merge_done);
  wake_up_end.Signal();

  merge_end.Wait();
  ASSERT_TRUE(merge_done);

  tasks_to_run_now_thread.join();
  merge_thread.join();
}

TEST(MessageLoopTaskQueueMergeUnmerge,
     FollowingTasksSwitchQueueIfFirstTaskMergesThreads) {
  auto task_queue = fml::MessageLoopTaskQueues::GetInstance();

  auto queue_id_1 = task_queue->CreateTaskQueue();
  auto queue_id_2 = task_queue->CreateTaskQueue();

  fml::CountDownLatch latch(2);

  task_queue->SetWakeable(
      queue_id_1,
      new TestWakeable([&](fml::TimePoint wake_time) { latch.CountDown(); }));
  task_queue->SetWakeable(
      queue_id_2,
      new TestWakeable([&](fml::TimePoint wake_time) { latch.CountDown(); }));

  task_queue->RegisterTask(
      queue_id_2, [&]() { task_queue->Merge(queue_id_1, queue_id_2); },
      ChronoTicksSinceEpoch());

  task_queue->RegisterTask(
      queue_id_2, []() {}, ChronoTicksSinceEpoch());

  ASSERT_EQ(CountRemainingTasks(task_queue, queue_id_2, true), 1);
  ASSERT_EQ(CountRemainingTasks(task_queue, queue_id_1, true), 1);

  latch.Wait();
}

}  // namespace testing
}  // namespace fml
}  // namespace lynx
