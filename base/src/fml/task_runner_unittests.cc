// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
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

TEST_F(TaskRunnerTest, Bind) {
  fml::Thread original_thread("original_thread");
  fml::Thread target_thread("target_thread");

  auto original_loop = original_thread.GetLoop();
  auto target_loop = target_thread.GetLoop();

  auto task_runner = fml::MakeRefCounted<fml::TaskRunner>(original_loop);
  int32_t result = 0;
  constexpr int32_t expected = 10;

  for (int32_t i = 0; i < expected; ++i) {
    task_runner->PostTask([&result, original_loop, target_loop]() {
      ++result;
      ASSERT_EQ(MessageLoop::GetCurrent().GetLoopImpl().get(),
                original_loop.get());
      ASSERT_NE(MessageLoop::GetCurrent().GetLoopImpl().get(),
                target_loop.get());
    });
  }

  fml::AutoResetWaitableEvent arwe;
  original_thread.GetTaskRunner()->PostTask([&arwe]() { arwe.Signal(); });

  arwe.Wait();
  ASSERT_EQ(result, expected);

  arwe.Reset();
  target_thread.GetTaskRunner()->PostTask([&arwe, task_runner, target_loop]() {
    task_runner->Bind(target_loop);
    arwe.Signal();
  });

  arwe.Wait();
  result = 0;

  for (int32_t i = 0; i < expected; ++i) {
    task_runner->PostTask([&result, original_loop, target_loop]() {
      ++result;
      ASSERT_EQ(MessageLoop::GetCurrent().GetLoopImpl().get(),
                target_loop.get());
      ASSERT_NE(MessageLoop::GetCurrent().GetLoopImpl().get(),
                original_loop.get());
    });
  }

  arwe.Reset();
  target_thread.GetTaskRunner()->PostTask([&arwe]() { arwe.Signal(); });

  arwe.Wait();
  ASSERT_EQ(result, expected);
}

TEST_F(TaskRunnerTest, BindWithShouldRunExpiredTasksImmediately) {
  fml::Thread original_thread("original_thread");
  fml::Thread target_thread("target_thread");

  auto original_loop = original_thread.GetLoop();
  auto target_loop = target_thread.GetLoop();

  auto task_runner = fml::MakeRefCounted<fml::TaskRunner>(original_loop);
  int32_t result = 0;
  static constexpr int32_t expected = 10;

  fml::AutoResetWaitableEvent arwe;
  original_thread.GetTaskRunner()->PostTask([&arwe, &result, &target_thread,
                                             task_runner, original_loop,
                                             target_loop]() {
    for (int32_t i = 0; i < expected; ++i) {
      task_runner->PostTask([&result, original_loop, target_loop]() {
        ++result;
        ASSERT_EQ(MessageLoop::GetCurrent().GetLoopImpl().get(),
                  target_loop.get());
        ASSERT_NE(MessageLoop::GetCurrent().GetLoopImpl().get(),
                  original_loop.get());
      });
    }

    task_runner->UnBind();

    target_thread.GetTaskRunner()->PostTask(
        [&arwe, &result, task_runner, target_loop]() {
          task_runner->Bind(target_loop);
          ASSERT_EQ(result, 0);
          arwe.Signal();
        });
  });

  arwe.Wait();
  arwe.Reset();

  task_runner->PostTask([&arwe]() { arwe.Signal(); });
  arwe.Wait();

  ASSERT_EQ(result, expected);

  original_thread.GetTaskRunner()->PostTask(
      [task_runner, original_loop]() { task_runner->Bind(original_loop); });

  result = 0;
  original_thread.GetTaskRunner()->PostTask([&arwe, &result, &target_thread,
                                             task_runner, original_loop,
                                             target_loop]() {
    for (int32_t i = 0; i < expected; ++i) {
      task_runner->PostTask([&result, original_loop, target_loop]() {
        ++result;
        ASSERT_EQ(MessageLoop::GetCurrent().GetLoopImpl().get(),
                  target_loop.get());
        ASSERT_NE(MessageLoop::GetCurrent().GetLoopImpl().get(),
                  original_loop.get());
      });
    }

    task_runner->UnBind();

    target_thread.GetTaskRunner()->PostTask(
        [&arwe, &result, task_runner, target_loop]() {
          task_runner->Bind(target_loop, true);
          ASSERT_EQ(result, expected);
          arwe.Signal();
        });
  });

  arwe.Wait();
  arwe.Reset();

  task_runner->PostTask([&arwe]() { arwe.Signal(); });
  arwe.Wait();

  ASSERT_EQ(result, expected);
}

}  // namespace testing
}  // namespace fml
}  // namespace lynx
