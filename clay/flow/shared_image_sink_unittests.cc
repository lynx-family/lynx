// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "clay/flow/testing/mock_shared_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
namespace {

class SharedImageSinkDamageTest
    : public ::testing::TestWithParam<SharedImageSink::BufferMode> {
 protected:
  void SetUp() override {
    sink_ = fml::MakeRefCounted<SharedImageSinkManaged>(
        GetParam(), [this](skity::Vec2 size, auto) {
          ++allocation_count_;
          return fml::MakeRefCounted<MockSharedImageBacking>(size);
        });
    sink_->SetFrameAvailableCallback([] {});
  }

  void FillQueue() {
    for (uint32_t i = 0; i < sink_->Capacity(); ++i) {
      auto [backing, age] = sink_->AcquireBackForced(size_);
      ASSERT_TRUE(backing);
      EXPECT_EQ(age, 0u);
      backings_.push_back(backing);
      ASSERT_TRUE(sink_->SwapBack(nullptr));
    }
  }

  const skity::Vec2 size_{100, 80};
  fml::RefPtr<SharedImageSinkManaged> sink_;
  std::vector<fml::RefPtr<SharedImageBacking>> backings_;
  uint32_t allocation_count_ = 0;
};

TEST_P(SharedImageSinkDamageTest, ReturnedBufferRetainsAllMissedFramesInAge) {
  ASSERT_NO_FATAL_FAILURE(FillQueue());
  auto front = sink_->UpdateFront(nullptr);
  ASSERT_EQ(front, backings_.front());
  front = sink_->UpdateFront(nullptr);
  ASSERT_EQ(front, backings_[1]);

  auto [backing, age] = sink_->AcquireBackForced(size_);
  EXPECT_EQ(backing, backings_.front());
  EXPECT_EQ(age, sink_->Capacity());
  EXPECT_EQ(allocation_count_, sink_->Capacity());
  ASSERT_TRUE(sink_->SwapBack(nullptr));
}

TEST_P(SharedImageSinkDamageTest, PendingReuseKeepsCurrentFrontAndBufferAge) {
  ASSERT_NO_FATAL_FAILURE(FillQueue());
  auto front = sink_->UpdateFront(nullptr);
  ASSERT_EQ(front, backings_.front());
  auto pending = backings_.back();

  // A stalled consumer retains its current image. The producer can replace
  // the latest pending frame repeatedly without losing its buffer history.
  for (int i = 0; i < 32; ++i) {
    auto [backing, age] = sink_->AcquireBackForced(size_);
    ASSERT_EQ(backing, pending);
    EXPECT_NE(backing, front);
    EXPECT_EQ(age, 1u);
    ASSERT_TRUE(sink_->SwapBack(nullptr));
  }
  EXPECT_EQ(allocation_count_, sink_->Capacity());
  EXPECT_EQ(sink_->UpdateFrontToLatest(nullptr), pending);

  // Reading again frees the original current front, whose age includes every
  // submitted replacement even though intermediate frames were not consumed.
  auto [backing, age] = sink_->AcquireBackForced(size_);
  EXPECT_EQ(backing, front);
  EXPECT_EQ(age, sink_->Capacity() + 32);
  ASSERT_TRUE(sink_->SwapBack(nullptr));
}

TEST_P(SharedImageSinkDamageTest, ResizedPendingBufferHasUnknownContents) {
  ASSERT_NO_FATAL_FAILURE(FillQueue());
  auto front = sink_->UpdateFront(nullptr);
  ASSERT_EQ(front, backings_.front());

  auto [backing, age] = sink_->AcquireBackForced({120, 90});
  ASSERT_TRUE(backing);
  EXPECT_NE(backing, front);
  EXPECT_NE(backing, backings_.back());
  EXPECT_EQ(backing->GetSize(), skity::Vec2(120, 90));
  EXPECT_EQ(age, 0u);
  ASSERT_TRUE(sink_->SwapBack(nullptr));
}

INSTANTIATE_TEST_SUITE_P(DoubleAndTripleBuffer, SharedImageSinkDamageTest,
                         ::testing::Values(SharedImageSink::kDoubleBuffer,
                                           SharedImageSink::kTripleBuffer));

}  // namespace
}  // namespace testing
}  // namespace clay
