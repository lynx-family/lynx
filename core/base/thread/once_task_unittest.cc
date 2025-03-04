// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/thread/once_task.h"

#include <functional>
#include <vector>

#include "base/include/log/logging.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class OnceTaskTest : public ::testing::Test {
 protected:
  OnceTaskTest() {}
  ~OnceTaskTest() {}

  void SetUp() override {}

  void TearDown() override {}

  std::vector<std::function<void()>> GenerateTask(int* parallelArray,
                                                  int* reduceArray, int index) {
    *(parallelArray + index) += 1;

    return std::vector<std::function<void()>>{
        [reduceArray, index]() { *(reduceArray + index) = index; }};
  }
};

TEST_F(OnceTaskTest, TestOnceTask0) {
  using TestOnceTaskRefptr = OnceTaskRefptr<std::vector<std::function<void()>>>;

  int index = 10000;
  int* parallelArray = new int[index];
  std::fill(parallelArray, parallelArray + index, 0);

  int* reduceArray = new int[index];
  std::fill(reduceArray, reduceArray + index, 0);

  std::vector<TestOnceTaskRefptr> task_vec{};

  for (int i = 0; i < index; ++i) {
    std::promise<std::vector<std::function<void()>>> promise;
    std::future<std::vector<std::function<void()>>> future =
        promise.get_future();

    base::closure task_with_promise =
        base::closure([this, parallelArray, reduceArray, i,
                       promise = std::move(promise)]() mutable {
          promise.set_value(GenerateTask(parallelArray, reduceArray, i));
        });

    TestOnceTaskRefptr task_info_ptr = fml::AdoptRef(
        new OnceTask(std::move(task_with_promise), std::move(future)));

    base::closure task_with_guard =
        base::closure([task_info_ptr]() { task_info_ptr->Run(); });

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        std::move(task_with_guard), base::ConcurrentTaskType::HIGH_PRIORITY);
    task_vec.emplace_back(std::move(task_info_ptr));
  }

  for (auto iter = task_vec.rbegin(); iter != task_vec.rend(); ++iter) {
    if (!(*iter)->Run()) {
      break;
    }
  }

  for (auto& task : task_vec) {
    if (task->GetFuture().valid()) {
      std::vector<std::function<void()>> operations = task->GetFuture().get();
      for (auto it = operations.begin(); it != operations.end(); it++) {
        (*it)();
      }
    }
  }

  for (int i = 0; i < index; ++i) {
    EXPECT_EQ(*(parallelArray + i), 1);
    EXPECT_EQ(*(reduceArray + i), i);
  }

  delete[] parallelArray;
  delete[] reduceArray;
}

TEST_F(OnceTaskTest, TestOnceTask1) {
  using TestOnceTaskRefptr =
      OnceTaskRefptr<std::vector<std::function<void()>>, int>;

  int index = 10000;
  int* parallelArray = new int[index];
  std::fill(parallelArray, parallelArray + index, 0);

  int* reduceArray = new int[index];
  std::fill(reduceArray, reduceArray + index, 0);

  std::vector<TestOnceTaskRefptr> task_vec{};

  for (int i = 0; i < index; ++i) {
    std::promise<std::vector<std::function<void()>>> promise;
    std::future<std::vector<std::function<void()>>> future =
        promise.get_future();

    base::MoveOnlyClosure<void, int> task_with_promise(
        [this, parallelArray, reduceArray,
         promise = std::move(promise)](int index) mutable {
          promise.set_value(GenerateTask(parallelArray, reduceArray, index));
        });

    TestOnceTaskRefptr task_info_ptr = fml::AdoptRef(
        new OnceTask(std::move(task_with_promise), std::move(future)));

    base::closure task_with_guard = base::closure(
        [task_info_ptr, i]() mutable { task_info_ptr->Run(std::move(i)); });

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        std::move(task_with_guard), base::ConcurrentTaskType::HIGH_PRIORITY);
    task_vec.emplace_back(std::move(task_info_ptr));
  }

  for (auto& task : task_vec) {
    if (task->GetFuture().valid()) {
      std::vector<std::function<void()>> operations = task->GetFuture().get();
      for (auto it = operations.begin(); it != operations.end(); it++) {
        (*it)();
      }
    }
  }

  for (int i = 0; i < index; ++i) {
    EXPECT_EQ(*(parallelArray + i), 1);
    EXPECT_EQ(*(reduceArray + i), i);
  }

  delete[] parallelArray;
  delete[] reduceArray;
}

}  // namespace base
}  // namespace lynx
