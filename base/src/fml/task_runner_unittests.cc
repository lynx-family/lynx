// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/task_runner.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {
namespace testing {

class TaskRunnerTest : public ::testing::Test {
 protected:
  TaskRunnerTest() = default;
  ~TaskRunnerTest() override = default;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(TaskRunnerTest, Bind) {}

}  // namespace testing
}  // namespace fml
}  // namespace lynx
