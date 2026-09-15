// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/create_view_scheduler.h"

#include <future>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace {

using Scheduler = CreateViewScheduler<std::unique_ptr<int>>;

class CreateViewSchedulerTest : public ::testing::Test {
 protected:
  Scheduler scheduler_{
      [this](base::closure task) { workers_.push_back(std::move(task)); }};
  std::vector<base::closure> workers_;
};

TEST_F(CreateViewSchedulerTest, MainClaimsAndConsumesOnlyOnce) {
  int preparations = 0;
  auto task = scheduler_.Schedule([&] {
    ++preparations;
    return std::make_unique<int>(7);
  });
  auto result = scheduler_.Consume(task);
  ASSERT_TRUE(result);
  EXPECT_EQ(**result, 7);
  workers_[0]();
  EXPECT_EQ(preparations, 1);
  EXPECT_FALSE(scheduler_.Consume(task));
}

TEST_F(CreateViewSchedulerTest, WorkerClaimsAndNullResultStillCompletes) {
  int preparations = 0;
  auto task = scheduler_.Schedule([&]() -> std::unique_ptr<int> {
    ++preparations;
    return nullptr;
  });
  std::thread worker([&] { workers_[0](); });
  worker.join();
  auto result = scheduler_.Consume(task);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->get(), nullptr);
  EXPECT_FALSE(scheduler_.Consume(task));
  EXPECT_EQ(preparations, 1);
}

TEST_F(CreateViewSchedulerTest,
       WaitAssistsSnapshotInLifoOrderWithoutConsuming) {
  std::promise<void> started;
  std::promise<void> release;
  auto gate = release.get_future();
  std::vector<int> order;
  auto first = scheduler_.Schedule([&] {
    started.set_value();
    EXPECT_EQ(gate.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
    return std::make_unique<int>(1);
  });
  auto second = scheduler_.Schedule([&] {
    order.push_back(2);
    release.set_value();
    return std::make_unique<int>(2);
  });
  auto third = scheduler_.Schedule([&] {
    order.push_back(3);
    return std::make_unique<int>(3);
  });
  auto batch = scheduler_.TakeBatch();
  int later_preparations = 0;
  auto later = scheduler_.Schedule([&] {
    ++later_preparations;
    return std::make_unique<int>(4);
  });
  scheduler_.ActivateBatch(std::move(batch));
  std::thread worker([&] { workers_[0](); });
  started.get_future().wait();
  auto first_result = scheduler_.Consume(first);
  worker.join();
  ASSERT_TRUE(first_result);
  EXPECT_EQ(**first_result, 1);
  EXPECT_EQ(order, (std::vector<int>{3, 2}));
  EXPECT_EQ(later_preparations, 0);
  // Assistance neither gets these futures nor runs a caller's finalizer.
  EXPECT_EQ(**scheduler_.Consume(second), 2);
  EXPECT_EQ(**scheduler_.Consume(third), 3);
  scheduler_.ActivateBatch(scheduler_.TakeBatch());
  EXPECT_EQ(**scheduler_.Consume(later), 4);
  EXPECT_FALSE(scheduler_.TakeBatch());
}

TEST_F(CreateViewSchedulerTest, ContextFreeTasksDispatchOnAttachmentInOrder) {
  std::vector<int> order;
  auto first = scheduler_.Schedule(
      [&] {
        order.push_back(1);
        return std::make_unique<int>(1);
      },
      true);
  auto second = scheduler_.Schedule(
      [&] {
        order.push_back(2);
        return std::make_unique<int>(2);
      },
      true);
  EXPECT_TRUE(workers_.empty());
  EXPECT_FALSE(scheduler_.TakeBatch());
  // A UI flush can claim a deferred task before attachment.
  EXPECT_EQ(**scheduler_.Consume(first), 1);
  scheduler_.DispatchDeferred();
  ASSERT_EQ(workers_.size(), 2u);
  workers_[0]();
  workers_[1]();
  scheduler_.DispatchDeferred();
  EXPECT_EQ(workers_.size(), 2u);
  EXPECT_EQ(order, (std::vector<int>{1, 2}));
  EXPECT_EQ(**scheduler_.Consume(second), 2);
  EXPECT_TRUE(scheduler_.TakeBatch());
  EXPECT_FALSE(scheduler_.TakeBatch());
}

TEST_F(CreateViewSchedulerTest, ReentrantConsumptionClaimsBeforePreparation) {
  Scheduler::TaskRef task;
  task = scheduler_.Schedule([&] {
    EXPECT_FALSE(scheduler_.Consume(task));
    scheduler_.ResetBatch();
    // Consume must retain the task even when its caller releases it.
    task = nullptr;
    return std::make_unique<int>(9);
  });
  scheduler_.ActivateBatch(scheduler_.TakeBatch());
  EXPECT_EQ(**scheduler_.Consume(task), 9);
  workers_[0]();
}

TEST_F(CreateViewSchedulerTest,
       ReentrantFlushRetainsOldBatchAndAdvancesCursor) {
  std::promise<void> started;
  std::promise<void> release;
  auto gate = release.get_future();
  auto first = scheduler_.Schedule([&] {
    started.set_value();
    EXPECT_EQ(gate.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
    return std::make_unique<int>(1);
  });
  auto second = scheduler_.Schedule([&] {
    release.set_value();
    return std::make_unique<int>(2);
  });
  Scheduler::TaskRef later;
  Scheduler::TaskRef third;
  third = scheduler_.Schedule([&] {
    EXPECT_FALSE(scheduler_.Consume(third));
    // Nested consumption must skip the preparation currently on the stack.
    EXPECT_FALSE(scheduler_.Consume(first));
    later = scheduler_.Schedule([] { return std::make_unique<int>(4); });
    scheduler_.ActivateBatch(scheduler_.TakeBatch());
    return std::make_unique<int>(3);
  });
  scheduler_.ActivateBatch(scheduler_.TakeBatch());
  std::thread worker([&] { workers_[0](); });
  started.get_future().wait();
  EXPECT_EQ(**scheduler_.Consume(first), 1);
  worker.join();
  EXPECT_EQ(**scheduler_.Consume(second), 2);
  EXPECT_EQ(**scheduler_.Consume(third), 3);
  EXPECT_EQ(**scheduler_.Consume(later), 4);
}

TEST_F(CreateViewSchedulerTest, NestedWaitSharesAdvancedAssistanceCursor) {
  std::promise<void> first_started, second_started, release_first,
      release_second;
  auto first_gate = release_first.get_future();
  auto second_gate = release_second.get_future();
  auto first = scheduler_.Schedule([&] {
    first_started.set_value();
    EXPECT_EQ(first_gate.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
    return std::make_unique<int>(1);
  });
  auto release = scheduler_.Schedule([&] {
    release_second.set_value();
    return std::make_unique<int>(2);
  });
  auto second = scheduler_.Schedule([&] {
    second_started.set_value();
    EXPECT_EQ(second_gate.wait_for(std::chrono::seconds(5)),
              std::future_status::ready);
    return std::make_unique<int>(3);
  });
  auto nested = scheduler_.Schedule([&] {
    EXPECT_EQ(**scheduler_.Consume(second), 3);
    release_first.set_value();
    return std::make_unique<int>(4);
  });
  scheduler_.ActivateBatch(scheduler_.TakeBatch());
  std::thread first_worker([&] { workers_[0](); });
  std::thread second_worker([&] { workers_[2](); });
  first_started.get_future().wait();
  second_started.get_future().wait();
  EXPECT_EQ(**scheduler_.Consume(first), 1);
  first_worker.join();
  second_worker.join();
  EXPECT_FALSE(scheduler_.Consume(second));
  EXPECT_EQ(**scheduler_.Consume(release), 2);
  EXPECT_EQ(**scheduler_.Consume(nested), 4);
}

TEST_F(CreateViewSchedulerTest,
       PreparationCapturesReleaseAndPagesStayIndependent) {
  Scheduler other([](base::closure) {});
  for (int i = 0; i < 20; ++i) {
    auto owner = std::make_shared<int>(i);
    std::weak_ptr<int> weak = owner;
    auto task = scheduler_.Schedule(
        [owner = std::move(owner)] { return std::make_unique<int>(*owner); });
    scheduler_.ActivateBatch(scheduler_.TakeBatch());
    EXPECT_FALSE(other.TakeBatch());
    EXPECT_EQ(**scheduler_.Consume(task), i);
    // Preparation captures are released even while the posted task is retained.
    EXPECT_TRUE(weak.expired());
    scheduler_.ResetBatch();
    EXPECT_FALSE(scheduler_.TakeBatch());
  }
}

TEST(CreateViewSchedulerDispatchTest, DefaultDispatchUsesHighPriorityLoop) {
  Scheduler scheduler;
  auto on_worker = std::make_shared<std::promise<bool>>();
  auto dispatched = on_worker->get_future();
  auto task = scheduler.Schedule([on_worker] {
    on_worker->set_value(base::TaskRunnerManufactor::IsOnConcurrentLoopWorker(
        base::ConcurrentTaskType::HIGH_PRIORITY));
    return std::make_unique<int>(1);
  });
  ASSERT_EQ(dispatched.wait_for(std::chrono::seconds(5)),
            std::future_status::ready);
  EXPECT_TRUE(dispatched.get());
  EXPECT_EQ(**scheduler.Consume(task), 1);
}

TEST(CreateViewSchedulerLifetimeTest,
     ReplacingAndResettingBatchesReleasesResults) {
  std::vector<base::closure> workers;
  CreateViewScheduler<std::shared_ptr<int>> scheduler(
      [&](base::closure task) { workers.push_back(std::move(task)); });
  std::weak_ptr<int> previous;
  for (int i = 0; i < 20; ++i) {
    auto result = std::make_shared<int>(i);
    std::weak_ptr<int> current = result;
    scheduler.Schedule([result = std::move(result)] { return result; });
    workers.back()();
    workers.clear();
    scheduler.ActivateBatch(scheduler.TakeBatch());
    EXPECT_TRUE(previous.expired());
    EXPECT_FALSE(current.expired());
    previous = current;
  }
  scheduler.ResetBatch();
  EXPECT_TRUE(previous.expired());
}

TEST(CreateViewSchedulerLifetimeTest,
     ConsumeRetainsStateAcrossSchedulerTeardown) {
  base::closure worker;
  auto scheduler = std::make_unique<Scheduler>(
      [&](base::closure task) { worker = std::move(task); });
  auto task = scheduler->Schedule([&] {
    scheduler.reset();
    return std::make_unique<int>(5);
  });
  scheduler->ActivateBatch(scheduler->TakeBatch());
  EXPECT_EQ(**scheduler->Consume(task), 5);
  EXPECT_FALSE(scheduler);
  worker();
}

TEST(CreateViewSchedulerLifetimeTest, WorkerOutlivesScheduler) {
  base::closure worker;
  int preparations = 0;
  {
    Scheduler scheduler([&](base::closure task) { worker = std::move(task); });
    scheduler.Schedule([&] {
      ++preparations;
      return std::make_unique<int>(1);
    });
  }
  worker();
  worker();
  EXPECT_EQ(preparations, 1);
}

}  // namespace
}  // namespace tasm
}  // namespace lynx
