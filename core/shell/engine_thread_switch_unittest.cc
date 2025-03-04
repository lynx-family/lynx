// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/engine_thread_switch.h"

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
#include "core/shell/dynamic_ui_operation_queue.h"
#include "core/shell/testing/mock_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace shell {
namespace testing {

class EngineThreadSwitchTest : public ::testing::Test {
 protected:
  EngineThreadSwitchTest() = default;
  ~EngineThreadSwitchTest() override = default;

  static constexpr int32_t kExpect = 10;

  static void SetUpTestSuite() { base::UIThread::Init(); }
};

TEST_F(EngineThreadSwitchTest, AttachAndDetachEngine) {
  fml::AutoResetWaitableEvent arwe;

  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  auto engine_runner = MockRunnerManufactor::GetHookTASMTaskRunner();
  auto queue = std::make_shared<DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::MULTI_THREADS,
      MockRunnerManufactor::GetHookUITaskRunner());

  auto engine_thread_switch =
      std::make_shared<EngineThreadSwitch>(ui_runner, engine_runner, queue);

  ui_runner->PostTask([&engine_runner, &arwe]() {
    ASSERT_FALSE(engine_runner->RunsTasksOnCurrentThread());
    arwe.Signal();
  });
  arwe.Wait();

  ui_runner->PostTask([engine_thread_switch, &engine_runner, &arwe]() {
    engine_thread_switch->AttachEngineToUIThread();
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
    arwe.Signal();
  });
  arwe.Wait();

  ui_runner->PostTask(
      [engine_thread_switch, &engine_runner, &arwe, &ui_runner]() {
        engine_thread_switch->DetachEngineFromUIThread();
        ASSERT_FALSE(engine_runner->RunsTasksOnCurrentThread());
        engine_runner->PostTask([&arwe, &ui_runner]() {
          ASSERT_FALSE(ui_runner->RunsTasksOnCurrentThread());
          arwe.Signal();
        });
      });
  arwe.Wait();
}

TEST_F(EngineThreadSwitchTest, AttachAndDetachWhenEngineLoopSameWithUILoop) {
  fml::AutoResetWaitableEvent arwe;

  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  auto engine_runner =
      fml::MakeRefCounted<fml::TaskRunner>(ui_runner->GetLoop());
  auto queue = std::make_shared<DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::ALL_ON_UI,
      MockRunnerManufactor::GetHookUITaskRunner());

  auto engine_thread_switch =
      std::make_shared<EngineThreadSwitch>(ui_runner, engine_runner, queue);

  ui_runner->PostTask([&engine_runner, &arwe]() {
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
    arwe.Signal();
  });
  arwe.Wait();

  ui_runner->PostTask([engine_thread_switch, &engine_runner, &arwe]() {
    engine_thread_switch->AttachEngineToUIThread();
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
    arwe.Signal();
  });
  arwe.Wait();

  ui_runner->PostTask([engine_thread_switch, &engine_runner, &arwe]() {
    engine_thread_switch->DetachEngineFromUIThread();
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
    arwe.Signal();
  });
  arwe.Wait();
}

TEST_F(EngineThreadSwitchTest, SetEngineLoopAfterCreated) {
  auto ui_runner = MockRunnerManufactor::GetHookUITaskRunner();
  auto engine_runner =
      fml::MakeRefCounted<fml::TaskRunner>(ui_runner->GetLoop());
  auto queue = std::make_shared<DynamicUIOperationQueue>(
      base::ThreadStrategyForRendering::ALL_ON_UI,
      MockRunnerManufactor::GetHookUITaskRunner());
  auto engine_thread_switch =
      std::make_shared<EngineThreadSwitch>(ui_runner, engine_runner, queue);

  ui_runner->PostSyncTask([&engine_runner, &engine_thread_switch]() {
    engine_thread_switch->DetachEngineFromUIThread();
    // Did not set engine loop, DetachEngineFromUIThread will do nothing.
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
  });

  ui_runner->PostSyncTask([&engine_runner, &engine_thread_switch]() {
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
    engine_thread_switch->SetEngineLoop(
        MockRunnerManufactor::GetHookTASMTaskRunner()->GetLoop());
    engine_thread_switch->DetachEngineFromUIThread();
    ASSERT_FALSE(engine_runner->RunsTasksOnCurrentThread());
  });
}
}  // namespace testing
}  // namespace shell
}  // namespace lynx
