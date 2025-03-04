// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/dynamic_ui_operation_queue.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class DynamicUIOperationQueueTest : public ::testing::Test {
 protected:
  DynamicUIOperationQueueTest() = default;
  ~DynamicUIOperationQueueTest() override = default;

  static constexpr int32_t kExpect = 10;

  static void SetUpTestSuite() { base::UIThread::Init(); }
};

TEST_F(DynamicUIOperationQueueTest, AllOnUIFlush) {
  int32_t result = 0;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::ALL_ON_UI,
                                MockRunnerManufactor::GetHookUITaskRunner());

  for (int32_t i = 0; i < kExpect; ++i) {
    queue.EnqueueUIOperation([&result] { ++result; });
  }

  queue.Flush();

  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, MultiThreadsFlush) {
  int32_t result = 0;
  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  fml::AutoResetWaitableEvent arwe;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());

  tasm_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  tasm_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, SyncToSync) {
  int32_t result = 0;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::ALL_ON_UI,
                                MockRunnerManufactor::GetHookUITaskRunner());
  fml::AutoResetWaitableEvent arwe;
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();

  ui_runner->PostTask([&result, &queue, &arwe]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Transfer(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
    arwe.Signal();
  });

  arwe.Wait();
  ASSERT_EQ(result, 0);

  arwe.Reset();
  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  arwe.Wait();
  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, AsyncToAsync) {
  int32_t result = 0;
  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  fml::AutoResetWaitableEvent arwe;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());

  tasm_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&tasm_runner, &queue, &result]() {
    // Stop tasm thread.
    tasm_runner->PostSyncTask([]() {});

    queue.Transfer(base::ThreadStrategyForRendering::MOST_ON_TASM);
    ASSERT_EQ(result, 0);
  });

  tasm_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, SyncToAsync) {
  int32_t result = 0;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::ALL_ON_UI,
                                MockRunnerManufactor::GetHookUITaskRunner());
  fml::AutoResetWaitableEvent arwe;
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();

  ui_runner->PostTask([&result, &queue, &arwe]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Transfer(base::ThreadStrategyForRendering::MULTI_THREADS);
    arwe.Signal();
  });

  arwe.Wait();
  ASSERT_EQ(result, kExpect);

  result = 0;

  tasm_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  tasm_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, AsyncToSync) {
  int32_t result = 0;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());
  fml::AutoResetWaitableEvent arwe;
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();

  tasm_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Flush();
  });

  ui_runner->PostTask([&tasm_runner, &queue, &arwe]() {
    // Stop tasm thread.
    tasm_runner->PostSyncTask([]() {});

    queue.Transfer(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
    arwe.Signal();
  });

  arwe.Wait();
  ASSERT_EQ(result, kExpect);

  result = 0;

  arwe.Reset();
  ui_runner->PostTask([&result, &queue, &arwe]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Flush();
    arwe.Signal();
  });

  arwe.Wait();
  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, NestedFlush) {
  int32_t result = 0;
  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  fml::AutoResetWaitableEvent arwe;
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());

  tasm_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result, &queue] {
        queue.Transfer(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
        ++result;
      });
    }
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  tasm_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

TEST_F(DynamicUIOperationQueueTest, EnqueueDuringForceFlush) {
  int32_t result = 0;
  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());

  tasm_runner->PostTask([&result, &queue] {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&queue, &result]() {
        queue.EnqueueUIOperation([&result]() { ++result; });
      });
    }

    queue.Flush();
  });

  ui_runner->PostSyncTask([&queue, &tasm_runner]() {
    tasm_runner->PostSyncTask([]() {});
    queue.Transfer(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
    queue.Flush();
  });

  ASSERT_EQ(result, kExpect);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
