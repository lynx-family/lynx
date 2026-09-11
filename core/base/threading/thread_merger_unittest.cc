// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/threading/thread_merger.h"

#include <memory>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/task_runner.h"
#include "base/include/fml/thread.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace base {

TEST(ThreadMergerTest, SameRunner) {
  fml::Thread thread("thread_merger");
  auto* runner = thread.GetTaskRunner().get();
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
    EXPECT_EQ(looper, &(fml::MessageLoop::GetCurrent()));

    merger = nullptr;
    EXPECT_EQ(looper, &(fml::MessageLoop::GetCurrent()));

    arwe.Signal();
  });

  arwe.Wait();
}

TEST(ThreadMergerTest, DifferentRunners) {
  fml::Thread owner_thread("thread_merger_owner");
  fml::Thread subsumed_thread("thread_merger_subsumed");
  auto* owner_runner = owner_thread.GetTaskRunner().get();
  auto* subsumed_runner = subsumed_thread.GetTaskRunner().get();

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
    EXPECT_EQ(owner_looper, &(fml::MessageLoop::GetCurrent()));
    EXPECT_NE(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
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
    EXPECT_NE(owner_looper, &(fml::MessageLoop::GetCurrent()));
    EXPECT_EQ(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
    arwe.Signal();
  });

  arwe.Wait();
}

TEST(ThreadMergerTest, RvalueMove) {
  fml::Thread owner_thread("thread_merger_owner");
  fml::Thread subsumed_thread("thread_merger_subsumed");
  auto* owner_runner = owner_thread.GetTaskRunner().get();
  auto* subsumed_runner = subsumed_thread.GetTaskRunner().get();

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
    EXPECT_EQ(owner_looper, &(fml::MessageLoop::GetCurrent()));
    EXPECT_NE(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
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
    EXPECT_NE(owner_looper, &(fml::MessageLoop::GetCurrent()));
    EXPECT_EQ(subsumed_looper, &(fml::MessageLoop::GetCurrent()));
    arwe.Signal();
  });

  arwe.Wait();
}

}  // namespace base
}  // namespace lynx
