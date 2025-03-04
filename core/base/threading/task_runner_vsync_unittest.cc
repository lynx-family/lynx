// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/task_runner_vsync.h"

#include <memory>
#include <thread>

#include "base/include/fml/memory/ref_ptr.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/platform/linux/message_loop_linux.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {
namespace threading {

class TaskRunnerVSyncTest : public ::testing::Test {
 protected:
  TaskRunnerVSyncTest() = default;
  ~TaskRunnerVSyncTest() override = default;

  static void SetUpTestSuite() { base::UIThread::Init(); }
};

TEST_F(TaskRunnerVSyncTest, RunsTasksOnCurrentThread) {
  const auto original_ui_runner = base::UIThread::GetRunner(false);
  const auto original_vsync_runner = base::UIThread::GetRunner(true);

  const auto ui_thread = std::make_unique<fml::Thread>("MOCK_UI_THREAD");
  const auto bg_thread = std::make_unique<fml::Thread>("BACKGROUND_THREAD");

  // Although on real enviroment the vsync loop thread is actually the platform
  // ui thread, the unit test is running on Linux, and it seems that the
  // MessageLoopLinux does not support multiple loops on one thread like
  // MessageLoopAndroid and MessageLoopDarwin do. So we mock vsync thread here
  // to make sure this unit test can run. The focus of this unit test is to
  // verify whether TaskRunnerVSync work correctly when its bound loop changes.
  const auto vsync_thread = std::make_unique<fml::Thread>("MOCK_VSYNC_THREAD");

  base::UIThread::GetRunner(false) =
      fml::MakeRefCounted<fml::TaskRunner>(ui_thread->GetLoop());
  base::UIThread::GetRunner(true) =
      fml::MakeRefCounted<base::TaskRunnerVSync>(vsync_thread->GetLoop());

  const auto engine_runner =
      fml::MakeRefCounted<base::TaskRunnerVSync>(vsync_thread->GetLoop());

  ui_thread->GetTaskRunner()->PostSyncTask([&engine_runner]() {
    ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
  });

  bg_thread->GetTaskRunner()->PostSyncTask(
      [&engine_runner, &bg_loop = bg_thread->GetLoop()]() {
        ASSERT_FALSE(engine_runner->RunsTasksOnCurrentThread());
        engine_runner->Bind(bg_loop);
        ASSERT_TRUE(engine_runner->RunsTasksOnCurrentThread());
      });

  ui_thread->GetTaskRunner()->PostSyncTask([&engine_runner]() {
    ASSERT_FALSE(engine_runner->RunsTasksOnCurrentThread());
  });

  vsync_thread->GetTaskRunner()->PostSyncTask([&engine_runner]() {
    ASSERT_FALSE(engine_runner->RunsTasksOnCurrentThread());
  });

  base::UIThread::GetRunner(false) = original_ui_runner;
  base::UIThread::GetRunner(true) = original_vsync_runner;
}
}  // namespace threading
}  // namespace base
}  // namespace lynx
