// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/utils/base/tasm_worker_task_runner.h"

#include "core/renderer/utils/base/tasm_worker_basic_task_runner.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

class TasmWorkerEnvironment : public ::testing::Environment {
 public:
  virtual void SetUp() override {}

  virtual void TearDown() override {
    TasmWorkerBasicTaskRunner::GetTasmWorkerBasicTaskRunner().Join();
  }
};

::testing::Environment* const tasm_env =
    ::testing::AddGlobalTestEnvironment(new TasmWorkerEnvironment);

class TasmWorkerTaskRunnerTest : public ::testing::Test {
 public:
  TasmWorkerTaskRunnerTest() {}
  ~TasmWorkerTaskRunnerTest() override {}

  void SetUp() override {
    task_runner_ = std::make_shared<TasmWorkerTaskRunner>();
  }

  std::shared_ptr<TasmWorkerTaskRunner> task_runner_;
};

struct ExecutionStatistics {
  int execute_count = 0;
};

TEST_F(TasmWorkerTaskRunnerTest, PostTask) {
  auto statistics = std::make_shared<ExecutionStatistics>();
  task_runner_->PostTask([statistics]() { statistics->execute_count++; });
  task_runner_->WaitForCompletion();
  EXPECT_TRUE(statistics->execute_count == 1);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
