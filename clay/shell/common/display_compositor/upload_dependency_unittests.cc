// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display_compositor/test_support.h"
#include "clay/shell/common/display_compositor/upload_dependency.h"
#include "gtest/gtest.h"

namespace clay::display {
using testing_support::Signal;

TEST(UploadDependency, ContinuousLayoutsCanProduceBeforeUploadsSignalOnCpu) {
  UploadDependency uploads;
  int waits = 0;
  auto pending = std::make_shared<Signal>();
  pending->status = CompletionStatus::kPending;
  // Model a new tree at every VSync: the newly inserted upload fence is
  // invariably pending at the CPU query, although the GPU makes progress.
  for (uint64_t layout = 1; layout <= 120; ++layout) {
    EXPECT_TRUE(uploads.Prepare(
        layout, [&] { return pending; },
        [&](const auto& completion) {
          EXPECT_EQ(completion, pending);
          ++waits;
          return true;
        }));
    // Queuing a dependency must not make an unfinished output displayable.
    EXPECT_EQ(pending->Poll(), CompletionStatus::kPending);
  }
  EXPECT_EQ(waits, 120);
}

TEST(UploadDependency, RetryQueuesOnlyOneWaitUntilUploadsAreInvalidated) {
  UploadDependency uploads;
  int creates = 0, waits = 0;
  auto pending = std::make_shared<Signal>();
  pending->status = CompletionStatus::kPending;
  auto create = [&] {
    ++creates;
    return pending;
  };
  auto enqueue = [&](const auto&) {
    ++waits;
    return true;
  };
  EXPECT_TRUE(uploads.Prepare(1, create, enqueue));
  EXPECT_TRUE(uploads.Prepare(1, create, enqueue));
  EXPECT_EQ(creates, 1);
  EXPECT_EQ(waits, 1);
  uploads.Invalidate();
  EXPECT_TRUE(uploads.Prepare(1, create, enqueue));
  EXPECT_EQ(creates, 2);
  EXPECT_EQ(waits, 2);
}

TEST(UploadDependency, UnsupportedGpuWaitDefersUntilFenceCompletes) {
  UploadDependency uploads;
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  int creates = 0, waits = 0;
  auto create = [&] {
    ++creates;
    return signal;
  };
  auto enqueue = [&](const auto&) {
    ++waits;
    return false;
  };
  EXPECT_FALSE(uploads.Prepare(1, create, enqueue));
  signal->status = CompletionStatus::kComplete;
  EXPECT_TRUE(uploads.Prepare(1, create, enqueue));
  EXPECT_EQ(creates, 1);
  EXPECT_EQ(waits, 1);
}

TEST(UploadDependency, FailedFenceDoesNotAuthorizeProductionAfterQueuedWait) {
  UploadDependency uploads;
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  int waits = 0;
  auto enqueue = [&](const auto&) {
    ++waits;
    return true;
  };
  EXPECT_TRUE(uploads.Prepare(
      1, [&] { return signal; }, enqueue));
  signal->status = CompletionStatus::kFailed;
  EXPECT_FALSE(uploads.Prepare(
      1, [&] { return signal; }, enqueue));
  EXPECT_EQ(waits, 1);
}

TEST(UploadDependency, MissingFenceNeverQueuesAWait) {
  UploadDependency uploads;
  EXPECT_FALSE(uploads.Prepare(
      1, [] { return std::shared_ptr<GPUCompletion>(); },
      [](const auto&) {
        ADD_FAILURE() << "Cannot queue a dependency without a fence";
        return true;
      }));
}
}  // namespace clay::display
