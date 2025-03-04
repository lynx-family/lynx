// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <thread>

#include "base/include/fml/macros.h"
#include "base/include/fml/task_source.h"
#include "base/include/fml/time/chrono_timestamp_provider.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {
namespace testing {

TEST(TaskSourceTests, SimpleInitialization) {
  TaskSource task_source = TaskSource(TaskQueueId(1));
  task_source.RegisterTask(
      {1, [] {}, ChronoTicksSinceEpoch(), TaskSourceGrade::kUnspecified});
  ASSERT_EQ(task_source.GetNumPendingTasks(), 1u);
}

TEST(TaskSourceTests, MultipleTaskGrades) {
  TaskSource task_source = TaskSource(TaskQueueId(1));
  task_source.RegisterTask(
      {1, [] {}, ChronoTicksSinceEpoch(), TaskSourceGrade::kUnspecified});
  task_source.RegisterTask(
      {2, [] {}, ChronoTicksSinceEpoch(), TaskSourceGrade::kUserInteraction});
  task_source.RegisterTask(
      {3, [] {}, ChronoTicksSinceEpoch(), TaskSourceGrade::kEmergency});
  task_source.RegisterTask(
      {3, [] {}, ChronoTicksSinceEpoch(), TaskSourceGrade::kIdle});
  task_source.RegisterTask(
      {4, [] {}, ChronoTicksSinceEpoch(), TaskSourceGrade::kMicrotask});
  ASSERT_EQ(task_source.GetNumPendingTasks(), 5u);
}

TEST(TaskSourceTests, SimpleOrdering) {
  TaskSource task_source = TaskSource(TaskQueueId(1));
  auto time_stamp = ChronoTicksSinceEpoch();
  int value = 0;
  task_source.RegisterTask(
      {1, [&] { value = 1; }, time_stamp, TaskSourceGrade::kUnspecified});
  task_source.RegisterTask({2, [&] { value = 7; },
                            time_stamp + fml::TimeDelta::FromMilliseconds(1),
                            TaskSourceGrade::kUnspecified});
  task_source.Top().task.GetTask()();
  task_source.PopTask(TaskSourceGrade::kUnspecified);
  ASSERT_EQ(value, 1);
  task_source.Top().task.GetTask()();
  task_source.PopTask(TaskSourceGrade::kUnspecified);
  ASSERT_EQ(value, 7);
}

TEST(TaskSourceTests, SimpleOrderingMultiTaskHeaps) {
  TaskSource task_source = TaskSource(TaskQueueId(1));
  auto time_stamp = ChronoTicksSinceEpoch();
  int value = 0;
  task_source.RegisterTask({0, [&] { value = 17; },
                            time_stamp + fml::TimeDelta::FromMilliseconds(1),
                            TaskSourceGrade::kIdle});
  task_source.RegisterTask({1, [&] { value = 1; },
                            time_stamp + fml::TimeDelta::FromMilliseconds(1),
                            TaskSourceGrade::kUserInteraction});
  task_source.RegisterTask({2, [&] { value = 7; },
                            time_stamp + fml::TimeDelta::FromMilliseconds(1),
                            TaskSourceGrade::kEmergency});
  task_source.RegisterTask({3, [&] { value = 20; },
                            time_stamp + fml::TimeDelta::FromMilliseconds(1),
                            TaskSourceGrade::kMicrotask});
  auto zero_task = task_source.Top();
  zero_task.task.GetTask()();
  task_source.PopTask(zero_task.task.GetTaskSourceGrade());
  ASSERT_EQ(value, 20);

  auto top_task = task_source.Top();
  top_task.task.GetTask()();
  task_source.PopTask(top_task.task.GetTaskSourceGrade());
  ASSERT_EQ(value, 7);

  auto second_task = task_source.Top();
  second_task.task.GetTask()();
  task_source.PopTask(second_task.task.GetTaskSourceGrade());
  ASSERT_EQ(value, 1);

  auto third_task = task_source.Top();
  third_task.task.GetTask()();
  task_source.PopTask(third_task.task.GetTaskSourceGrade());
  ASSERT_EQ(value, 17);
}

}  // namespace testing
}  // namespace fml
}  // namespace lynx
