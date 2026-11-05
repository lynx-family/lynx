// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "clay/flow/testing/mock_shared_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
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

namespace {

class TestBacking final : public SharedImageBacking {
 public:
  explicit TestBacking(skity::Vec2 size)
      : SharedImageBacking(PixelFormat::kNative8888, size) {}
  BackingType GetType() const override { return BackingType::kMocking; }
  GraphicsMemoryHandle GetGFXHandle() const override { return nullptr; }
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override {
    return nullptr;
  }
#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrContext*) override {
    return nullptr;
  }
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      GrContext*) override {
    return nullptr;
  }
#endif
};

auto MakeBacking(skity::Vec2 size,
                 std::optional<SharedImageSink::GraphicsMemoryHandle>) {
  return fml::MakeRefCounted<TestBacking>(size);
}

using Status = SharedImageSink::AcquireBackStatus;
const skity::Vec2 kSize(16, 16);

TEST(SharedImageSinkTest, ForcedAcquireDetachedWithAvailableBack) {
  SharedImageSinkManaged sink(SharedImageSink::kDoubleBuffer, MakeBacking);
  sink.SetFrameAvailableCallback([] {});
  Status status;
  auto [back, age] = sink.AcquireBackForced(kSize, {}, &status);
  ASSERT_NE(back, nullptr);
  ASSERT_EQ(status, Status::kSuccess);
  sink.SetFrameAvailableCallback(nullptr);
  auto [detached, detached_age] = sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(detached, nullptr);
  EXPECT_EQ(status, Status::kDetached);

  sink.SetFrameAvailableCallback([] {});
  auto [reattached, reattached_age] =
      sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(reattached, back);
  EXPECT_EQ(status, Status::kSuccess);
}

TEST(SharedImageSinkTest, ForcedAcquireDetachedWithFullQueueAndReattach) {
  SharedImageSinkManaged sink(SharedImageSink::kDoubleBuffer, MakeBacking);
  sink.SetFrameAvailableCallback([] {});
  auto [current, age] = sink.AcquireBackForced(kSize);
  ASSERT_NE(current, nullptr);
  ASSERT_TRUE(sink.SwapBack(nullptr));
  ASSERT_EQ(sink.UpdateFront(nullptr), current);
  auto [pending, pending_age] = sink.AcquireBackForced(kSize);
  ASSERT_NE(pending, nullptr);
  ASSERT_NE(pending, current);
  ASSERT_TRUE(sink.SwapBack(nullptr));

  sink.SetFrameAvailableCallback(nullptr);
  Status status;
  auto [detached, detached_age] = sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(detached, nullptr);
  EXPECT_EQ(status, Status::kDetached);
  sink.SetFrameAvailableCallback([] {});
  auto [recycled, recycled_age] = sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(recycled, pending);
  EXPECT_NE(recycled, current);
  EXPECT_EQ(status, Status::kSuccess);
  ASSERT_TRUE(sink.SwapBack(nullptr));
  EXPECT_EQ(sink.UpdateFrontToLatest(nullptr), pending);
}

TEST(SharedImageSinkTest, ForcedAcquireNeverStealsCurrentFront) {
  SharedImageSinkManaged sink(SharedImageSink::kSingleBuffer, MakeBacking);
  sink.SetFrameAvailableCallback([] {});
  auto [back, age] = sink.AcquireBack(kSize);
  ASSERT_NE(back, nullptr);
  ASSERT_TRUE(sink.SwapBack(nullptr));
  ASSERT_EQ(sink.UpdateFront(nullptr), back);
  Status status;
  auto [unavailable, unavailable_age] =
      sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(unavailable, nullptr);
  EXPECT_EQ(status, Status::kFullAttach);
  sink.ReleaseFront(nullptr);
  auto [released, released_age] = sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(released, back);
  EXPECT_EQ(status, Status::kSuccess);
}

TEST(SharedImageSinkTest, ForcedAcquireAllocationFailureIsNotDetached) {
  bool fail_allocation = true;
  SharedImageSinkManaged sink(
      SharedImageSink::kDoubleBuffer,
      [&](skity::Vec2 size, auto handle) -> fml::RefPtr<SharedImageBacking> {
        return fail_allocation ? nullptr : MakeBacking(size, handle);
      });
  sink.SetFrameAvailableCallback([] {});
  Status status = Status::kDetached;
  auto [failed, age] = sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_EQ(failed, nullptr);
  EXPECT_EQ(status, Status::kUnkown);
  fail_allocation = false;
  auto [retried, retried_age] = sink.AcquireBackForced(kSize, {}, &status);
  EXPECT_NE(retried, nullptr);
  EXPECT_EQ(status, Status::kSuccess);
}

}  // namespace
}  // namespace clay
