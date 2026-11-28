// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <vector>

#include "clay/flow/testing/mock_shared_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/shared_image/shared_image_sink_accessor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {
namespace {

class SharedImageSinkDamageTest
    : public ::testing::TestWithParam<SharedImageSink::BufferMode> {
 protected:
  void SetUp() override {
    sink_ = fml::MakeRefCounted<SharedImageSinkManaged>(
        GetParam(),
        [this](skity::Vec2 size, auto) -> fml::RefPtr<SharedImageBacking> {
          ++allocation_count_;
          if (reject_allocations_) return nullptr;
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
  bool reject_allocations_ = false;
};

TEST_P(SharedImageSinkDamageTest, AllocationFailureCanBeRetried) {
  reject_allocations_ = true;
  EXPECT_FALSE(std::get<0>(sink_->AcquireBack(size_)));
  EXPECT_FALSE(std::get<0>(sink_->TryAcquireBack(size_)));
  EXPECT_FALSE(std::get<0>(sink_->AcquireBackForced(size_)));
  reject_allocations_ = false;
  ASSERT_TRUE(std::get<0>(sink_->AcquireBack(size_)));
  const skity::Vec2 resized{120, 100};
  reject_allocations_ = true;
  EXPECT_FALSE(std::get<0>(sink_->AcquireBack(resized)));
  EXPECT_FALSE(std::get<0>(sink_->TryAcquireBack(resized)));
  EXPECT_FALSE(std::get<0>(sink_->AcquireBackForced(resized)));
  reject_allocations_ = false;
  auto [backing, age] = sink_->AcquireBack(resized);
  ASSERT_TRUE(backing);
  EXPECT_EQ(backing->GetSize(), resized);
  EXPECT_EQ(age, 0u);
}

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

TEST_P(SharedImageSinkDamageTest, ReconfigurationRejectsAnAcquiredBack) {
  auto [backing, age] = sink_->AcquireBack(size_);
  ASSERT_TRUE(backing);
  EXPECT_FALSE(sink_->UpdateSharedImageFactory(
      [](auto, auto) { return fml::RefPtr<SharedImageBacking>(); }));
  ASSERT_TRUE(sink_->SwapBack(nullptr));
  EXPECT_EQ(sink_->UpdateFront(nullptr), backing);
}

TEST_P(SharedImageSinkDamageTest, EveryAcquirePathReplacesOldGenerations) {
  auto [previous, initial_age] = sink_->AcquireBack(size_);
  ASSERT_TRUE(previous);
  ASSERT_TRUE(sink_->SwapBack(nullptr));
  ASSERT_EQ(sink_->UpdateFront(nullptr), previous);
  sink_->ReleaseFront(nullptr);
  for (int path = 0; path < 3; ++path) {
    ASSERT_TRUE(sink_->UpdateSharedImageFactory(
        [this](skity::Vec2 size, auto) -> fml::RefPtr<SharedImageBacking> {
          if (reject_allocations_) return nullptr;
          return fml::MakeRefCounted<MockSharedImageBacking>(size);
        }));
    reject_allocations_ = true;
    EXPECT_FALSE(std::get<0>(sink_->AcquireBack(size_)));
    EXPECT_FALSE(std::get<0>(sink_->TryAcquireBack(size_)));
    EXPECT_FALSE(std::get<0>(sink_->AcquireBackForced(size_)));
    reject_allocations_ = false;
    std::tuple<fml::RefPtr<SharedImageBacking>, uint32_t> acquired;
    if (path == 0) {
      acquired = sink_->AcquireBack(size_);
    } else if (path == 1) {
      auto [backing, age, status] = sink_->TryAcquireBack(size_);
      acquired = {backing, age};
      EXPECT_EQ(status, SharedImageSink::AcquireBackStatus::kSuccess);
    } else {
      acquired = sink_->AcquireBackForced(size_);
    }
    auto [backing, age] = acquired;
    ASSERT_TRUE(backing);
    EXPECT_NE(backing, previous);
    EXPECT_EQ(age, 0u);
    ASSERT_TRUE(sink_->SwapBack(nullptr));
    EXPECT_EQ(sink_->UpdateFront(nullptr), backing);
    sink_->ReleaseFront(nullptr);
    previous = backing;
  }
}

TEST_P(SharedImageSinkDamageTest, ReconfigurationKeepsQueuedAndCurrentFronts) {
  ASSERT_NO_FATAL_FAILURE(FillQueue());
  auto current = sink_->UpdateFront(nullptr);
  ASSERT_EQ(current, backings_.front());
  ASSERT_TRUE(sink_->UpdateSharedImageFactory([](skity::Vec2 size, auto) {
    return fml::MakeRefCounted<MockSharedImageBacking>(size);
  }));
  auto [next, age] = sink_->AcquireBackForced(size_);
  ASSERT_TRUE(next);
  EXPECT_NE(next, current);
  EXPECT_EQ(age, 0u);
  ASSERT_TRUE(sink_->SwapBack(nullptr));
  EXPECT_EQ(sink_->UpdateFrontToLatest(nullptr), next);
  EXPECT_EQ(current, backings_.front());
}

TEST_P(SharedImageSinkDamageTest, PendingGLReuseRetainsFenceUntilReady) {
  struct ReleaseFence final : FenceSync {
    explicit ReleaseFence(PollResult& state) : completion(state) {}
    Type GetType() const override { return Type::kClientWaitOnly; }
    bool ClientWait() override {
      ADD_FAILURE() << "Pending release must be retried";
      return false;
    }
    PollResult Poll() override { return completion; }
    PollResult& completion;
  };
  auto [back, age] = sink_->AcquireBackForced(size_);
  ASSERT_TRUE(back);
  ASSERT_TRUE(sink_->SwapBack(nullptr));
  ASSERT_EQ(sink_->UpdateFront(nullptr), back);
  auto completion = FenceSync::PollResult::kPending;
  sink_->ReleaseFront(std::make_unique<ReleaseFence>(completion));

  SharedImageSinkAccessor writer(sink_, [](auto backing) {
    return backing->CreateRepresentation(nullptr);
  });
  SharedImageSink::AcquireBackStatus status;
  EXPECT_FALSE(std::get<0>(writer.AcquireBackForcedForGL(size_, status)));
  EXPECT_EQ(status, SharedImageSink::AcquireBackStatus::kFencePending);
  EXPECT_FALSE(writer.SwapBack());
  EXPECT_FALSE(std::get<0>(writer.AcquireBackForcedForGL(size_, status)));
  EXPECT_EQ(status, SharedImageSink::AcquireBackStatus::kFencePending);
  completion = FenceSync::PollResult::kSignaled;
  auto [repr, restored_age] = writer.AcquireBackForcedForGL(size_, status);
  ASSERT_TRUE(repr);
  EXPECT_EQ(repr->GetBacking(), back.get());
  EXPECT_EQ(status, SharedImageSink::AcquireBackStatus::kSuccess);
}

class ReconfigurationFence final : public FenceSync {
 public:
  ReconfigurationFence(bool& ready, int& waits,
                       const fml::RefPtr<SharedImageSinkManaged>& sink)
      : ready_(ready), waits_(waits), sink_(sink.get()) {}
  bool ClientWait() override {
    ++waits_;
    // This locks the sink again: retirement must wait outside its mutex, with
    // the producer slot reserved so another configuration cannot intervene.
    EXPECT_FALSE(sink_->UpdateSharedImageFactory([](skity::Vec2 size, auto) {
      return fml::MakeRefCounted<MockSharedImageBacking>(size);
    }));
    return ready_;
  }
  Type GetType() const override { return Type::kClientWaitOnly; }

 private:
  bool& ready_;
  int& waits_;
  SharedImageSinkManaged* sink_;
};

TEST_P(SharedImageSinkDamageTest, RetiresOldBuffersAfterTheirFenceAndCanRetry) {
  bool ready = false;
  int waits = 0;
  auto [old, initial_age] = sink_->AcquireBack(size_);
  ASSERT_TRUE(old);
  ASSERT_TRUE(sink_->SwapBack(nullptr));
  ASSERT_EQ(sink_->UpdateFront(nullptr), old);
  sink_->ReleaseFront(
      std::make_unique<ReconfigurationFence>(ready, waits, sink_));
  ASSERT_TRUE(sink_->UpdateSharedImageFactory([](skity::Vec2 size, auto) {
    return fml::MakeRefCounted<MockSharedImageBacking>(size);
  }));
  EXPECT_FALSE(std::get<0>(sink_->AcquireBack(size_)));
  EXPECT_EQ(waits, 1);
  ready = true;
  auto [backing, age] = sink_->AcquireBack(size_);
  ASSERT_TRUE(backing);
  EXPECT_NE(backing, old);
  EXPECT_EQ(age, 0u);
  EXPECT_EQ(waits, 2);
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

void CheckSkippedWriteFence(SharedImageSinkManaged& sink, bool preserve) {
  sink.SetFrameAvailableCallback([] {});
  auto current = std::get<0>(sink.AcquireBackForced(kSize));
  ASSERT_TRUE(sink.SwapBack(nullptr));
  ASSERT_EQ(sink.UpdateFront(nullptr), current);
  auto skipped = std::get<0>(sink.AcquireBackForced(kSize));
  auto write = std::make_unique<FailedFenceSync>();
  auto* expected_write = write.get();
  ASSERT_TRUE(sink.SwapBack(std::move(write)));
  auto latest = std::get<0>(sink.AcquireBackForced(kSize));
  ASSERT_TRUE(sink.SwapBack(nullptr));
  auto read = std::make_unique<FailedFenceSync>();
  auto* expected_read = read.get();
  EXPECT_EQ(sink.UpdateFrontToLatest(std::move(read)), latest);
  EXPECT_EQ(current->GetFenceSync().get(), expected_read);
  auto recycled_fence = skipped->GetFenceSync();
  if (preserve) {
    EXPECT_EQ(recycled_fence.get(), expected_write);
  } else {
    EXPECT_EQ(recycled_fence, nullptr);
  }
}

TEST(SharedImageSinkTest, SkippingUnreadFrontDiscardsItsWriteFenceByDefault) {
  SharedImageSinkManaged sink(SharedImageSink::kTripleBuffer, MakeBacking,
                              SharedImageSink::UpdateFrontMode::kLatest);
  CheckSkippedWriteFence(sink, false);
}

TEST(SharedImageSinkTest,
     SkippingUnreadFrontPreservesItsWriteFenceWhenEnabled) {
  SharedImageSinkManaged sink(
      SharedImageSink::kTripleBuffer, MakeBacking,
      SharedImageSink::UpdateFrontMode::kLatest,
      SharedImageSinkManaged::SkippedWriteFence::kPreserve);
  CheckSkippedWriteFence(sink, true);
}

class CanvasWriteFence final : public FenceSync {
 public:
  explicit CanvasWriteFence(PollResult& result) : result_(result) {}
  Type GetType() const override { return Type::kClientWaitOnly; }
  PollResult Poll() override { return result_; }
  bool ClientWait() override {
    EXPECT_EQ(result_, PollResult::kSignaled);
    return result_ == PollResult::kSignaled;
  }

 private:
  PollResult& result_;
};

auto MakeCanvasSink(
    SharedImageSink::BufferMode mode = SharedImageSink::kDoubleBuffer) {
  return fml::MakeRefCounted<SharedImageSinkManaged>(
      mode,
      [](auto size, auto) {
        return fml::MakeRefCounted<testing::MockSharedImageBacking>(size);
      },
      SharedImageSink::UpdateFrontMode::kLatestReady,
      SharedImageSinkManaged::SkippedWriteFence::kPreserve);
}

TEST(SharedImageSinkTest, SlowCanvasKeepsDisplayedFrameUntilRasterRetry) {
  auto sink = MakeCanvasSink();
  int notifications = 0;
  sink->SetFrameAvailableCallback([&] { ++notifications; });
  SharedImageSinkAccessor raster(sink, [](auto backing) {
    return backing->CreateRepresentation(nullptr);
  });
  bool retry = false;
  auto previous = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(nullptr));
  auto displayed = raster.UpdateFront(retry);
  ASSERT_TRUE(displayed);
  EXPECT_EQ(displayed->GetBacking(), previous.get());
  EXPECT_FALSE(retry);

  auto next = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_NE(next, previous);
  auto write = FenceSync::PollResult::kPending;
  ASSERT_TRUE(sink->SwapBack(std::make_unique<CanvasWriteFence>(write)));
  EXPECT_FALSE(raster.UpdateFront(retry));
  EXPECT_TRUE(retry);
  // Neither the displayed backing nor the deferred publication can be stolen.
  EXPECT_FALSE(std::get<0>(sink->AcquireBackForced(kSize)));
  write = FenceSync::PollResult::kSignaled;
  auto* mock = static_cast<testing::MockSharedImageBacking*>(previous.get());
  EXPECT_EQ(mock->ProducedFenceCount(), 0);
  auto ready = raster.UpdateFront(retry);
  ASSERT_TRUE(ready);
  EXPECT_EQ(mock->ProducedFenceCount(), 1);
  EXPECT_EQ(ready->GetBacking(), next.get());
  EXPECT_FALSE(retry);
  EXPECT_EQ(notifications, 2);
  EXPECT_EQ(std::get<0>(sink->AcquireBackForced(kSize)), previous);
}

TEST(SharedImageSinkTest, FirstCanvasPaintSelectsReadyFrameBehindPendingFrame) {
  auto sink = MakeCanvasSink(SharedImageSink::kTripleBuffer);
  sink->SetFrameAvailableCallback([] {});
  SharedImageSinkAccessor raster(
      sink, [](auto b) { return b->CreateRepresentation(nullptr); });
  auto first = std::get<0>(sink->AcquireBackForced(kSize));
  auto first_status = FenceSync::PollResult::kPending;
  ASSERT_TRUE(sink->SwapBack(std::make_unique<CanvasWriteFence>(first_status)));
  bool retry = false;
  EXPECT_FALSE(raster.UpdateFront(retry));
  EXPECT_TRUE(retry);
  auto second = std::get<0>(sink->AcquireBackForced(kSize));
  auto second_status = FenceSync::PollResult::kPending;
  ASSERT_TRUE(
      sink->SwapBack(std::make_unique<CanvasWriteFence>(second_status)));
  first_status = FenceSync::PollResult::kSignaled;
  auto ready = raster.UpdateFront(retry);
  ASSERT_TRUE(ready);
  EXPECT_EQ(ready->GetBacking(), first.get());
  EXPECT_TRUE(retry);
  second_status = FenceSync::PollResult::kFailed;
  EXPECT_FALSE(raster.UpdateFront(retry));
  EXPECT_FALSE(retry);
  auto fresh = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(fresh);
  EXPECT_NE(fresh, second);
  EXPECT_NE(fresh, first);
}

TEST(SharedImageSinkTest, FailedCanvasImportKeepsDisplayedBackingOwned) {
  auto sink = MakeCanvasSink();
  sink->SetFrameAvailableCallback([] {});
  bool reject_representation = false;
  SharedImageSinkAccessor raster(
      sink, [&](auto b) -> fml::RefPtr<SharedImageRepresentation> {
        return reject_representation ? nullptr
                                     : b->CreateRepresentation(nullptr);
      });
  auto displayed = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(nullptr));
  bool retry = false;
  ASSERT_TRUE(raster.UpdateFront(retry));
  auto* mock = static_cast<testing::MockSharedImageBacking*>(displayed.get());
  for (bool reject_factory : {true, false}) {
    auto candidate = std::get<0>(sink->AcquireBackForced(kSize));
    ASSERT_TRUE(candidate);
    ASSERT_NE(candidate, displayed);
    ASSERT_TRUE(sink->SwapBack(nullptr));
    reject_representation = reject_factory;
    bool called = false;
    EXPECT_FALSE(raster.UpdateFront(retry, [&](auto*) {
      called = true;
      // No queue mutex is held; both the old and candidate are reserved.
      EXPECT_FALSE(std::get<0>(sink->AcquireBackForced(kSize)));
      return false;
    }));
    EXPECT_EQ(called, !reject_factory);
    EXPECT_EQ(mock->ProducedFenceCount(), 0);
  }
  auto replacement = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_NE(replacement, displayed);
  ASSERT_TRUE(sink->SwapBack(nullptr));
  ASSERT_TRUE(raster.UpdateFront(retry));
  EXPECT_EQ(mock->ProducedFenceCount(), 1);
}

TEST(SharedImageSinkTest, FailedNewestImportFallsBackToEarlierReadyFrame) {
  auto sink = MakeCanvasSink(SharedImageSink::kTripleBuffer);
  sink->SetFrameAvailableCallback([] {});
  auto earlier = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(nullptr));
  auto newest = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(nullptr));
  SharedImageSinkAccessor raster(sink, [](auto backing) {
    return backing->CreateRepresentation(nullptr);
  });
  bool retry = false;
  auto front = raster.UpdateFront(
      retry, [&](auto* repr) { return repr->GetBacking() != newest.get(); });
  ASSERT_TRUE(front);
  EXPECT_EQ(front->GetBacking(), earlier.get());
  EXPECT_FALSE(retry);
}

TEST(SharedImageSinkTest, MissingRasterRetryDoesNotPinBufferIndefinitely) {
  auto sink = MakeCanvasSink();
  sink->SetFrameAvailableCallback([] {});
  SharedImageSinkAccessor raster(
      sink, [](auto b) { return b->CreateRepresentation(nullptr); });
  bool retry;
  auto displayed = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(nullptr));
  ASSERT_TRUE(raster.UpdateFront(retry));
  auto pending = std::get<0>(sink->AcquireBackForced(kSize));
  auto completion = FenceSync::PollResult::kPending;
  ASSERT_TRUE(sink->SwapBack(std::make_unique<CanvasWriteFence>(completion)));
  EXPECT_FALSE(raster.UpdateFront(retry));
  ASSERT_TRUE(retry);
  EXPECT_FALSE(std::get<0>(sink->AcquireBackForced(kSize)));
  auto reused = std::get<0>(sink->AcquireBackForced(kSize));
  EXPECT_NE(reused, displayed);
  ASSERT_EQ(reused, pending);
  ASSERT_TRUE(reused->GetFenceSync());
}

TEST(SharedImageSinkTest, WriterRetiresPermanentlyFailedBacking) {
  auto sink = MakeCanvasSink();
  sink->SetFrameAvailableCallback([] {});
  auto old = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(nullptr));
  ASSERT_EQ(sink->UpdateFront(nullptr), old);
  auto bad = std::get<0>(sink->AcquireBackForced(kSize));
  ASSERT_TRUE(sink->SwapBack(std::make_unique<FailedFenceSync>()));
  SharedImageSinkAccessor writer(
      sink, [](auto b) { return b->CreateRepresentation(nullptr); });
  Status status;
  EXPECT_FALSE(std::get<0>(writer.AcquireBackForcedForGL(kSize, status)));
  EXPECT_EQ(status, Status::kBackingRetired);
  auto fresh = std::get<0>(writer.AcquireBackForcedForGL(kSize, status));
  ASSERT_TRUE(fresh);
  EXPECT_NE(fresh->GetBacking(), bad.get());
  EXPECT_NE(fresh->GetBacking(), old.get());
  EXPECT_TRUE(writer.SwapBack());
}

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

TEST(SharedImageSinkTest, CancelBackAllowsReconfigurationWithoutPublishing) {
  SharedImageSinkManaged sink(SharedImageSink::kSingleBuffer, MakeBacking);
  int available = 0;
  sink.SetFrameAvailableCallback([&] { ++available; });
  auto [back, age] = sink.AcquireBackForced(kSize);
  ASSERT_TRUE(back);
  sink.CancelBack();
  EXPECT_EQ(available, 0);
  EXPECT_TRUE(sink.UpdateSharedImageFactory(MakeBacking));
  EXPECT_NE(std::get<0>(sink.AcquireBackForced(kSize)), back);
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
