// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/thread_merger.h"

#include <memory>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

class ThreadMergerTest : public ::testing::Test {
 protected:
  ThreadMergerTest() = default;
  ~ThreadMergerTest() override = default;

  void SetUp() override { UIThread::Init(); }
};

TEST_F(ThreadMergerTest, SameRunner) {
  TaskRunnerManufactor manufactor(ThreadStrategyForRendering::MULTI_THREADS,
                                  true, true);
  auto* runner = manufactor.GetTASMTaskRunner().get();
  fml::AutoResetWaitableEvent arwe;

  fml::MessageLoop* looper = nullptr;
  runner->PostTask([&arwe, &looper]() {
    looper = &(fml::MessageLoop::GetCurrent());
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  runner->PostTask([&arwe, runner, looper]() {
    auto merger = std::make_unique<ThreadMerger>(runner, runner);
    ASSERT_EQ(looper, &(fml::MessageLoop::GetCurrent()));

    merger = nullptr;
    ASSERT_EQ(looper, &(fml::MessageLoop::GetCurrent()));

    arwe.Signal();
  });

  arwe.Wait();
}

TEST_F(ThreadMergerTest, DifferentRunners) {
  TaskRunnerManufactor manufactor(ThreadStrategyForRendering::MULTI_THREADS,
                                  true, true);
  auto* owner_runner = manufactor.GetTASMTaskRunner().get();
  auto* subsumed_runner = manufactor.GetLayoutTaskRunner().get();

  fml::AutoResetWaitableEvent arwe;

  fml::MessageLoop* owner_looper = nullptr;
  fml::MessageLoop* subsumed_looper = nullptr;

  owner_runner->PostTask([&arwe, &owner_looper]() {
    owner_looper = &(fml::MessageLoop::GetCurrent());
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  subsumed_runner->PostTask([&arwe, &subsumed_looper]() {
    subsumed_looper = &(fml::MessageLoop::GetCurrent());
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  ThreadMerger* merger = nullptr;
  owner_runner->PostTask([&merger, &arwe, owner_runner, subsumed_runner]() {
    merger = new ThreadMerger(owner_runner, subsumed_runner);
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  subsumed_runner->PostTask([&arwe, owner_looper, subsumed_looper]() {
    ASSERT_EQ(owner_looper, &(fml::MessageLoop::GetCurrent()));
    ASSERT_NE(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  owner_runner->PostTask([&merger, &arwe]() {
    delete merger;
    merger = nullptr;
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  subsumed_runner->PostTask([&arwe, owner_looper, subsumed_looper]() {
    ASSERT_NE(owner_looper, &(fml::MessageLoop::GetCurrent()));
    ASSERT_EQ(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
    arwe.Signal();
  });

  arwe.Wait();
}

TEST_F(ThreadMergerTest, RvalueMove) {
  TaskRunnerManufactor manufactor(ThreadStrategyForRendering::MULTI_THREADS,
                                  true, true);
  auto* owner_runner = manufactor.GetTASMTaskRunner().get();
  auto* subsumed_runner = manufactor.GetLayoutTaskRunner().get();

  fml::AutoResetWaitableEvent arwe;

  fml::MessageLoop* owner_looper = nullptr;
  fml::MessageLoop* subsumed_looper = nullptr;

  owner_runner->PostTask([&arwe, &owner_looper]() {
    owner_looper = &(fml::MessageLoop::GetCurrent());
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  subsumed_runner->PostTask([&arwe, &subsumed_looper]() {
    subsumed_looper = &(fml::MessageLoop::GetCurrent());
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  ThreadMerger* merger = nullptr;
  owner_runner->PostTask([&merger, &arwe, owner_runner, subsumed_runner]() {
    merger = new ThreadMerger(owner_runner, subsumed_runner);
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  subsumed_runner->PostTask([&arwe, owner_looper, subsumed_looper]() {
    ASSERT_EQ(owner_looper, &(fml::MessageLoop::GetCurrent()));
    ASSERT_NE(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  owner_runner->PostTask([&merger, &arwe]() {
    ThreadMerger moved_merger(std::move(*merger));
    *merger = std::move(moved_merger);
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  owner_runner->PostTask([&merger, &arwe]() {
    delete merger;
    merger = nullptr;
    arwe.Signal();
  });

  arwe.Wait();
  arwe.Reset();

  subsumed_runner->PostTask([&arwe, owner_looper, subsumed_looper]() {
    ASSERT_NE(owner_looper, &(fml::MessageLoop::GetCurrent()));
    ASSERT_EQ(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
    arwe.Signal();
  });

  arwe.Wait();
}

}  // namespace base
}  // namespace lynx
