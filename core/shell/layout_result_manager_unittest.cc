// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/layout_result_manager.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class LayoutResultManagerTest : public ::testing::Test {
 protected:
  LayoutResultManagerTest() = default;
  ~LayoutResultManagerTest() override = default;

  static constexpr int32_t kOperationCounts = 10;

  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(LayoutResultManagerTest, ExecuteNonTrivialOperations) {
  LayoutResultManager layout_result_manager;

  int32_t ret = 0;
  auto operations = layout_result_manager.FetchTASMOperations();

  ASSERT_TRUE(operations.empty());
  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));

  ASSERT_EQ(ret, 0);

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    layout_result_manager.EnqueueOperation([&ret]() { ++ret; });
  }

  ASSERT_EQ(ret, 0);

  operations = layout_result_manager.FetchTASMOperations();
  ASSERT_EQ(operations.size(), kOperationCounts);

  ASSERT_TRUE(LayoutResultManager::ExecuteTASMOperations(operations));
  ASSERT_EQ(ret, kOperationCounts);

  operations = layout_result_manager.FetchTASMOperations();

  ASSERT_TRUE(operations.empty());
  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));
  ASSERT_EQ(ret, kOperationCounts);
}

TEST_F(LayoutResultManagerTest, ExecuteTrivialOperations) {
  LayoutResultManager layout_result_manager;

  int32_t ret = 0;
  auto operations = layout_result_manager.FetchTASMOperations();

  ASSERT_TRUE(operations.empty());
  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));

  ASSERT_EQ(ret, 0);

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    layout_result_manager.EnqueueTrivialOperation([&ret]() { ++ret; });
  }

  ASSERT_EQ(ret, 0);

  operations = layout_result_manager.FetchTASMOperations();
  ASSERT_EQ(operations.size(), kOperationCounts);

  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));
  ASSERT_EQ(ret, kOperationCounts);

  operations = layout_result_manager.FetchTASMOperations();

  ASSERT_TRUE(operations.empty());
  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));
  ASSERT_EQ(ret, kOperationCounts);
}

TEST_F(LayoutResultManagerTest, ExecuteTrivialAndNonTrivialOperations) {
  LayoutResultManager layout_result_manager;

  int32_t ret = 0;
  auto operations = layout_result_manager.FetchTASMOperations();

  ASSERT_TRUE(operations.empty());
  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));

  ASSERT_EQ(ret, 0);

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    layout_result_manager.EnqueueOperation([&ret]() { ++ret; });
  }

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    layout_result_manager.EnqueueTrivialOperation([&ret]() { ++ret; });
  }

  ASSERT_EQ(ret, 0);

  operations = layout_result_manager.FetchTASMOperations();
  ASSERT_EQ(operations.size(), kOperationCounts + kOperationCounts);

  ASSERT_TRUE(LayoutResultManager::ExecuteTASMOperations(operations));
  ASSERT_EQ(ret, kOperationCounts + kOperationCounts);

  operations = layout_result_manager.FetchTASMOperations();

  ASSERT_TRUE(operations.empty());
  ASSERT_FALSE(LayoutResultManager::ExecuteTASMOperations(operations));
  ASSERT_EQ(ret, kOperationCounts + kOperationCounts);
}

TEST_F(LayoutResultManagerTest, RunOnLayoutAfterTasks) {
  LayoutResultManager layout_result_manager;

  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  fml::AutoResetWaitableEvent arwe;

  int32_t ret = 0;

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    for (int32_t i = 0; i < kOperationCounts; ++i) {
      layout_result_manager.EnqueueOperation([&ret]() { ++ret; });
    }

    auto operations = layout_result_manager.FetchTASMOperations();

    auto on_layout_after_task = [&ret, operations = std::move(operations),
                                 count = i + 1]() {
      ASSERT_EQ(operations.size(), kOperationCounts);

      ASSERT_TRUE(LayoutResultManager::ExecuteTASMOperations(operations));

      ASSERT_EQ(ret, kOperationCounts * count);
    };

    layout_result_manager.EnqueueOnLayoutAfterTask(
        std::move(on_layout_after_task));

    tasm_runner->PostTask([&layout_result_manager]() {
      layout_result_manager.RunOnLayoutAfterTasks();
    });
  }

  tasm_runner->PostTask([&arwe]() { arwe.Signal(); });

  arwe.Wait();

  ASSERT_EQ(ret, kOperationCounts * kOperationCounts);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
