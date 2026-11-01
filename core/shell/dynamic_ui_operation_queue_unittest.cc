// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/dynamic_ui_operation_queue.h"

#include <memory>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/renderer/ui_wrapper/painting/empty/painting_context_implementation.h"
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

TEST_F(DynamicUIOperationQueueTest,
       PaintingContextHandoffRemovesOldNodesBeforeInsertingNewNodes) {
  class PlatformRef : public tasm::PaintingCtxPlatformRef {
   public:
    void RemovePaintingNode(int parent, int child, int index,
                            bool is_move) override {
      ASSERT_EQ(parent, 0);
      ASSERT_FALSE(is_move);
      ASSERT_GE(index, 0);
      ASSERT_LT(static_cast<size_t>(index), children.size());
      ASSERT_EQ(children[index], child);
      children.erase(children.begin() + index);
      mutations.push_back(-child);
    }
    void InsertPaintingNode(int parent, int child, int index) override {
      ASSERT_EQ(parent, 0);
      ASSERT_GE(index, 0);
      ASSERT_LE(static_cast<size_t>(index), children.size());
      children.insert(children.begin() + index, child);
      mutations.push_back(child);
    }
    std::vector<int> children{1, 2};
    std::vector<int> mutations;
  };
  class PlatformImpl : public tasm::PaintingContextPlatformImpl {
   public:
    explicit PlatformImpl(const std::shared_ptr<PlatformRef>& ref) {
      platform_ref_ = ref;
    }
    bool EnableUIOperationQueue() override { return true; }
  };

  auto tasm_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  auto platform = std::make_shared<PlatformRef>();
  tasm::PaintingContext context(std::make_unique<PlatformImpl>(platform));
  auto old_queue = std::make_shared<DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::MULTI_THREADS, ui_runner);
  auto new_queue = std::make_shared<DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::MULTI_THREADS, ui_runner);
  old_queue->SetEnableFlush(false);
  new_queue->SetEnableFlush(false);
  ui_runner->PostSyncTask([&] { context.SetUIOperationQueue(old_queue); });
  tasm_runner->PostSyncTask([&] {
    // One removal is ready for UI; another is still pending on TASM.
    context.RemovePaintingNode(0, 1, 0, false);
    old_queue->Flush();
    context.RemovePaintingNode(0, 2, 0, false);
  });
  ui_runner->PostSyncTask([&] {
    old_queue->Destroy();
    context.SetUIOperationQueue(new_queue);
    EXPECT_TRUE(platform->mutations.empty());
  });
  tasm_runner->PostSyncTask([&] {
    context.InsertPaintingNode(0, 3, 0);
    new_queue->Flush();
  });
  ui_runner->PostSyncTask([&] {
    new_queue->SetEnableFlush(true);
    new_queue->ForceFlush();
    EXPECT_EQ(platform->children, (std::vector<int>{3}));
    EXPECT_EQ(platform->mutations, (std::vector<int>{-1, -2, 3}));
    old_queue->ForceFlush();
    new_queue->ForceFlush();
    EXPECT_EQ(platform->mutations, (std::vector<int>{-1, -2, 3}));
  });
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
