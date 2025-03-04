// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/thread_mode_auto_switch.h"

#include "base/include/fml/synchronization/waitable_event.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class ThreadModeAutoSwitchTest : public ::testing::Test {
 protected:
  ThreadModeAutoSwitchTest() = default;
  ~ThreadModeAutoSwitchTest() override = default;

  static constexpr int32_t kExpect = 10;

  static void SetUpTestSuite() { base::UIThread::Init(); }
};

TEST_F(ThreadModeAutoSwitchTest, AutoSwitch) {
  int32_t result = 0;
  fml::AutoResetWaitableEvent arwe;

  auto* ui_runner = MockRunnerManufactor::GetHookUITaskRunner().get();
  auto* engine_runner = MockRunnerManufactor::GetHookTASMTaskRunner().get();
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());
  ThreadModeManager manager{ui_runner, engine_runner, &queue};

  engine_runner->PostEmergencyTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Flush();
  });

  ui_runner->PostTask([&manager, &result, &queue]() {
    ThreadModeAutoSwitch auto_switch(manager);

    ASSERT_EQ(result, kExpect);
    result = 0;

    // now work as PART_ON_LAYOUT/ALL_ON_UI
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&arwe]() { arwe.Signal(); });

  arwe.Wait();
  arwe.Reset();
  ASSERT_EQ(result, kExpect);

  result = 0;

  engine_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  engine_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

TEST_F(ThreadModeAutoSwitchTest, DISABLED_ConstructWithNullValue) {
  int32_t result = 0;
  fml::AutoResetWaitableEvent arwe;

  auto* ui_runner = MockRunnerManufactor::GetHookUITaskRunner().get();
  auto* engine_runner = MockRunnerManufactor::GetHookTASMTaskRunner().get();
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());

  engine_runner->PostEmergencyTask([&queue, &result, &ui_runner, &arwe]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Flush();

    ui_runner->PostTask([&result, &arwe]() {
      ASSERT_EQ(result, kExpect);
      result = 0;
      arwe.Signal();
    });
  });

  ui_runner->PostTask([&result]() {
    ThreadModeManager manager;
    ThreadModeAutoSwitch auto_switch(manager);

    ASSERT_EQ(result, 0);
  });

  arwe.Wait();
  arwe.Reset();

  engine_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  engine_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

TEST_F(ThreadModeAutoSwitchTest, DuplicatedAutoSwitch) {
  int32_t result = 0;
  fml::AutoResetWaitableEvent arwe;

  auto* ui_runner = MockRunnerManufactor::GetHookUITaskRunner().get();
  auto* engine_runner = MockRunnerManufactor::GetHookTASMTaskRunner().get();
  DynamicUIOperationQueue queue(base::ThreadStrategyForRendering::MULTI_THREADS,
                                MockRunnerManufactor::GetHookUITaskRunner());
  ThreadModeManager manager{ui_runner, engine_runner, &queue};

  engine_runner->PostEmergencyTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
    queue.Flush();
  });

  ui_runner->PostTask([&manager, &result, &queue]() {
    ThreadModeAutoSwitch auto_switch(manager);
    ThreadModeAutoSwitch duplicated_auto_switch(manager);

    ASSERT_EQ(result, kExpect);
    result = 0;

    // now work as PART_ON_LAYOUT/ALL_ON_UI
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&arwe]() { arwe.Signal(); });

  arwe.Wait();
  arwe.Reset();
  ASSERT_EQ(result, kExpect);

  result = 0;

  engine_runner->PostTask([&queue, &result]() {
    for (int32_t i = 0; i < kExpect; ++i) {
      queue.EnqueueUIOperation([&result] { ++result; });
    }
  });

  ui_runner->PostTask([&queue, &arwe]() {
    queue.Flush();
    arwe.Signal();
  });

  engine_runner->PostTask([&queue]() {
    queue.UpdateStatus(shell::UIOperationStatus::TASM_FINISH);
    queue.Flush();
    queue.UpdateStatus(shell::UIOperationStatus::LAYOUT_FINISH);
    queue.Flush();
  });
  arwe.Wait();

  ASSERT_EQ(result, kExpect);
}

}  // namespace testing
}  // namespace shell
}  // namespace lynx
