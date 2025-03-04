// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/lynx_ui_operation_async_queue.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {
class LynxUIOperationAsyncQueueTest : public ::testing::Test {
 protected:
  LynxUIOperationAsyncQueueTest() = default;
  ~LynxUIOperationAsyncQueueTest() override = default;
  fml::RefPtr<fml::TaskRunner> ui_runner_ =
      MockRunnerManufactor::GetHookUITaskRunner();
  fml::RefPtr<fml::TaskRunner> tasm_runner_ =
      MockRunnerManufactor::GetHookTASMTaskRunner();
  std::shared_ptr<shell::LynxUIOperationAsyncQueue> queue_ =
      std::make_shared<shell::LynxUIOperationAsyncQueue>(ui_runner_);

  fml::AutoResetWaitableEvent arwe_;

  int32_t result_ = 0;
  int32_t expect_ = 0;
  static constexpr int32_t kOperationCounts = 10;

  void EnqueueOperations() {
    queue_->SetEnableFlush(true);
    for (int32_t i = 0; i < kOperationCounts; ++i) {
      queue_->EnqueueUIOperation([this]() { ++result_; });
      ++expect_;
    }
  }

  void SetUp() override { EnqueueOperations(); }
};

TEST_F(LynxUIOperationAsyncQueueTest, FlushOnTasmThread) {
  queue_->EnqueueUIOperation([this]() { arwe_.Signal(); });
  tasm_runner_->PostTask([this]() { queue_->Flush(); });
  arwe_.Wait();
  ASSERT_EQ(result_, expect_);
}

TEST_F(LynxUIOperationAsyncQueueTest, FlushOnTasmThreadForInsertUIOp) {
  std::shared_ptr<fml::AutoResetWaitableEvent> arwe_on_ui =
      std::make_shared<fml::AutoResetWaitableEvent>();
  ui_runner_->PostTask([arwe_on_ui]() {
    // Wait until tasm calls flush twice.
    arwe_on_ui->Wait();
  });
  tasm_runner_->PostTask([this, arwe_on_ui]() {
    queue_->Flush();
    queue_->EnqueueUIOperation([this]() { arwe_.Signal(); });
    queue_->Flush();
    // Signal ui thread.
    arwe_on_ui->Signal();
  });
  arwe_.Wait();
  ASSERT_EQ(result_, expect_);
}

TEST_F(LynxUIOperationAsyncQueueTest, FlushOnUIThread) {
  ui_runner_->PostTask([this]() {
    queue_->Flush();
    arwe_.Signal();
  });
  tasm_runner_->PostTask([this]() {
    queue_->UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue_->Flush();
    queue_->UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue_->Flush();
  });
  arwe_.Wait();
  ASSERT_EQ(result_, expect_);
}

TEST_F(LynxUIOperationAsyncQueueTest, ForceFlush) {
  queue_->SetEnableFlush(false);
  queue_->EnqueueUIOperation([this]() { arwe_.Signal(); });
  tasm_runner_->PostTask([this]() {
    queue_->Flush();
    arwe_.Signal();
  });
  arwe_.Wait();
  ui_runner_->PostTask([this]() { queue_->ForceFlush(); });
  arwe_.Wait();
  ASSERT_EQ(result_, expect_);
}

TEST_F(LynxUIOperationAsyncQueueTest, FlushOnUIThreadTimeout) {
  tasm_runner_->PostTask([this]() {
    queue_->Flush();
    ui_runner_->PostTask([this]() {
      // Without updating queue's status, ui_runner will wait until timeout.
      queue_->Flush();
      arwe_.Signal();
    });
  });

  arwe_.Wait();
  ASSERT_EQ(result_, expect_);
}

TEST_F(LynxUIOperationAsyncQueueTest, FlushOnUIThreadWhenDestroyed) {
  bool executed = false;
  queue_->LynxUIOperationQueue::EnqueueUIOperation(
      [&executed]() { executed = true; });

  queue_->Destroy();
  ui_runner_->PostTask([this]() {
    queue_->Flush();
    arwe_.Signal();
  });
  arwe_.Wait();
  // We expect that the operation will not be executed since the queue has been
  // destroyed.
  ASSERT_FALSE(executed);
}

TEST_F(LynxUIOperationAsyncQueueTest, FlushOnUIThreadWhenAllFinish) {
  bool executed = false;
  queue_->LynxUIOperationQueue::EnqueueUIOperation(
      [&executed]() { executed = true; });

  queue_->UpdateStatus(shell::UIOperationStatus::ALL_FINISH);
  ui_runner_->PostTask([this]() {
    queue_->Flush();
    arwe_.Signal();
  });
  arwe_.Wait();
  // We expect that the operation will not be executed since the status has been
  // updated to ALL_FINISH.
  ASSERT_FALSE(executed);
}

TEST_F(LynxUIOperationAsyncQueueTest, FlushHighPriorityTask) {
  int32_t result = 0;
  int32_t count = 0;
  queue_->EnqueueUIOperation([this, &result, &count]() {
    result = 1;
    count++;
    arwe_.Signal();
  });
  queue_->EnqueueHighPriorityOperation([&result, &count]() {
    result = 2;
    count++;
  });

  tasm_runner_->PostTask([this]() { queue_->Flush(); });
  arwe_.Wait();

  // expect that the HighPriorityOperation will be executed first.
  ASSERT_EQ(result, 1);
  ASSERT_EQ(count, 2);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
