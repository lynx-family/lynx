// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/staged_image_read_scope.h"
#include "clay/common/raster_thread_group.h"
#include "clay/common/render_service_runtime.h"
#include "clay/flow/compositor/compositor_frame_builder.h"
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
TEST(ResourceRecord, PendingProducerCannotBeReadOrDestroyed) {
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  auto record = Record(signal);
  EXPECT_EQ(record->BeginRead(), 0u);
  EXPECT_FALSE(record->CanDestroy());
  signal->status = CompletionStatus::kComplete;
  EXPECT_TRUE(record->CanReuse());
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
TEST(FrameMailbox, UnfinishedReplacementKeepsReadyPendingFrame) {
  FrameMailbox mailbox(Binding());
  ASSERT_NE(mailbox.Publish(Publication(1)),
            FrameMailbox::PublishResult::kRejected);
  auto signal = std::make_shared<Signal>();
  signal->status = CompletionStatus::kPending;
  EXPECT_EQ(mailbox.Publish(Publication(2, 1, signal)),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_EQ(mailbox.TakeLatest().result->render_version, 1u);
  signal->status = CompletionStatus::kFailed;
  EXPECT_EQ(mailbox.Publish(Publication(2, 1, signal)),
            FrameMailbox::PublishResult::kRejected);
  signal->status = CompletionStatus::kComplete;
  EXPECT_NE(mailbox.Publish(Publication(2, 1, signal)),
            FrameMailbox::PublishResult::kRejected);
  EXPECT_EQ(mailbox.TakeLatest().result->render_version, 2u);
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
