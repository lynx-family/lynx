// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/tasm_operation_queue.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class TASMOperationQueueTest : public ::testing::Test {
 protected:
  TASMOperationQueueTest() = default;
  ~TASMOperationQueueTest() override = default;

  static constexpr int32_t kOperationCounts = 10;
};

TEST_F(TASMOperationQueueTest, FlushNonTrivialOperations) {
  TASMOperationQueue queue;

  int32_t ret = 0;
  ASSERT_FALSE(queue.Flush());

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    queue.EnqueueOperation([&ret]() { ++ret; });
  }

  queue.AppendPendingTask();

  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(queue.Flush());
  ASSERT_EQ(ret, kOperationCounts);

  ASSERT_FALSE(queue.Flush());
}

TEST_F(TASMOperationQueueTest, FlushTrivialOperations) {
  TASMOperationQueue queue;

  int32_t ret = 0;
  ASSERT_FALSE(queue.Flush());

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    queue.EnqueueTrivialOperation([&ret]() { ++ret; });
  }

  queue.AppendPendingTask();

  ASSERT_EQ(ret, 0);

  ASSERT_FALSE(queue.Flush());
  ASSERT_EQ(ret, kOperationCounts);

  ASSERT_FALSE(queue.Flush());
}

TEST_F(TASMOperationQueueTest, FlushTrivialAndNonTrivialOperations) {
  TASMOperationQueue queue;

  int32_t ret = 0;
  ASSERT_FALSE(queue.Flush());

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    queue.EnqueueTrivialOperation([&ret]() { ++ret; });
  }

  for (int32_t i = 0; i < kOperationCounts; ++i) {
    queue.EnqueueOperation([&ret]() { ++ret; });
  }

  queue.AppendPendingTask();

  ASSERT_EQ(ret, 0);

  ASSERT_TRUE(queue.Flush());
  ASSERT_EQ(ret, kOperationCounts + kOperationCounts);

  ASSERT_FALSE(queue.Flush());
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
