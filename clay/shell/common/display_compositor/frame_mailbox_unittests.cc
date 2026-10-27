// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>

#include "clay/common/graphics/staged_image_read_scope.h"
#include "clay/common/raster_thread_group.h"
#include "clay/common/render_service_runtime.h"
#include "clay/flow/compositor/compositor_frame_builder.h"
#include "clay/flow/compositor/raster_buffer_pool.h"
#include "clay/gfx/shared_image/texture_budget.h"
#include "clay/shell/common/display_compositor/test_support.h"
#include "gtest/gtest.h"
namespace clay::display {
using testing_support::Binding;
using testing_support::Config;
using testing_support::ManualRunner;
using testing_support::Publication;
using testing_support::Record;
using testing_support::Signal;
namespace {
RasterPublication PlannedPublication(uint64_t render, uint64_t plan,
                                     uint32_t segment_count) {
  auto publication = Publication(render);
  auto scene = std::make_shared<PageSceneSnapshot>(*publication.scene);
  auto result = std::make_shared<RasterResult>(*publication.result);
  scene->segment_plan_version = result->segment_plan_version = plan;
  for (uint32_t i = 1; i < segment_count; ++i) {
    const SegmentPlacement placement{i + 1, {0, static_cast<int32_t>(i * 50)}};
    scene->ordered_segments.push_back(placement);
    result->outputs.push_back(
        {placement.segment_id, render,
         std::make_shared<OutputTexture>(Record(), render),
         placement.properties, std::nullopt});
  }
  return {std::move(scene), std::move(result)};
}
}  // namespace
TEST(ResourceRecord, CpuLeaseAndEachGpuReadPreventRewrite) {
  auto record = Record();
  auto texture = std::make_shared<OutputTexture>(record, 1);
  auto a = record->BeginRead(), b = record->BeginRead();
  ASSERT_NE(a, b);
  auto first = std::make_shared<Signal>(), second = std::make_shared<Signal>();
  first->status = second->status = CompletionStatus::kPending;
  record->EndRead(a, first);
  record->EndRead(b, second);
  texture.reset();
  EXPECT_FALSE(record->CanReuse());
  first->status = CompletionStatus::kComplete;
  EXPECT_FALSE(record->CanReuse());
  second->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}
TEST(ResourceRecord, SubmittedReadKeepsBothGpuDependenciesAlive) {
  auto write = std::make_shared<Signal>();
  write->status = CompletionStatus::kPending;
  auto record = Record(write);
  auto texture = std::make_shared<OutputTexture>(record, 1);
  auto use = record->BeginRead();
  ASSERT_NE(use, 0u);
  EXPECT_EQ(write->polls, 0u);
  EXPECT_TRUE(texture->ready()->EnqueueWait());
  EXPECT_EQ(write->waits, 1u);
  auto read = std::make_shared<Signal>();
  read->status = CompletionStatus::kPending;
  record->EndRead(use, read);
  texture.reset();
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
  write->status = CompletionStatus::kComplete;
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
  read->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}
TEST(ResourceRecord, FinishedReadCannotReleaseAnUnfinishedProducerWrite) {
  auto write = std::make_shared<Signal>();
  write->status = CompletionStatus::kPending;
  auto record = Record(write);
  auto use = record->BeginRead();
  ASSERT_NE(use, 0u);
  ASSERT_TRUE(write->EnqueueWait());
  record->EndRead(use, std::make_shared<Signal>());
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
  write->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}
TEST(ResourceRecord, MissingAndUnresolvedWritesRejectReads) {
  auto prototype = Record();
  auto record = std::make_shared<ResourceRecord>(
      prototype->key(), prototype->image(), prototype->bytes(), nullptr);
  EXPECT_EQ(record->BeginRead(), 0u);
  ASSERT_TRUE(record->BeginWrite());
  EXPECT_EQ(record->BeginRead(), 0u);
  record->EndWrite(nullptr);
  EXPECT_EQ(record->BeginRead(), 0u);
  EXPECT_FALSE(record->CanDestroy());
}
TEST(GPUCompletion, DefaultWaitRequiresCompletionAndDoesNotExportAFence) {
  class PollOnly : public GPUCompletion {
   public:
    CompletionStatus Poll() const override { return status; }
    CompletionStatus status = CompletionStatus::kPending;
  };
  PollOnly completion;
  EXPECT_FALSE(completion.EnqueueWait());
  EXPECT_EQ(completion.DuplicateNativeFenceFD(), -1);
  completion.status = CompletionStatus::kFailed;
  EXPECT_FALSE(completion.EnqueueWait());
  completion.status = CompletionStatus::kComplete;
  EXPECT_TRUE(completion.EnqueueWait());
}
TEST(ResourceRecord, FailedProducerIsNeverRewritten) {
  auto signal = std::make_shared<Signal>();
  auto record = Record(signal);
  signal->status = CompletionStatus::kFailed;
  EXPECT_FALSE(record->BeginWrite());
  EXPECT_TRUE(record->CanDestroy());
}
TEST(ResourceRecord, RetiredAllocationCanDrainButCannotBeReused) {
  auto record = Record();
  record->Retire();
  EXPECT_EQ(record->BeginRead(), 0u);
  EXPECT_FALSE(record->CanReuse());
  EXPECT_TRUE(record->CanDestroy());
}
TEST(ResourceRecord, PendingReadCanBeCancelled) {
  auto record = Record();
  auto id = record->BeginRead();
  EXPECT_FALSE(record->CanDestroy());
  record->CancelRead(id);
  EXPECT_TRUE(record->CanReuse());
}
TEST(ResourceRecord, PublishedSnapshotPinsOneStorageVersion) {
  auto record = Record();
  auto texture = std::make_shared<OutputTexture>(record, 19);
  auto copy = texture;
  texture.reset();
  EXPECT_FALSE(record->BeginWrite());
  EXPECT_EQ(copy->content_version(), 19u);
  copy.reset();
  EXPECT_TRUE(record->BeginWrite());
  record->EndWrite(std::make_shared<Signal>());
}
TEST(OutputTexture, PublicationCropIsImmutableWhileGpuReadIsPending) {
  auto record = Record();
  const PixelRect first_crop{0, 50, 100, 150};
  const PixelRect second_crop{0, 25, 100, 175};
  auto first = OutputTexture::WithValidRect(record, 19, first_crop);
  ASSERT_TRUE(first);
  auto read = record->BeginRead();
  ASSERT_NE(read, 0u);
  auto completion = std::make_shared<Signal>();
  completion->status = CompletionStatus::kPending;
  record->EndRead(read, completion);
  // Two immutable views of one submitted storage version need not share crop.
  auto second = OutputTexture::WithValidRect(record, 19, second_crop);
  ASSERT_TRUE(second);
  EXPECT_EQ(first->image().valid_rect, first_crop);
  EXPECT_EQ(second->image().valid_rect, second_crop);
  EXPECT_EQ(record->image().valid_rect, (PixelRect{0, 0, 100, 200}));
  EXPECT_EQ(second->image().width, record->image().width);
  EXPECT_EQ(second->image().height, record->image().height);
  EXPECT_EQ(second->image().origin, record->image().origin);
  EXPECT_FALSE(record->BeginWrite());
  first.reset();
  second.reset();
  EXPECT_FALSE(record->BeginWrite());
  completion->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->BeginWrite());
  record->EndWrite(std::make_shared<Signal>());
}
TEST(OutputTexture, InvalidCropDoesNotAcquireAStorageLease) {
  auto record = Record();
  EXPECT_FALSE(OutputTexture::WithValidRect(nullptr, 1, {0, 0, 1, 1}));
  EXPECT_FALSE(OutputTexture::WithValidRect(record, 1, {-1, 0, 1, 1}));
  EXPECT_FALSE(OutputTexture::WithValidRect(record, 1, {0, -1, 1, 1}));
  EXPECT_FALSE(OutputTexture::WithValidRect(record, 1, {0, 0, 0, 1}));
  EXPECT_FALSE(OutputTexture::WithValidRect(record, 1, {0, 0, 1, 0}));
  EXPECT_FALSE(OutputTexture::WithValidRect(record, 1, {1, 0, 100, 200}));
  EXPECT_FALSE(OutputTexture::WithValidRect(record, 1, {0, 1, 100, 200}));
  EXPECT_FALSE(
      OutputTexture::WithValidRect(record, 1, {2147483647, 0, UINT32_MAX, 1}));
  EXPECT_TRUE(record->CanReuse());
}
TEST(RasterBufferPool, KeepsCapacityAcrossScrollBucketBoundaries) {
  RasterBufferSize capacity;
  uint32_t allocations = 0;
  for (auto height : {1800u, 1790u, 1700u, 1640u, 1700u, 1790u, 1800u}) {
    const RasterBufferSize logical{1002, height};
    if (!CanUseRasterBuffer(capacity, logical, 2040)) {
      capacity = RasterBufferAllocationSize(logical, 2040);
      ++allocations;
    }
    EXPECT_TRUE(CanUseRasterBuffer(capacity, logical, 2040));
  }
  EXPECT_EQ(allocations, 1u);
  EXPECT_EQ(capacity, (RasterBufferSize{1002, 2040}));
  EXPECT_FALSE(CanUseRasterBuffer(capacity, {1002, 1530}, 2040));
  EXPECT_FALSE(CanUseRasterBuffer(capacity, {1003, 1800}, 2040));
  EXPECT_EQ(RasterBufferAllocationSize({1002, 1530}, 2040),
            (RasterBufferSize{1002, 1536}));
}
TEST(RasterBufferPool, RejectsInvalidSizesBeforeAligning) {
  EXPECT_EQ(RasterBufferAllocationSize({0, 1}, 2040), RasterBufferSize{});
  EXPECT_EQ(RasterBufferAllocationSize({1, 0}, 2040), RasterBufferSize{});
  EXPECT_EQ(RasterBufferAllocationSize({1, 2041}, 2040), RasterBufferSize{});
  EXPECT_EQ(RasterBufferAllocationSize({UINT32_MAX, 1}, 2040),
            RasterBufferSize{});
  EXPECT_EQ(RasterBufferAllocationSize({1, UINT32_MAX}, UINT32_MAX),
            RasterBufferSize{});
  EXPECT_EQ(RasterBufferAllocationSize({16384, 16384}, 16384),
            (RasterBufferSize{16384, 16384}));
  EXPECT_EQ(RasterBufferAllocationSize({753, 425}, 2040),
            (RasterBufferSize{753, 512}));
}
TEST(RasterBufferPool, BottomLeftCropPreservesLogicalDestination) {
  const RasterBufferSize physical{1002, 2040}, logical{1002, 1640};
  const auto bottom =
      RasterBufferValidRect(physical, logical, ImageOrigin::kBottomLeft);
  EXPECT_EQ(bottom, (PixelRect{0, 400, 1002, 1640}));
  EXPECT_EQ(physical.height - bottom.y - bottom.height, 0u);
  EXPECT_EQ(physical.height - bottom.y, logical.height);
  EXPECT_EQ(RasterBufferValidRect(physical, logical, ImageOrigin::kTopLeft),
            (PixelRect{0, 0, 1002, 1640}));
  EXPECT_EQ(RasterBufferValidRect(logical, physical, ImageOrigin::kBottomLeft),
            PixelRect{});
}
TEST(FrameMailbox, CoalescesNotificationsAndKeepsLatestCompleteResult) {
  FrameMailbox mailbox(Binding());
  EXPECT_EQ(mailbox.Publish(Publication(1)),
            FrameMailbox::PublishResult::kWake);
  EXPECT_EQ(mailbox.Publish(Publication(2)),
            FrameMailbox::PublishResult::kCoalesced);
  EXPECT_EQ(mailbox.TakeLatest().result->render_version, 2u);
  EXPECT_FALSE(mailbox.TakeLatest());
  EXPECT_EQ(mailbox.Publish(Publication(3)),
            FrameMailbox::PublishResult::kWake);
}
TEST(FrameMailbox, SubmittedReplacementTransfersBeforeGpuCompletion) {
  FrameMailbox mailbox(Binding());
  ASSERT_NE(mailbox.Publish(Publication(1)),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  EXPECT_NE(mailbox.Publish(Publication(2, 1, signal)),
            FrameMailbox::PublishResult::kRejected);
  auto submitted = mailbox.TakeLatest();
  ASSERT_TRUE(submitted);
  EXPECT_EQ(submitted.result->render_version, 2u);
  EXPECT_EQ(submitted.result->outputs[0].texture_ref->ready(), signal);
  EXPECT_EQ(signal->polls, 0u);
  EXPECT_TRUE(signal->EnqueueWait());
  EXPECT_EQ(signal->status, CompletionStatus::kPending);
  EXPECT_FALSE(mailbox.TakeLatest());
}
TEST(FrameMailbox, MissingDependencyDoesNotReplaceSubmittedSnapshot) {
  FrameMailbox mailbox(Binding());
  ASSERT_NE(mailbox.Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_EQ(mailbox.Publish(Publication(2, 1, nullptr)),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_EQ(mailbox.TakeLatest().result->render_version, 1u);
}
TEST(FrameMailbox, CompleteMetadataPublishesAllSubmittedSegmentsTogether) {
  FrameMailbox mailbox(Binding());
  ASSERT_NE(mailbox.Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  auto first = std::make_shared<Signal>(), second = std::make_shared<Signal>();
  first->status = second->status = CompletionStatus::kPending;
  auto p = Publication(2, 1, first);
  auto scene = std::make_shared<PageSceneSnapshot>(*p.scene);
  scene->ordered_segments.push_back({2, {}});
  auto result = std::make_shared<RasterResult>(*p.result);
  EXPECT_EQ(mailbox.Publish({scene, result}),
            FrameMailbox::PublishResult::kRejected);
  result->outputs.push_back(
      {2, 2, std::make_shared<OutputTexture>(Record(second), 2), {}, {}});
  result->outputs[1].content_version = 1;
  EXPECT_EQ(mailbox.Publish({scene, result}),
            FrameMailbox::PublishResult::kRejected);
  result->outputs[1].content_version = 2;
  result->outputs[1].segment_id = 1;
  EXPECT_EQ(mailbox.Publish({scene, result}),
            FrameMailbox::PublishResult::kRejected);
  result->outputs[1].segment_id = 2;
  ASSERT_NE(mailbox.Publish({scene, result}),
            FrameMailbox::PublishResult::kRejected);
  auto latest = mailbox.TakeLatest();
  ASSERT_TRUE(latest);
  EXPECT_EQ(latest.result, result);
  ASSERT_EQ(latest.result->outputs.size(), 2u);
  EXPECT_EQ(latest.result->outputs[0].texture_ref->ready(), first);
  EXPECT_EQ(latest.result->outputs[1].texture_ref->ready(), second);
  EXPECT_EQ(first->polls, 0u);
  EXPECT_EQ(second->polls, 0u);
}
TEST(FrameMailbox, ReplacedPendingWriteStillProtectsItsAllocation) {
  FrameMailbox mailbox(Binding());
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto p = Publication(1, 1, signal);
  auto record = p.result->outputs[0].texture_ref->resource();
  std::weak_ptr<const OutputTexture> old = p.result->outputs[0].texture_ref;
  ASSERT_NE(mailbox.Publish(std::move(p)),
            FrameMailbox::PublishResult::kRejected);
  ASSERT_NE(mailbox.Publish(Publication(2)),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_TRUE(old.expired());
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
  signal->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}
TEST(FrameMailbox, RejectsOutOfOrderRenderAndOldLayout) {
  FrameMailbox mailbox(Binding());
  mailbox.Publish(Publication(3, 2));
  EXPECT_EQ(mailbox.Publish(Publication(2, 2)),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_EQ(mailbox.Publish(Publication(4, 1)),
            FrameMailbox::PublishResult::kRejected);
}
TEST(FrameMailbox, OldBindingAndClosedMailboxRejectResults) {
  auto binding = Binding();
  ++binding.binding_generation;
  FrameMailbox mailbox(binding);
  EXPECT_EQ(mailbox.Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
  FrameMailbox open(Binding());
  open.Close();
  EXPECT_EQ(open.Publish(Publication()),
            FrameMailbox::PublishResult::kRejected);
}
TEST(FrameMailbox, ReplacedCandidateReleasesCpuLease) {
  FrameMailbox mailbox(Binding());
  auto p = Publication();
  auto record = p.result->outputs[0].texture_ref->resource();
  mailbox.Publish(std::move(p));
  EXPECT_FALSE(record->CanReuse());
  mailbox.Publish(Publication(2));
  EXPECT_TRUE(record->CanReuse());
}
TEST(FrameMailbox, ReadPendingAndWriteSlotsAdvanceWithoutWaitingForReader) {
  FrameMailbox mailbox(Binding());
  auto reading = Record(), pending = Record(), writing = Record();
  auto publish = [&](const auto& record, uint64_t version) {
    ASSERT_TRUE(record->BeginWrite());
    record->EndWrite(std::make_shared<Signal>());
    auto p = Publication(version);
    auto result = std::make_shared<RasterResult>(*p.result);
    result->outputs[0].texture_ref =
        std::make_shared<OutputTexture>(record, version);
    p.result = std::move(result);
    EXPECT_NE(mailbox.Publish(std::move(p)),
              FrameMailbox::PublishResult::kRejected);
  };
  publish(reading, 1);
  auto consumer = mailbox.TakeLatest();
  ASSERT_TRUE(consumer);
  EXPECT_FALSE(mailbox.TakeLatest());
  auto read_done = std::make_shared<Signal>();
  read_done->status = CompletionStatus::kPending;
  reading->EndRead(reading->BeginRead(), read_done);
  consumer = {};

  publish(pending, 2);
  // A GPU reader and a pending frame do not consume the producer's work slot.
  publish(writing, 3);
  EXPECT_FALSE(reading->CanReuse());
  EXPECT_TRUE(pending->CanReuse());
  auto latest = mailbox.TakeLatest();
  ASSERT_TRUE(latest);
  EXPECT_EQ(latest.result->render_version, 3u);
  EXPECT_EQ(latest.result->outputs[0].texture_ref->resource(), writing);
  EXPECT_FALSE(mailbox.TakeLatest());
  publish(pending, 4);
  EXPECT_EQ(mailbox.TakeLatest().result->render_version, 4u);
  EXPECT_FALSE(reading->BeginWrite());
  read_done->status = CompletionStatus::kComplete;
  EXPECT_TRUE(reading->CanReuse());
}
class BuilderTest : public testing::Test {
 protected:
  void SetUp() override {
    builder.SetBinding(Binding());
    builder.SetOutputConfig(Config());
    builder.NotifyLayoutSubmitted(1);
  }
  bool Accept(RasterPublication p) {
    builder.NotifyLayoutSubmitted(p.scene->layout_version);
    return builder.SetScene(p.scene) && builder.Accept(p.result);
  }
  CompositorFrameBuilder builder;
};
TEST_F(BuilderTest, BuildTransfersPendingResultWithoutRetainingItsTexture) {
  auto publication = Publication();
  auto record = publication.result->outputs[0].texture_ref->resource();
  ASSERT_TRUE(Accept(std::move(publication)));
  auto frame = builder.Build({1, 1});
  ASSERT_TRUE(frame);
  EXPECT_FALSE(builder.latest());
  EXPECT_FALSE(builder.Build({1, 2}));
  EXPECT_FALSE(record->CanReuse());
  frame.reset();
  EXPECT_TRUE(record->CanReuse());
}
TEST_F(BuilderTest, SubmittedDependencyPassesThroughWithoutCpuReadinessQuery) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  ASSERT_TRUE(Accept(Publication(1, 1, signal)));
  auto frame = builder.Build({1, 1});
  ASSERT_TRUE(frame);
  EXPECT_EQ(frame->result->outputs[0].texture_ref->ready(), signal);
  EXPECT_EQ(signal->polls, 0u);
  EXPECT_FALSE(builder.latest());
}
TEST_F(BuilderTest, InvalidWindowDoesNotConsumeThePendingResult) {
  ASSERT_TRUE(Accept(Publication()));
  EXPECT_FALSE(builder.Build({0, 1}));
  EXPECT_FALSE(builder.Build({1, 0}));
  ASSERT_TRUE(builder.latest());
  auto frame = builder.Build({1, 2});
  ASSERT_TRUE(frame);
  EXPECT_EQ(frame->window.generation, 2u);
  EXPECT_FALSE(builder.latest());
}
TEST_F(BuilderTest, PendingLayoutPreservesCompleteSnapshot) {
  ASSERT_TRUE(Accept(Publication()));
  builder.NotifyLayoutSubmitted(2);
  auto retained = builder.Build({1, 1});
  ASSERT_TRUE(retained);
  EXPECT_EQ(retained->result->layout_version, 1u);
  EXPECT_TRUE(Accept(Publication(2, 1)));
  ASSERT_TRUE(Accept(Publication(3, 2)));
  EXPECT_EQ(builder.Build({1, 1})->result->layout_version, 2u);
  EXPECT_FALSE(Accept(Publication(4, 1)));
}
TEST_F(BuilderTest, ContinuousSubmissionsDoNotStarveCompletedFrames) {
  // Reproduce UI announcing N+1 before Raster publishes the complete N.
  for (uint64_t layout = 1; layout <= 60; ++layout) {
    builder.NotifyLayoutSubmitted(layout + 1);
    ASSERT_TRUE(Accept(Publication(layout, layout)));
    auto frame = builder.Build({1, 1});
    ASSERT_TRUE(frame);
    EXPECT_EQ(frame->result->layout_version, layout);
  }
}
TEST_F(BuilderTest, UnannouncedLayoutAndMixedLayoutResultAreRejected) {
  auto newer = Publication(2, 2);
  EXPECT_FALSE(builder.SetScene(newer.scene));
  builder.NotifyLayoutSubmitted(2);
  ASSERT_TRUE(builder.SetScene(newer.scene));
  EXPECT_FALSE(builder.Accept(Publication(3, 1).result));
  EXPECT_FALSE(builder.Build({1, 1}));
  EXPECT_TRUE(builder.Accept(newer.result));
}
TEST_F(BuilderTest, ConfigChangeRejectsOldSizeAndDpr) {
  ASSERT_TRUE(Accept(Publication()));
  auto config = Config();
  config.device_pixel_ratio = 2;
  config.version = 2;
  builder.SetOutputConfig(config);
  EXPECT_FALSE(builder.Build({1, 1}));
  EXPECT_FALSE(Accept(Publication(2)));
}
TEST_F(BuilderTest, PartialOrReorderedOutputIsRejected) {
  auto p = Publication();
  auto scene = std::make_shared<PageSceneSnapshot>(*p.scene);
  scene->ordered_segments.push_back({2, {}});
  ASSERT_TRUE(builder.SetScene(scene));
  EXPECT_FALSE(builder.Accept(p.result));
}
TEST_F(BuilderTest, NewPlanDoesNotLeaveAnOldBuildableResult) {
  auto p = Publication();
  ASSERT_TRUE(Accept(p));
  auto scene = std::make_shared<PageSceneSnapshot>(*p.scene);
  ++scene->segment_plan_version;
  EXPECT_TRUE(builder.SetScene(scene));
  EXPECT_FALSE(builder.Build({1, 1}));
  EXPECT_FALSE(builder.SetScene(p.scene));
}
TEST_F(BuilderTest, SameLayoutSwitchesSplitWholeAndSplitAsCompletePlans) {
  FrameMailbox mailbox(Binding());
  for (uint32_t phase = 1; phase <= 3; ++phase) {
    const uint32_t count = phase == 2 ? 1 : 3;
    const uint64_t plan = phase + 10;
    ASSERT_EQ(mailbox.Publish(PlannedPublication(phase, plan, count)),
              FrameMailbox::PublishResult::kWake);
    auto publication = mailbox.TakeLatest();
    ASSERT_TRUE(publication);
    ASSERT_TRUE(Accept(std::move(publication)));
    auto frame = builder.Build({1, 1});
    ASSERT_TRUE(frame);
    EXPECT_EQ(frame->result->layout_version, 1u);
    EXPECT_EQ(frame->result->segment_plan_version, plan);
    EXPECT_EQ(frame->result->render_version, phase);
    ASSERT_EQ(frame->result->outputs.size(), count);
    for (uint32_t i = 0; i < count; ++i) {
      EXPECT_EQ(frame->result->outputs[i].segment_id, i + 1);
      EXPECT_EQ(frame->result->outputs[i].content_version, phase);
    }
    EXPECT_FALSE(builder.Build({1, 1}));
    EXPECT_FALSE(mailbox.TakeLatest());
  }
}
TEST_F(BuilderTest, IncompleteRestoredPlanCannotReplaceWholeOrMixOldOutputs) {
  FrameMailbox mailbox(Binding());
  auto whole = PlannedPublication(1, 11, 1);
  ASSERT_EQ(mailbox.Publish(whole), FrameMailbox::PublishResult::kWake);
  auto restored = PlannedPublication(2, 12, 3);
  auto partial = std::make_shared<RasterResult>(*restored.result);
  partial->outputs.pop_back();
  EXPECT_EQ(mailbox.Publish({restored.scene, partial}),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_EQ(mailbox.Publish({restored.scene, whole.result}),
            FrameMailbox::PublishResult::kRejected);

  auto retained = mailbox.TakeLatest();
  ASSERT_TRUE(retained);
  EXPECT_EQ(retained.result, whole.result);
  ASSERT_TRUE(Accept(std::move(retained)));
  auto displayed = builder.Build({1, 1});
  ASSERT_TRUE(displayed);
  ASSERT_EQ(displayed->result->outputs.size(), 1u);
  EXPECT_EQ(displayed->result->segment_plan_version, 11u);

  ASSERT_TRUE(builder.SetScene(restored.scene));
  EXPECT_FALSE(builder.Accept(whole.result));
  EXPECT_FALSE(builder.Accept(partial));
  auto mixed = std::make_shared<RasterResult>(*restored.result);
  mixed->outputs[1] = whole.result->outputs[0];
  EXPECT_FALSE(builder.Accept(mixed));
  EXPECT_FALSE(builder.Build({1, 1}));
  EXPECT_EQ(displayed->result, whole.result);

  ASSERT_EQ(mailbox.Publish(restored), FrameMailbox::PublishResult::kWake);
  ASSERT_TRUE(Accept(mailbox.TakeLatest()));
  auto next = builder.Build({1, 1});
  ASSERT_TRUE(next);
  ASSERT_EQ(next->result->outputs.size(), 3u);
  EXPECT_EQ(next->result->segment_plan_version, 12u);
  EXPECT_EQ(displayed->result->segment_plan_version, 11u);
}
TEST_F(BuilderTest, PlanReplacementReleasesPendingButPreservesConsumedReads) {
  FrameMailbox mailbox(Binding());
  auto split = PlannedPublication(1, 11, 3);
  std::vector<std::shared_ptr<ResourceRecord>> records;
  for (const auto& entry : split.result->outputs)
    records.push_back(entry.texture_ref->resource());
  ASSERT_EQ(mailbox.Publish(std::move(split)),
            FrameMailbox::PublishResult::kWake);
  ASSERT_TRUE(Accept(mailbox.TakeLatest()));
  auto displayed = builder.Build({1, 1});
  ASSERT_TRUE(displayed);
  auto read_done = std::make_shared<Signal>();
  read_done->status = CompletionStatus::kPending;
  for (auto& record : records) {
    EXPECT_FALSE(record->CanReuse());
    const auto use = record->BeginRead();
    ASSERT_NE(use, 0u);
    record->EndRead(use, read_done);
  }

  auto whole = PlannedPublication(2, 12, 1);
  auto pending_record = whole.result->outputs[0].texture_ref->resource();
  std::weak_ptr<const OutputTexture> pending =
      whole.result->outputs[0].texture_ref;
  ASSERT_EQ(mailbox.Publish(std::move(whole)),
            FrameMailbox::PublishResult::kWake);
  EXPECT_FALSE(pending_record->CanReuse());
  EXPECT_EQ(mailbox.Publish(PlannedPublication(3, 13, 3)),
            FrameMailbox::PublishResult::kCoalesced);
  EXPECT_TRUE(pending.expired());
  EXPECT_TRUE(pending_record->CanReuse());
  for (auto& record : records) EXPECT_FALSE(record->BeginWrite());

  displayed.reset();
  for (auto& record : records) {
    EXPECT_FALSE(record->CanReuse());
    EXPECT_FALSE(record->CanDestroy());
  }
  read_done->status = CompletionStatus::kComplete;
  for (auto& record : records) EXPECT_TRUE(record->CanReuse());
  auto latest = mailbox.TakeLatest();
  ASSERT_TRUE(latest);
  EXPECT_EQ(latest.result->segment_plan_version, 13u);
  EXPECT_EQ(latest.result->render_version, 3u);
  EXPECT_EQ(latest.result->outputs.size(), 3u);
}
TEST_F(BuilderTest, WrongDeviceAllocationCannotBeImported) {
  auto p = Publication();
  auto result = std::make_shared<RasterResult>(*p.result);
  auto image = result->outputs[0].texture_ref->image();
  auto record = std::make_shared<ResourceRecord>(ResourceKey{1, 2, 1, 1}, image,
                                                 80000, nullptr);
  record->BeginWrite();
  record->EndWrite(std::make_shared<Signal>());
  result->outputs[0].texture_ref = std::make_shared<OutputTexture>(record, 1);
  ASSERT_TRUE(builder.SetScene(p.scene));
  EXPECT_FALSE(builder.Accept(result));
}
TEST(OutputConfig, WindowAttachmentPolicyChangesContentConfig) {
  auto window = Config();
  EXPECT_TRUE(window.uses_window_attachments);
  auto functor = window;
  functor.uses_window_attachments = false;
  EXPECT_EQ(window.version, functor.version);
  EXPECT_FALSE(window.SameContentConfig(functor));
  EXPECT_NE(window, functor);
  EXPECT_TRUE(functor.IsValid());
}
TEST(TextureBudget, FunctorPreflightDoesNotCountUnusedWindowAttachments) {
  auto config = Config();
  config.width = 1080;
  config.height = 2040;
  constexpr uint64_t output_pixels = 4280000;
  const auto window =
      TextureBudget::EstimatePoolBytes(config, output_pixels, 12, 12, 8);
  config.uses_window_attachments = false;
  const auto functor =
      TextureBudget::EstimatePoolBytes(config, output_pixels, 12, 12, 8);
  EXPECT_EQ(window, 268646400u);
  EXPECT_EQ(functor, 233395200u);
  EXPECT_EQ(window - functor, 35251200u);

  auto budget = std::make_shared<TextureBudget>();
  EXPECT_GT(window, budget->limit());
  EXPECT_LT(functor, budget->limit());
  EXPECT_FALSE(budget->Reserve(window));
  auto pools = budget->Reserve(functor);
  ASSERT_TRUE(pools);
  auto remaining = budget->Reserve(budget->limit() - budget->used());
  ASSERT_TRUE(remaining);
  EXPECT_EQ(budget->used(), budget->limit());
  EXPECT_FALSE(budget->Reserve(1));
  EXPECT_FALSE(pools->Resize(functor + 1));
  EXPECT_EQ(budget->used(), budget->limit());
  pools.reset();
  remaining.reset();
  EXPECT_EQ(budget->used(), 0u);
  budget->SetLowMemory();
  EXPECT_EQ(budget->limit(), 128u * 1024u * 1024u);
  EXPECT_GT(functor, budget->limit());
  EXPECT_GT(window, budget->limit());
  EXPECT_FALSE(budget->Reserve(functor));
  EXPECT_EQ(budget->used(), 0u);
}
TEST(TextureBudget, FullPageRasterLeavesRoomForFunctorToAdvance) {
  // Both read/pending/write pools, two native-window submissions and an
  // explicit capture must fit together at a representative full-page size.
  constexpr uint64_t pixels = 1080u * 2208u;
  auto budget = std::make_shared<TextureBudget>();
  std::vector<std::shared_ptr<TextureBudget::Reservation>> raster;
  for (uint32_t i = 0; i < kMaxRasterBuffersPerSegment; ++i) {
    raster.push_back(budget->Reserve(pixels * 12));
    ASSERT_TRUE(raster.back());
  }
  std::vector<std::shared_ptr<TextureBudget::Reservation>> display;
  for (uint32_t i = 0; i < kMaxDisplayOutputBuffers; ++i) {
    display.push_back(budget->Reserve(pixels * 12));
    ASSERT_TRUE(display.back());
  }
  auto first_window = budget->Reserve(pixels * 8);
  auto second_window = budget->Reserve(pixels * 8);
  ASSERT_TRUE(first_window);
  ASSERT_TRUE(second_window);
  auto readback = budget->Reserve(pixels * 8);
  ASSERT_TRUE(readback);
  EXPECT_LE(budget->used(), budget->limit());
  // The shared limit still prevents unbounded growth beyond the two pools.
  EXPECT_FALSE(budget->Reserve(pixels * 24));
}
TEST(TextureBudget, ReservationOutlivesPageOwner) {
  auto budget = std::make_shared<TextureBudget>(100);
  std::weak_ptr<TextureBudget> weak = budget;
  auto token = budget->Reserve(60);
  EXPECT_FALSE(budget->Reserve(41));
  EXPECT_EQ(budget->used(), 60u);
  budget.reset();
  EXPECT_FALSE(weak.expired());
  token.reset();
  EXPECT_TRUE(weak.expired());
}
TEST(TextureBudget, RejectsZeroAndOverflow) {
  auto budget = std::make_shared<TextureBudget>(100);
  EXPECT_FALSE(budget->Reserve(0));
  EXPECT_FALSE(budget->Reserve(UINT64_MAX));
}
class BindingTest : public testing::Test {
 protected:
  void SetUp() override {
    group = std::make_unique<RasterThreadGroup>(platform, worker);
  }
  std::shared_ptr<RasterBinding> Register(bool success = true) {
    return group->RegisterRasterizer(
        1, 1, [success] { return success; },
        [this](bool ready) { ready_count += ready; },
        [this](uint64_t version) { layout = version; }, [](auto) {});
  }
  fml::RefPtr<ManualRunner> platform = fml::MakeRefCounted<ManualRunner>(),
                            worker = fml::MakeRefCounted<ManualRunner>();
  std::unique_ptr<RasterThreadGroup> group;
  int ready_count = 0;
  uint64_t layout = 0;
};
TEST_F(BindingTest, RegisteringKeepsOnlyLatestTree) {
  auto binding = Register();
  int count = 0;
  binding->PostTreeUpdate([&](auto) { count = 1; });
  binding->PostTreeUpdate([&](auto version) { count = version; });
  EXPECT_EQ(count, 0);
  worker->Run();
  platform->Run();
  EXPECT_EQ(count, 2);
  EXPECT_EQ(layout, 2u);
  EXPECT_EQ(ready_count, 1);
}
TEST_F(BindingTest, ContentCoalescesAndRejectsStaleSourceGeneration) {
  auto binding = Register();
  int count = 0;
  EXPECT_TRUE(binding->NotifyContentUpdated({7, 2, 1, [&] { count = 1; }}));
  EXPECT_TRUE(binding->NotifyContentUpdated({7, 2, 2, [&] { count = 2; }}));
  EXPECT_FALSE(binding->NotifyContentUpdated({7, 1, 100, [] {}}));
  worker->Run();
  platform->Run();
  EXPECT_EQ(count, 2);
}
TEST_F(BindingTest, LatestContentIsMarkedBeforeTreeCommit) {
  auto binding = Register();
  uint64_t dirty_version = 0, version_at_tree_commit = 0;
  int content_callbacks = 0, tree_commits = 0;
  binding->PostTreeUpdate([&](auto) {
    ++tree_commits;
    version_at_tree_commit = dirty_version;
  });
  ASSERT_TRUE(binding->NotifyContentUpdated({7, 2, 1, [&] {
                                               ++content_callbacks;
                                               dirty_version = 1;
                                             }}));
  ASSERT_TRUE(binding->NotifyContentUpdated({7, 2, 2, [&] {
                                               ++content_callbacks;
                                               dirty_version = 2;
                                             }}));
  worker->Run();
  platform->Run();
  EXPECT_EQ(content_callbacks, 1);
  EXPECT_EQ(tree_commits, 1);
  EXPECT_EQ(version_at_tree_commit, 2u);
}
TEST_F(BindingTest, ClosingStopsQueuedWorkAndRetiresOnlyAfterDrain) {
  auto binding = Register();
  worker->Run();
  platform->Run();
  int count = 0;
  std::function<void()> finish;
  binding->PostTreeUpdate([&](auto) { ++count; });
  binding->Close([&](auto drained) { finish = std::move(drained); },
                 [&] { ++count; });
  worker->Run();
  platform->Run();
  EXPECT_EQ(count, 0);
  EXPECT_EQ(binding->state(), RasterBinding::State::kClosing);
  EXPECT_EQ(binding->PostTreeUpdate([](auto) {}), 0u);
  finish();
  platform->Run();
  EXPECT_EQ(count, 1);
  EXPECT_EQ(binding->state(), RasterBinding::State::kRetired);
}
TEST_F(BindingTest, CloseIsIdempotentAndNotifiesEveryWaiter) {
  auto binding = Register();
  worker->Run();
  int releases = 0, callbacks = 0;
  auto release = [&](auto done) {
    ++releases;
    done();
  };
  binding->Close(release, [&] { ++callbacks; });
  binding->Close(release, [&] { ++callbacks; });
  worker->Run();
  platform->Run();
  EXPECT_EQ(releases, 1);
  EXPECT_EQ(callbacks, 2);
}
TEST_F(BindingTest, RegistrationFailureNeverRunsTree) {
  auto binding = Register(false);
  int count = 0;
  binding->PostTreeUpdate([&](auto) { ++count; });
  worker->Run();
  platform->Run();
  EXPECT_EQ(count, 0);
  EXPECT_EQ(binding->state(), RasterBinding::State::kClosing);
  binding->Close([](auto done) { done(); }, [] {});
  worker->Run();
  platform->Run();
}
TEST(TextureBudget, ResizeUsesTheSameSharedLimitAndRollsBackOnFailure) {
  auto budget = std::make_shared<TextureBudget>(100);
  auto a = budget->Reserve(40), b = budget->Reserve(40);
  EXPECT_FALSE(a->Resize(70));
  EXPECT_EQ(budget->used(), 80u);
  ASSERT_TRUE(a->Resize(60));
  EXPECT_EQ(budget->used(), 100u);
  ASSERT_TRUE(a->Resize(20));
  EXPECT_EQ(budget->used(), 60u);
  b.reset();
  EXPECT_EQ(budget->used(), 20u);
}
TEST(TextureBudget, LowMemoryNeverReclaimsLiveReservations) {
  auto budget = std::make_shared<TextureBudget>();
  auto allocation = budget->Reserve(160u * 1024 * 1024);
  budget->SetLowMemory();
  EXPECT_FALSE(budget->Reserve(1));
  EXPECT_EQ(budget->used(), 160u * 1024 * 1024);
  allocation.reset();
  EXPECT_EQ(budget->used(), 0u);
  EXPECT_TRUE(budget->Reserve(128u * 1024 * 1024));
}
TEST(Runtime, DeviceBudgetIsSharedAcrossWorkersAndSurvivesRuntimeRelease) {
  auto platform = fml::MakeRefCounted<ManualRunner>(),
       worker = fml::MakeRefCounted<ManualRunner>(),
       other = fml::MakeRefCounted<ManualRunner>();
  auto first = RenderServiceRuntime::Acquire(123, platform, worker);
  EXPECT_EQ(first, RenderServiceRuntime::Acquire(123, platform, worker));
  auto second = RenderServiceRuntime::Acquire(123, platform, other);
  EXPECT_NE(first, second);
  EXPECT_EQ(first->budget(), second->budget());
  auto token = first->budget()->Reserve(512);
  first.reset();
  second.reset();
  auto next = RenderServiceRuntime::Acquire(123, platform, worker);
  EXPECT_EQ(next->budget()->used(), 512u);
  token.reset();
  EXPECT_EQ(next->budget()->used(), 0u);
}
namespace {
class InputReadRecordingRunner : public ManualRunner {
 public:
  void PostDelayedTask(lynx::base::closure task,
                       fml::TimeDelta delay) override {
    delays.push_back(delay.ToMilliseconds());
    ManualRunner::PostDelayedTask(std::move(task), delay);
  }
  std::vector<int64_t> delays;
};
}  // namespace
TEST(InputReadScope, SourceIsHeldUntilEverySegmentWriteFinishes) {
  auto runner = fml::MakeRefCounted<InputReadRecordingRunner>();
  auto state = std::make_shared<SourceReadState>();
  auto first = std::make_shared<Signal>(), second = std::make_shared<Signal>();
  first->status = second->status = CompletionStatus::kPending;
  int releases = 0, contexts = 0;
  int source;
  {
    StagedImageReadScope scope(runner, [&](auto task) {
      ++contexts;
      task();
    });
    scope.Hold(&source, state, [&] { ++releases; });
    scope.Hold(&source, state, [&] { ++releases; });
    EXPECT_TRUE(scope.Contains(&source));
    scope.Track(Record(first));
    scope.Track(Record(second));
  }
  EXPECT_TRUE(state->pending);
  EXPECT_EQ(releases, 0);
  ASSERT_EQ(runner->delays.size(), 1u);
  EXPECT_EQ(runner->delays.back(), 1);
  first->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_EQ(releases, 0);
  ASSERT_EQ(runner->delays.size(), 2u);
  EXPECT_EQ(runner->delays.back(), 1);
  second->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_EQ(releases, 1);
  EXPECT_EQ(contexts, 1);
  EXPECT_FALSE(state->pending);
}
TEST(InputReadScope, ReusedRecordDoesNotExtendThePreviousSourceRead) {
  auto runner = fml::MakeRefCounted<ManualRunner>();
  auto state = std::make_shared<SourceReadState>();
  auto first = std::make_shared<Signal>(), second = std::make_shared<Signal>();
  first->status = second->status = CompletionStatus::kPending;
  auto record = Record(first);
  int released = 0, source;
  {
    StagedImageReadScope scope(runner, [](auto task) { task(); });
    scope.Hold(&source, state, [&] { ++released; });
    scope.Track(record);
  }
  EXPECT_TRUE(state->pending);
  EXPECT_EQ(released, 0);
  first->status = CompletionStatus::kComplete;
  ASSERT_TRUE(record->BeginWrite());
  record->EndWrite(second);
  runner->Poll();
  EXPECT_EQ(released, 1);
  EXPECT_FALSE(state->pending);
  EXPECT_FALSE(record->CanReuse());
  second->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
}
TEST(InputReadScope, ReleasedSourceWaitsForTheNextBatchCompletionAgain) {
  auto runner = fml::MakeRefCounted<ManualRunner>();
  auto state = std::make_shared<SourceReadState>();
  auto first = std::make_shared<Signal>(), second = std::make_shared<Signal>();
  first->status = second->status = CompletionStatus::kPending;
  int releases = 0, source;
  {
    StagedImageReadScope scope(runner, [](auto task) { task(); });
    scope.Hold(&source, state, [&] { ++releases; });
    scope.Track(Record(first));
  }
  EXPECT_TRUE(state->pending);
  runner->Poll();
  EXPECT_TRUE(state->pending);
  EXPECT_EQ(releases, 0);

  first->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_FALSE(state->pending);
  ASSERT_EQ(releases, 1);
  EXPECT_TRUE(runner->polls.empty());

  // Preflight can now permit a newer source read. Completion of the previous
  // batch must not release this second use of the same source/state.
  {
    StagedImageReadScope scope(runner, [](auto task) { task(); });
    scope.Hold(&source, state, [&] { ++releases; });
    scope.Track(Record(second));
  }
  EXPECT_TRUE(state->pending);
  runner->Poll();
  EXPECT_TRUE(state->pending);
  EXPECT_EQ(releases, 1);

  second->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_FALSE(state->pending);
  EXPECT_EQ(releases, 2);
  EXPECT_TRUE(runner->polls.empty());
}
TEST(InputReadScope, AbortedWriteKeepsStorageAndSourceUntilItsFenceFinishes) {
  auto runner = fml::MakeRefCounted<ManualRunner>();
  auto state = std::make_shared<SourceReadState>();
  auto record = Record();
  std::weak_ptr<ResourceRecord> storage_lifetime = record;
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  int released = 0, source;
  {
    StagedImageReadScope scope(runner, [](auto task) { task(); });
    scope.Hold(&source, state, [&] { ++released; });
    ASSERT_TRUE(record->BeginWrite());
    scope.Track(record);
    record->EndWrite(signal);
    record->Retire();
  }
  record.reset();
  runner->Poll();
  EXPECT_FALSE(storage_lifetime.expired());
  EXPECT_TRUE(state->pending);
  EXPECT_EQ(released, 0);
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_EQ(released, 1);
  EXPECT_FALSE(state->pending);
  EXPECT_TRUE(storage_lifetime.expired());
}
TEST(InputReadScope, MissingBatchCompletionCannotUseALaterWriteFence) {
  auto runner = fml::MakeRefCounted<InputReadRecordingRunner>();
  auto state = std::make_shared<SourceReadState>();
  auto record = Record();
  ASSERT_TRUE(record->BeginWrite());
  int released = 0, source;
  {
    StagedImageReadScope scope(runner, [](auto task) { task(); });
    scope.Hold(&source, state, [&] { ++released; });
    scope.Track(record);
  }
  ASSERT_EQ(runner->delays.size(), 1u);
  EXPECT_EQ(runner->delays.back(), 250);
  runner->Poll();
  EXPECT_EQ(released, 0);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  record->EndWrite(signal);
  record->Retire();
  runner->Poll();
  EXPECT_EQ(released, 0);
  signal->status = CompletionStatus::kComplete;
  runner->Poll();
  EXPECT_EQ(released, 0);
  EXPECT_TRUE(state->pending);
  for (auto delay : runner->delays) EXPECT_EQ(delay, 250);
  // Drop the fake runner's quarantined task without claiming GPU completion.
  runner->polls.clear();
}
TEST(InputReadScope, FailedQueryWaitsForSafetyProofWithoutBusyPolling) {
  class FailedQuery : public GPUCompletion {
   public:
    CompletionStatus Poll() const override { return CompletionStatus::kFailed; }
    bool IsSafeToRelease() const override { return safe; }
    bool safe = false;
  };
  auto runner = fml::MakeRefCounted<InputReadRecordingRunner>();
  auto state = std::make_shared<SourceReadState>();
  auto record = Record();
  auto signal = std::make_shared<FailedQuery>();
  ASSERT_TRUE(record->BeginWrite());
  record->EndWrite(signal);
  std::weak_ptr<ResourceRecord> storage_lifetime = record;
  int released = 0, source;
  {
    StagedImageReadScope scope(runner, [](auto task) { task(); });
    scope.Hold(&source, state, [&] { ++released; });
    scope.Track(record);
  }
  record.reset();
  ASSERT_EQ(runner->delays.size(), 1u);
  EXPECT_EQ(runner->delays.back(), 250);
  runner->Poll();
  EXPECT_EQ(released, 0);
  EXPECT_TRUE(state->pending);
  EXPECT_FALSE(storage_lifetime.expired());
  ASSERT_EQ(runner->delays.size(), 2u);
  EXPECT_EQ(runner->delays.back(), 250);
  // A backend may separately prove GPU termination without hiding the error.
  signal->safe = true;
  runner->Poll();
  EXPECT_EQ(signal->Poll(), CompletionStatus::kFailed);
  EXPECT_EQ(released, 1);
  EXPECT_FALSE(state->pending);
  EXPECT_TRUE(storage_lifetime.expired());
  EXPECT_TRUE(runner->polls.empty());
}
TEST(InputReadScope, OutputsWithoutSourceReadsDoNotScheduleADrain) {
  auto runner = fml::MakeRefCounted<ManualRunner>();
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto record = Record(signal);
  int contexts = 0;
  {
    StagedImageReadScope scope(runner, [&](auto task) {
      ++contexts;
      task();
    });
    scope.Track(record);
  }
  EXPECT_EQ(contexts, 0);
  EXPECT_TRUE(runner->polls.empty());
  EXPECT_TRUE(runner->tasks.empty());
  EXPECT_EQ(StagedImageReadScope::Current(), nullptr);
}
TEST(FrameMailbox, FailureUsesTheSameCoalescedWakeAndCanBeConsumedOnce) {
  FrameMailbox mailbox(Binding());
  EXPECT_TRUE(mailbox.ReportFailure(PipelineError::kProducerFailure));
  EXPECT_FALSE(mailbox.ReportFailure(PipelineError::kProducerFailure));
  EXPECT_EQ(mailbox.TakeFailure(), PipelineError::kProducerFailure);
  EXPECT_EQ(mailbox.TakeFailure(), PipelineError::kNone);
  mailbox.Close();
  EXPECT_FALSE(mailbox.ReportFailure(PipelineError::kGpuTimeout));
}
TEST(ResourceRecord, MissingCompletionQuarantinesTheAllocation) {
  auto record = Record();
  ASSERT_TRUE(record->BeginWrite());
  record->EndWrite(nullptr);
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
}
TEST(ResourceRecord, QueryFailureDoesNotProveGpuTermination) {
  class FailedQuery : public GPUCompletion {
   public:
    CompletionStatus Poll() const override { return CompletionStatus::kFailed; }
  };
  auto record = Record();
  ASSERT_TRUE(record->BeginWrite());
  record->EndWrite(std::make_shared<FailedQuery>());
  EXPECT_FALSE(record->CanReuse());
  EXPECT_FALSE(record->CanDestroy());
}
TEST_F(BindingTest, RebindStartsAfterOldReleaseWithoutWaitingForGpuDrain) {
  std::vector<int> order;
  std::function<void()> complete;
  auto first = group->RegisterRasterizer(
      1, 1,
      [&] {
        order.push_back(1);
        return true;
      },
      [](bool) {}, [](auto) {}, [](auto) {});
  worker->Run();
  first->Close(
      [&](auto done) {
        order.push_back(2);
        complete = std::move(done);
      },
      [] {});
  auto second = group->RegisterRasterizer(
      1, 2,
      [&] {
        order.push_back(3);
        return true;
      },
      [](bool) {}, [](auto) {}, [](auto) {});
  worker->Run();
  EXPECT_EQ(order, (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(second->state(), RasterBinding::State::kReady);
  EXPECT_EQ(first->state(), RasterBinding::State::kClosing);
  complete();
  platform->Run();
  EXPECT_EQ(first->state(), RasterBinding::State::kRetired);
}
TEST_F(BindingTest, ClosingSuppressesTheQueuedReadyCallback) {
  auto binding = Register();
  worker->Run();
  binding->Close([](auto done) { done(); }, [] {});
  platform->Run();
  EXPECT_EQ(ready_count, 0);
  worker->Run();
  platform->Run();
}
}  // namespace clay::display
