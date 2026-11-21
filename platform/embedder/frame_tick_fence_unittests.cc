// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/frame_tick_fence.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx::embedder {
namespace {

TEST(FrameTickFenceTest, CompletesOnlyAfterSealAndAllWork) {
  FrameTickFence fence;
  int completed = 0;
  fence.SetCompleteCallback([&](uint64_t id, int64_t start) {
    EXPECT_EQ(id, 1u);
    EXPECT_EQ(start, 100);
    ++completed;
  });
  EXPECT_EQ(fence.AcquireWork(100), nullptr);
  fence.Begin(1, 100);
  EXPECT_EQ(fence.AcquireWork(99), nullptr);
  auto first = fence.AcquireWork(100);
  auto second = fence.AcquireWork(100);
  ASSERT_NE(first, nullptr);
  ASSERT_NE(second, nullptr);
  first->Complete();
  first->Complete();
  EXPECT_EQ(completed, 0);
  fence.Seal(99);
  EXPECT_EQ(completed, 0);
  fence.Seal(100);
  EXPECT_EQ(fence.AcquireWork(100), nullptr);
  EXPECT_EQ(completed, 0);
  second.reset();
  EXPECT_EQ(completed, 1);
  fence.Seal(100);
  EXPECT_EQ(completed, 1);
}

TEST(FrameTickFenceTest, EmptyTickCompletesAtSealAndCallbackCanBeginNextTick) {
  FrameTickFence fence;
  int completed = 0;
  fence.SetCompleteCallback([&](uint64_t id, int64_t start) {
    ++completed;
    if (id == 1) {
      fence.Begin(2, start + 100);
    }
  });
  fence.Begin(1, 100);
  EXPECT_EQ(completed, 0);
  fence.Seal(100);
  EXPECT_EQ(completed, 1);
  fence.Seal(200);
  EXPECT_EQ(completed, 2);
}

}  // namespace
}  // namespace lynx::embedder
