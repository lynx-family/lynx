// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define FML_USED_ON_EMBEDDER

#include "base/include/fml/memory/task_runner_checker.h"

#include <thread>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/raster_thread_merger.h"
#include "base/include/fml/synchronization/count_down_latch.h"
#include "base/include/fml/synchronization/waitable_event.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace fml {
namespace testing {

TEST(TaskRunnerCheckerTests, RunsOnCurrentTaskRunner) {
  TaskRunnerChecker checker;
  EXPECT_EQ(checker.RunsOnCreationTaskRunner(), true);
}

TEST(TaskRunnerCheckerTests, FailsTheCheckIfOnDifferentTaskRunner) {
  TaskRunnerChecker checker;
  EXPECT_EQ(checker.RunsOnCreationTaskRunner(), true);
  fml::MessageLoop* loop = nullptr;
  fml::AutoResetWaitableEvent latch;
  std::thread anotherThread([&]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop = &fml::MessageLoop::GetCurrent();
    loop->GetTaskRunner()->PostTask([&]() {
      EXPECT_EQ(checker.RunsOnCreationTaskRunner(), false);
      latch.Signal();
    });
    loop->Run();
  });
  latch.Wait();
  loop->Terminate();
  anotherThread.join();
  EXPECT_EQ(checker.RunsOnCreationTaskRunner(), true);
}

TEST(TaskRunnerCheckerTests, SameTaskRunnerRunsOnTheSameThread) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  fml::MessageLoop& loop1 = fml::MessageLoop::GetCurrent();
  fml::MessageLoop& loop2 = fml::MessageLoop::GetCurrent();
  TaskQueueId a = loop1.GetTaskRunner()->GetTaskQueueId();
  TaskQueueId b = loop2.GetTaskRunner()->GetTaskQueueId();
  EXPECT_EQ(TaskRunnerChecker::RunsOnTheSameThread(a, b), true);
}

TEST(TaskRunnerCheckerTests, RunsOnDifferentThreadsReturnsFalse) {
  fml::MessageLoop::EnsureInitializedForCurrentThread();
  fml::MessageLoop& loop1 = fml::MessageLoop::GetCurrent();
  TaskQueueId a = loop1.GetTaskRunner()->GetTaskQueueId();
  fml::AutoResetWaitableEvent latch;
  std::thread anotherThread([&]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    fml::MessageLoop& loop2 = fml::MessageLoop::GetCurrent();
    TaskQueueId b = loop2.GetTaskRunner()->GetTaskQueueId();
    EXPECT_EQ(TaskRunnerChecker::RunsOnTheSameThread(a, b), false);
    latch.Signal();
  });
  latch.Wait();
  anotherThread.join();
}

TEST(TaskRunnerCheckerTests, MergedTaskRunnersRunsOnTheSameThread) {
  fml::MessageLoop* loop1 = nullptr;
  fml::AutoResetWaitableEvent latch1;
  fml::AutoResetWaitableEvent term1;
  std::thread thread1([&loop1, &latch1, &term1]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop1 = &fml::MessageLoop::GetCurrent();
    latch1.Signal();
    term1.Wait();
  });

  fml::MessageLoop* loop2 = nullptr;
  fml::AutoResetWaitableEvent latch2;
  fml::AutoResetWaitableEvent term2;
  std::thread thread2([&loop2, &latch2, &term2]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop2 = &fml::MessageLoop::GetCurrent();
    latch2.Signal();
    term2.Wait();
  });

  latch1.Wait();
  latch2.Wait();
  fml::TaskQueueId qid1 = loop1->GetTaskRunner()->GetTaskQueueId();
  fml::TaskQueueId qid2 = loop2->GetTaskRunner()->GetTaskQueueId();
  const auto raster_thread_merger_ =
      fml::MakeRefCounted<fml::RasterThreadMerger>(qid1, qid2);
  const size_t kNumFramesMerged = 5;

  raster_thread_merger_->MergeWithLease(kNumFramesMerged);

  // merged, running on the same thread
  EXPECT_EQ(TaskRunnerChecker::RunsOnTheSameThread(qid1, qid2), true);

  for (size_t i = 0; i < kNumFramesMerged; i++) {
    ASSERT_TRUE(raster_thread_merger_->IsMerged());
    raster_thread_merger_->DecrementLease();
  }

  ASSERT_FALSE(raster_thread_merger_->IsMerged());

  // un-merged, not running on the same thread
  EXPECT_EQ(TaskRunnerChecker::RunsOnTheSameThread(qid1, qid2), false);

  term1.Signal();
  term2.Signal();
  thread1.join();
  thread2.join();
}

TEST(TaskRunnerCheckerTests,
     PassesRunsOnCreationTaskRunnerIfOnDifferentTaskRunner) {
  fml::MessageLoop* loop1 = nullptr;
  fml::AutoResetWaitableEvent latch1;
  std::thread thread1([&]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop1 = &fml::MessageLoop::GetCurrent();
    latch1.Signal();
    loop1->Run();
  });

  fml::MessageLoop* loop2 = nullptr;
  fml::AutoResetWaitableEvent latch2;
  std::thread thread2([&]() {
    fml::MessageLoop::EnsureInitializedForCurrentThread();
    loop2 = &fml::MessageLoop::GetCurrent();
    latch2.Signal();
    loop2->Run();
  });

  latch1.Wait();
  latch2.Wait();

  fml::TaskQueueId qid1 = loop1->GetTaskRunner()->GetTaskQueueId();
  fml::TaskQueueId qid2 = loop2->GetTaskRunner()->GetTaskQueueId();
  fml::MessageLoopTaskQueues::GetInstance()->Merge(qid1, qid2);

  std::unique_ptr<TaskRunnerChecker> checker;

  fml::AutoResetWaitableEvent latch3;
  loop2->GetTaskRunner()->PostTask([&]() {
    checker = std::make_unique<TaskRunnerChecker>();
    EXPECT_EQ(checker->RunsOnCreationTaskRunner(), true);
    latch3.Signal();
  });
  latch3.Wait();

  fml::MessageLoopTaskQueues::GetInstance()->Unmerge(qid1, qid2);

  fml::AutoResetWaitableEvent latch4;
  loop2->GetTaskRunner()->PostTask([&]() {
    EXPECT_EQ(checker->RunsOnCreationTaskRunner(), true);
    latch4.Signal();
  });
  latch4.Wait();

  loop1->Terminate();
  loop2->Terminate();
  thread1.join();
  thread2.join();
}

}  // namespace testing
}  // namespace fml
}  // namespace lynx
