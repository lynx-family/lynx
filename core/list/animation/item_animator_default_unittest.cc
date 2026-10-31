// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>

#define private public
#define protected public

#include "core/list/animation/item_animator_default.h"

#undef protected
#undef private

#include "core/base/threading/task_runner_manufactor.h"
#include "core/list/testing/mock_animation_target.h"
#include "core/list/testing/mock_vsync_monitor.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

constexpr int64_t kFirstVSyncTimeMs = 1000;
constexpr int32_t kAnimationDurationMs = 100;

void ExpectLayoutInfoEquals(const ItemLayoutInfo& actual,
                            const ItemLayoutInfo& expected) {
  EXPECT_FLOAT_EQ(actual.left_, expected.left_);
  EXPECT_FLOAT_EQ(actual.top_, expected.top_);
  EXPECT_FLOAT_EQ(actual.right_, expected.right_);
  EXPECT_FLOAT_EQ(actual.bottom_, expected.bottom_);
}

class RecordingItemAnimatorListener : public ItemAnimator::Listener {
 public:
  void OnAllAnimationsFinished() override { ++finished_count_; }

  int finished_count() const { return finished_count_; }

 private:
  int finished_count_{0};
};

// Overrides only the BasicAnimator factory so real animation code runs against
// a controllable VSyncMonitor without changing ItemAnimatorDefault's production
// constructor.
class TestItemAnimatorDefault final : public ItemAnimatorDefault {
 public:
  explicit TestItemAnimatorDefault(
      std::shared_ptr<base::VSyncMonitor> vsync_monitor)
      : vsync_monitor_(std::move(vsync_monitor)) {}

 private:
  std::shared_ptr<::lynx::animation::basic::LynxBasicAnimator>
  CreateBasicAnimator(starlight::AnimationData animation_data) override {
    return std::make_shared<::lynx::animation::basic::LynxBasicAnimator>(
        std::move(animation_data), vsync_monitor_);
  }

  std::shared_ptr<base::VSyncMonitor> vsync_monitor_;
};

class ItemAnimatorDefaultTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    // Initialize UIThread and its MessageLoop before binding VSync on Linux.
    base::UIThread::Init();
  }

  void SetUp() override {
    vsync_monitor_ = std::make_shared<MockVSyncMonitor>();
    vsync_monitor_->BindToCurrentThread();
    item_animator_ = std::make_unique<TestItemAnimatorDefault>(vsync_monitor_);
    item_animator_->SetListener(&listener_);
  }

  void SetAnimationDurations(int32_t add_duration_ms,
                             int32_t remove_duration_ms,
                             int32_t move_duration_ms) {
    item_animator_->SetAddDuration(add_duration_ms);
    item_animator_->SetRemoveDuration(remove_duration_ms);
    item_animator_->SetMoveDuration(move_duration_ms);
  }

  void EstablishAnimationClock() {
    // Start() first calls DoAnimationFrame(dummy_time) synchronously and, if
    // the animation remains active, requests VSync. The first non-dummy VSync
    // replaces the dummy start time, so it is sampled at elapsed=0. Later test
    // frames advance from this timestamp.
    vsync_monitor_->TriggerFrameAtMilliseconds(kFirstVSyncTimeMs);
  }

  void TriggerFrameAfter(int64_t elapsed_ms) {
    vsync_monitor_->TriggerFrameAtMilliseconds(kFirstVSyncTimeMs + elapsed_ms);
  }

  std::shared_ptr<MockVSyncMonitor> vsync_monitor_;
  RecordingItemAnimatorListener listener_;
  std::unique_ptr<ItemAnimatorDefault> item_animator_;
};

// Verifies input rejection for the currently supported queues and confirms
// that Change remains unsupported.
TEST_F(ItemAnimatorDefaultTest, RejectsNullUnchangedAndUnsupportedAnimations) {
  MockAnimationTarget target("item");
  target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);
  target.SetAnimationLayout(0, 100.f, 200.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. The implemented Remove, Add, and Move queueing paths reject null.
  EXPECT_FALSE(item_animator_->AnimateDisappearance(nullptr, pre_layout_info));
  EXPECT_FALSE(item_animator_->AnimateAppearance(nullptr, post_layout_info));
  EXPECT_FALSE(item_animator_->AnimatePersistence(nullptr, pre_layout_info,
                                                  post_layout_info));

  // 2. Equal PRE and POST positions do not create a Move animation.
  EXPECT_FALSE(item_animator_->AnimatePersistence(&target, pre_layout_info,
                                                  pre_layout_info));

  // 3. The unimplemented Change path explicitly returns false.
  EXPECT_FALSE(item_animator_->AnimateChange(&target, pre_layout_info,
                                             post_layout_info));
}

// Verifies that AnimateRemoveImpl stores its target and PRE snapshot in the
// pending Remove queue.
TEST_F(ItemAnimatorDefaultTest, StoresRemoveAnimationInPendingQueue) {
  MockAnimationTarget target("remove");
  target.SetAnimationLayout(1, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);

  // 1. Remove accepts a live target.
  ASSERT_TRUE(item_animator_->AnimateRemoveImpl(&target, pre_layout_info));

  // 2. The pending entry retains the same weak target and full PRE snapshot.
  ASSERT_EQ(item_animator_->pending_removals_.size(), 1u);
  const ItemAnimatorDefault::PendingAnimationInfo& pending_remove =
      item_animator_->pending_removals_.front();
  EXPECT_EQ(pending_remove.target.get(), &target);
  ExpectLayoutInfoEquals(pending_remove.pre_layout_info, pre_layout_info);

  // 3. No other pending queue is modified.
  EXPECT_TRUE(item_animator_->pending_adds_.empty());
  EXPECT_TRUE(item_animator_->pending_moves_.empty());
  EXPECT_TRUE(item_animator_->pending_changes_.empty());
}

// Verifies that AnimateAddImpl stores its target and POST snapshot in the
// pending Add queue.
TEST_F(ItemAnimatorDefaultTest, StoresAddAnimationInPendingQueue) {
  MockAnimationTarget target("add");
  target.SetAnimationLayout(2, 50.f, 60.f, 70.f, 80.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Add accepts a live target.
  ASSERT_TRUE(item_animator_->AnimateAddImpl(&target, post_layout_info));

  // 2. The pending entry retains the same weak target and full POST snapshot.
  ASSERT_EQ(item_animator_->pending_adds_.size(), 1u);
  const ItemAnimatorDefault::PendingAnimationInfo& pending_add =
      item_animator_->pending_adds_.front();
  EXPECT_EQ(pending_add.target.get(), &target);
  ExpectLayoutInfoEquals(pending_add.post_layout_info, post_layout_info);

  // 3. No other pending queue is modified.
  EXPECT_TRUE(item_animator_->pending_removals_.empty());
  EXPECT_TRUE(item_animator_->pending_moves_.empty());
  EXPECT_TRUE(item_animator_->pending_changes_.empty());
}

// Verifies that AnimateMoveImpl stores its target and PRE/POST snapshots in the
// pending Move queue.
TEST_F(ItemAnimatorDefaultTest, StoresMoveAnimationInPendingQueue) {
  MockAnimationTarget target("move");
  target.SetAnimationLayout(3, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);
  target.SetAnimationLayout(3, 110.f, 220.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Move accepts a live target whose position changed.
  ASSERT_TRUE(item_animator_->AnimateMoveImpl(&target, pre_layout_info,
                                              post_layout_info));

  // 2. The pending entry retains the same weak target and full snapshots.
  ASSERT_EQ(item_animator_->pending_moves_.size(), 1u);
  const ItemAnimatorDefault::PendingAnimationInfo& pending_move =
      item_animator_->pending_moves_.front();
  EXPECT_EQ(pending_move.target.get(), &target);
  ExpectLayoutInfoEquals(pending_move.pre_layout_info, pre_layout_info);
  ExpectLayoutInfoEquals(pending_move.post_layout_info, post_layout_info);

  // 3. No other pending queue is modified.
  EXPECT_TRUE(item_animator_->pending_removals_.empty());
  EXPECT_TRUE(item_animator_->pending_adds_.empty());
  EXPECT_TRUE(item_animator_->pending_changes_.empty());
}

// Verifies that the unimplemented AnimateChangeImpl rejects the request and
// leaves every pending queue unchanged.
TEST_F(ItemAnimatorDefaultTest, DoesNotQueueUnsupportedChangeAnimation) {
  MockAnimationTarget target("change");
  target.SetAnimationLayout(4, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);
  target.SetAnimationLayout(4, 50.f, 60.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // Change returns false and creates no pending animation.
  EXPECT_FALSE(item_animator_->AnimateChangeImpl(&target, pre_layout_info,
                                                 post_layout_info));
  EXPECT_TRUE(item_animator_->pending_changes_.empty());
  EXPECT_TRUE(item_animator_->pending_removals_.empty());
  EXPECT_TRUE(item_animator_->pending_adds_.empty());
  EXPECT_TRUE(item_animator_->pending_moves_.empty());
}

// Verifies that an empty pending set completes the batch synchronously.
TEST_F(ItemAnimatorDefaultTest, FinishesEmptyBatchSynchronously) {
  // Running an empty batch dispatches exactly one completion notification.
  item_animator_->RunPendingAnimations();
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies the running records created from a mixed pending batch.
TEST_F(ItemAnimatorDefaultTest,
       TracksMixedAnimationsAfterRunPendingAnimations) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);

  MockAnimationTarget remove_target("remove");
  remove_target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo remove_pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&remove_target);

  MockAnimationTarget add_target("add");
  add_target.SetAnimationLayout(1, 50.f, 60.f, 70.f, 80.f);
  ItemLayoutInfo add_post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&add_target);

  MockAnimationTarget move_target("move");
  move_target.SetAnimationLayout(2, 100.f, 200.f, 30.f, 40.f);
  ItemLayoutInfo move_pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&move_target);
  move_target.SetAnimationLayout(2, 300.f, 400.f, 30.f, 40.f);
  ItemLayoutInfo move_post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&move_target);

  // 1. Queue one Remove, Add, and Move animation.
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&remove_target,
                                                   remove_pre_layout_info));
  ASSERT_TRUE(
      item_animator_->AnimateAppearance(&add_target, add_post_layout_info));
  ASSERT_TRUE(item_animator_->AnimatePersistence(
      &move_target, move_pre_layout_info, move_post_layout_info));

  // 2. Start the batch without manually delivering a non-dummy VSync.
  item_animator_->RunPendingAnimations();

  // All pending entries were consumed and now have running records.
  EXPECT_TRUE(item_animator_->pending_removals_.empty());
  EXPECT_TRUE(item_animator_->pending_adds_.empty());
  EXPECT_TRUE(item_animator_->pending_moves_.empty());
  EXPECT_TRUE(item_animator_->pending_changes_.empty());
  EXPECT_TRUE(item_animator_->has_running_animations());
  ASSERT_EQ(item_animator_->running_animations_.size(), 3u);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 3. The Remove record contains its type, target, animator, and PRE snapshot.
  auto remove_iter = item_animator_->running_animations_.find(
      reinterpret_cast<AnimationTargetKey>(&remove_target));
  ASSERT_NE(remove_iter, item_animator_->running_animations_.end());
  EXPECT_EQ(remove_iter->second.type, ItemAnimationType::kDisappearance);
  EXPECT_EQ(remove_iter->second.target.get(), &remove_target);
  EXPECT_NE(remove_iter->second.animator, nullptr);
  ExpectLayoutInfoEquals(remove_iter->second.pre_layout_info,
                         remove_pre_layout_info);

  // 4. The Add record contains its type, target, animator, and POST snapshot.
  auto add_iter = item_animator_->running_animations_.find(
      reinterpret_cast<AnimationTargetKey>(&add_target));
  ASSERT_NE(add_iter, item_animator_->running_animations_.end());
  EXPECT_EQ(add_iter->second.type, ItemAnimationType::kAppearance);
  EXPECT_EQ(add_iter->second.target.get(), &add_target);
  EXPECT_NE(add_iter->second.animator, nullptr);
  ExpectLayoutInfoEquals(add_iter->second.post_layout_info,
                         add_post_layout_info);

  // 5. The Move record contains its type, target, animator, and both snapshots.
  auto move_iter = item_animator_->running_animations_.find(
      reinterpret_cast<AnimationTargetKey>(&move_target));
  ASSERT_NE(move_iter, item_animator_->running_animations_.end());
  EXPECT_EQ(move_iter->second.type, ItemAnimationType::kPersistence);
  EXPECT_EQ(move_iter->second.target.get(), &move_target);
  EXPECT_NE(move_iter->second.animator, nullptr);
  ExpectLayoutInfoEquals(move_iter->second.pre_layout_info,
                         move_pre_layout_info);
  ExpectLayoutInfoEquals(move_iter->second.post_layout_info,
                         move_post_layout_info);
}

// Verifies that Appearance interpolates opacity from 0 to 1.
TEST_F(ItemAnimatorDefaultTest, RunsAppearanceAnimationFrames) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("add");
  target.SetAnimationLayout(0, 100.f, 200.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Put the appearing target into its animation lifecycle and queue Add.
  target.PrepareForAnimation(ItemAnimationType::kAppearance);
  ASSERT_TRUE(item_animator_->AnimateAppearance(&target, post_layout_info));

  item_animator_->RunPendingAnimations();
  // This batch contains only Add, so add_delay is zero. RunPendingAnimations()
  // writes opacity=0 before Start(); Start() then processes a dummy frame whose
  // custom callback writes opacity=0 again.
  ASSERT_EQ(target.opacity_updates().size(), 2u);
  // Both writes occur during startup and are left for the caller's batch flush.
  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.f);
  EXPECT_FALSE(target.opacity_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 0);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 2. Establish the VSync time origin, then sample the symmetric curve at
  // half duration.
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);

  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.5f);
  EXPECT_TRUE(target.opacity_updates().back().flush_immediately);
  // The target's running record remains until the animation ends.
  EXPECT_EQ(item_animator_->running_animations_.size(), 1u);

  // 3. The six opacity writes are, in order:
  // 1) StartAddAnimation writes the initial opacity=0;
  // 2) the dummy-frame custom callback writes opacity=0;
  // 3) EstablishAnimationClock's first non-dummy VSync writes opacity=0;
  // 4) the midpoint custom callback writes opacity=0.5;
  // 5) the final custom callback writes opacity=1; and
  // 6) the End callback writes opacity=1 through ResetTargetToFinalState.
  // The Start callback itself does not update opacity.
  TriggerFrameAfter(kAnimationDurationMs);

  // The End callback removes the running record before finishing the target.
  EXPECT_TRUE(item_animator_->running_animations_.empty());
  ASSERT_EQ(target.opacity_updates().size(), 6u);
  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 1.f);
  EXPECT_TRUE(target.opacity_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies that Disappearance interpolates opacity from 1 to 0.
TEST_F(ItemAnimatorDefaultTest, RunsDisappearanceAnimationFrames) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("remove");
  target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);

  // 1. Put the disappearing target into its animation lifecycle and queue
  // Remove.
  target.PrepareForAnimation(ItemAnimationType::kDisappearance);
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&target, pre_layout_info));

  item_animator_->RunPendingAnimations();
  // Remove has zero delay. RunPendingAnimations() writes opacity=1 before
  // Start(); Start() then processes a dummy frame whose custom callback writes
  // opacity=1 again.
  ASSERT_EQ(target.opacity_updates().size(), 2u);
  // Both writes occur during startup and are left for the caller's batch flush.
  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 1.f);
  EXPECT_FALSE(target.opacity_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 0);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 2. Sample the symmetric curve at half duration.
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);

  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.5f);
  EXPECT_TRUE(target.opacity_updates().back().flush_immediately);
  // The target's running record remains until the animation ends.
  EXPECT_EQ(item_animator_->running_animations_.size(), 1u);

  // 3. The six opacity writes are, in order:
  // 1) StartRemoveAnimation writes the initial opacity=1;
  // 2) the dummy-frame custom callback writes opacity=1;
  // 3) EstablishAnimationClock's first non-dummy VSync writes opacity=1;
  // 4) the midpoint custom callback writes opacity=0.5;
  // 5) the final custom callback writes opacity=0; and
  // 6) the End callback writes opacity=0 through ResetTargetToFinalState.
  // The Start callback itself does not update opacity.
  TriggerFrameAfter(kAnimationDurationMs);

  // The End callback removes the running record before finishing the target.
  EXPECT_TRUE(item_animator_->running_animations_.empty());
  ASSERT_EQ(target.opacity_updates().size(), 6u);
  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.f);
  EXPECT_TRUE(target.opacity_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies that Persistence interpolates presentation position from PRE to
// POST without changing the target's logical position.
TEST_F(ItemAnimatorDefaultTest, RunsPersistenceAnimationFrames) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("move");

  target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);
  target.SetAnimationLayout(1, 110.f, 220.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Queue Move with the target's PRE and POST snapshots.
  target.PrepareForAnimation(ItemAnimationType::kPersistence);
  ASSERT_TRUE(item_animator_->AnimatePersistence(&target, pre_layout_info,
                                                 post_layout_info));

  item_animator_->RunPendingAnimations();
  // This batch has no Remove, so the Move delay is zero.
  // RunPendingAnimations() writes PRE before Start(); the dummy-frame custom
  // callback then writes PRE again.
  ASSERT_EQ(target.position_updates().size(), 2u);
  // Both writes occur during startup and do not flush individually. The
  // target's logical position remains at POST.
  EXPECT_FLOAT_EQ(target.position_updates().back().left, 10.f);
  EXPECT_FLOAT_EQ(target.position_updates().back().top, 20.f);
  EXPECT_FALSE(target.position_updates().back().flush_immediately);
  EXPECT_FLOAT_EQ(target.GetAnimationLeft(), 110.f);
  EXPECT_FLOAT_EQ(target.GetAnimationTop(), 220.f);
  EXPECT_EQ(target.finish_animation_count(), 0);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 2. Establish the VSync time origin, then sample the presentation position
  // halfway between PRE and POST.
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);

  EXPECT_FLOAT_EQ(target.position_updates().back().left, 60.f);
  EXPECT_FLOAT_EQ(target.position_updates().back().top, 120.f);
  EXPECT_TRUE(target.position_updates().back().flush_immediately);
  // The target's running record remains until the animation ends.
  EXPECT_EQ(item_animator_->running_animations_.size(), 1u);

  // 3. The six position writes are, in order:
  // 1) StartMoveAnimation writes PRE;
  // 2) the dummy-frame custom callback writes PRE;
  // 3) EstablishAnimationClock's first non-dummy VSync writes PRE;
  // 4) the midpoint custom callback writes halfway between PRE and POST;
  // 5) the final custom callback writes POST; and
  // 6) the End callback writes the latest logical position through
  //    ResetTargetToFinalState.
  // The Start callback itself does not update position.
  TriggerFrameAfter(kAnimationDurationMs);

  // The End callback removes the running record before finishing the target.
  EXPECT_TRUE(item_animator_->running_animations_.empty());
  ASSERT_EQ(target.position_updates().size(), 6u);
  EXPECT_FLOAT_EQ(target.position_updates().back().left, 110.f);
  EXPECT_FLOAT_EQ(target.position_updates().back().top, 220.f);
  EXPECT_TRUE(target.position_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies that delays arrange a mixed batch's effective intervals as
// Remove -> Move -> Add.
TEST_F(ItemAnimatorDefaultTest, RunsRemoveMoveAndAddInOrder) {
  constexpr int32_t kRemoveDurationMs = 100;
  constexpr int32_t kMoveDurationMs = 200;
  constexpr int32_t kAddDurationMs = 100;
  SetAnimationDurations(kAddDurationMs, kRemoveDurationMs, kMoveDurationMs);

  MockAnimationTarget remove_target("remove");
  remove_target.SetAnimationLayout(0, 0.f, 0.f, 10.f, 10.f);
  ItemLayoutInfo remove_pre =
      item_animator_->GetPreItemLayoutInfo(&remove_target);

  MockAnimationTarget move_target("move");
  move_target.SetAnimationLayout(1, 0.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo move_pre = item_animator_->GetPreItemLayoutInfo(&move_target);
  move_target.SetAnimationLayout(1, 100.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo move_post =
      item_animator_->GetPostItemLayoutInfo(&move_target);

  MockAnimationTarget add_target("add");
  add_target.SetAnimationLayout(2, 0.f, 40.f, 10.f, 10.f);
  ItemLayoutInfo add_post = item_animator_->GetPostItemLayoutInfo(&add_target);

  // 1. Pending insertion order does not affect computed delays or intervals.
  add_target.PrepareForAnimation(ItemAnimationType::kAppearance);
  move_target.PrepareForAnimation(ItemAnimationType::kPersistence);
  remove_target.PrepareForAnimation(ItemAnimationType::kDisappearance);
  ASSERT_TRUE(item_animator_->AnimateAppearance(&add_target, add_post));
  ASSERT_TRUE(
      item_animator_->AnimatePersistence(&move_target, move_pre, move_post));
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&remove_target, remove_pre));

  // Start the batch.
  item_animator_->RunPendingAnimations();

  // 2. RunPendingAnimations creates all three running records in one pass.
  // Move and Add remain running records whose progress is gated by delay.
  ASSERT_EQ(item_animator_->running_animations_.size(), 3u);
  auto remove_iter = item_animator_->running_animations_.find(
      reinterpret_cast<AnimationTargetKey>(&remove_target));
  ASSERT_NE(remove_iter, item_animator_->running_animations_.end());
  EXPECT_EQ(remove_iter->second.type, ItemAnimationType::kDisappearance);

  auto move_iter = item_animator_->running_animations_.find(
      reinterpret_cast<AnimationTargetKey>(&move_target));
  ASSERT_NE(move_iter, item_animator_->running_animations_.end());
  EXPECT_EQ(move_iter->second.type, ItemAnimationType::kPersistence);

  auto add_iter = item_animator_->running_animations_.find(
      reinterpret_cast<AnimationTargetKey>(&add_target));
  ASSERT_NE(add_iter, item_animator_->running_animations_.end());
  EXPECT_EQ(add_iter->second.type, ItemAnimationType::kAppearance);

  EstablishAnimationClock();

  // Remove has no delay, so the first non-dummy VSync produces its third
  // update. Move and Add are still delayed, and their fill mode does not apply
  // backwards. Neither their dummy nor non-dummy pre-delay samples write a
  // value, leaving only the explicit StartXxxAnimation initialization.
  ASSERT_EQ(remove_target.opacity_updates().size(), 3u);
  ASSERT_EQ(move_target.position_updates().size(), 1u);
  ASSERT_EQ(add_target.opacity_updates().size(), 1u);

  // 3. While Remove progresses, Move and Add retain their initial presentation.
  TriggerFrameAfter(50);
  EXPECT_EQ(remove_target.opacity_updates().size(), 4u);
  EXPECT_EQ(move_target.position_updates().size(), 1u);
  EXPECT_EQ(add_target.opacity_updates().size(), 1u);
  EXPECT_EQ(remove_target.finish_animation_count(), 0);

  // 4. After Remove ends, Move progresses while Add remains delayed.
  TriggerFrameAfter(150);
  // Remove's final sample and final-state write bring it to six updates; Move's
  // first active sample brings it to two.
  EXPECT_EQ(remove_target.opacity_updates().size(), 6u);
  EXPECT_EQ(move_target.position_updates().size(), 2u);
  EXPECT_EQ(add_target.opacity_updates().size(), 1u);
  EXPECT_EQ(remove_target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 5. After Move ends, Add progresses and keeps the batch active.
  TriggerFrameAfter(350);
  // Move's final sample and final-state write bring it to four updates; Add's
  // first active sample brings it to two.
  EXPECT_EQ(move_target.position_updates().size(), 4u);
  EXPECT_EQ(add_target.opacity_updates().size(), 2u);
  EXPECT_EQ(move_target.finish_animation_count(), 1);
  EXPECT_EQ(add_target.finish_animation_count(), 0);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 6. The batch completes only after Add reaches its endpoint.
  TriggerFrameAfter(400);
  // Add's final sample and final-state write bring it to four updates.
  EXPECT_EQ(add_target.opacity_updates().size(), 4u);
  EXPECT_EQ(add_target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies that a zero-duration mixed batch completes synchronously and emits
// one batch-completion notification.
TEST_F(ItemAnimatorDefaultTest, CompletesZeroDurationBatchOnce) {
  SetAnimationDurations(0, 0, 0);

  MockAnimationTarget remove_target("remove");
  remove_target.SetAnimationLayout(0, 0.f, 0.f, 10.f, 10.f);
  ItemLayoutInfo remove_pre =
      item_animator_->GetPreItemLayoutInfo(&remove_target);

  MockAnimationTarget move_target("move");
  move_target.SetAnimationLayout(1, 0.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo move_pre = item_animator_->GetPreItemLayoutInfo(&move_target);
  move_target.SetAnimationLayout(1, 100.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo move_post =
      item_animator_->GetPostItemLayoutInfo(&move_target);

  MockAnimationTarget add_target("add");
  add_target.SetAnimationLayout(2, 0.f, 40.f, 10.f, 10.f);
  ItemLayoutInfo add_post = item_animator_->GetPostItemLayoutInfo(&add_target);

  // 1. Queue all three animation types with zero duration.
  remove_target.PrepareForAnimation(ItemAnimationType::kDisappearance);
  move_target.PrepareForAnimation(ItemAnimationType::kPersistence);
  add_target.PrepareForAnimation(ItemAnimationType::kAppearance);
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&remove_target, remove_pre));
  ASSERT_TRUE(
      item_animator_->AnimatePersistence(&move_target, move_pre, move_post));
  ASSERT_TRUE(item_animator_->AnimateAppearance(&add_target, add_post));

  // 2. RunPendingAnimations starts and finishes every BasicAnimator inline.
  item_animator_->RunPendingAnimations();

  EXPECT_EQ(remove_target.finish_animation_count(), 1);
  EXPECT_EQ(move_target.finish_animation_count(), 1);
  EXPECT_EQ(add_target.finish_animation_count(), 1);

  // 3. Each target reaches its final presentation without an individual flush.
  ASSERT_FALSE(remove_target.opacity_updates().empty());
  ASSERT_FALSE(move_target.position_updates().empty());
  ASSERT_FALSE(add_target.opacity_updates().empty());
  EXPECT_FLOAT_EQ(remove_target.opacity_updates().back().opacity, 0.f);
  EXPECT_FLOAT_EQ(move_target.position_updates().back().left, 100.f);
  EXPECT_FLOAT_EQ(move_target.position_updates().back().top, 20.f);
  EXPECT_FLOAT_EQ(add_target.opacity_updates().back().opacity, 1.f);
  // Zero duration completes during RunPendingAnimations startup. Because
  // in_starting_animations_ is still true, neither the sampled value nor the
  // explicit final-state write flushes immediately; the caller flushes the
  // accumulated updates.
  EXPECT_FALSE(remove_target.opacity_updates().back().flush_immediately);
  EXPECT_FALSE(move_target.position_updates().back().flush_immediately);
  EXPECT_FALSE(add_target.opacity_updates().back().flush_immediately);
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies that cancelling pending animations ends target lifecycles without
// writing presentation values.
TEST_F(ItemAnimatorDefaultTest, CancelsPendingAnimations) {
  MockAnimationTarget remove_target("remove");
  remove_target.SetAnimationLayout(0, 0.f, 0.f, 10.f, 10.f);
  ItemLayoutInfo remove_pre =
      item_animator_->GetPreItemLayoutInfo(&remove_target);

  MockAnimationTarget move_target("move");
  move_target.SetAnimationLayout(1, 0.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo move_pre = item_animator_->GetPreItemLayoutInfo(&move_target);
  move_target.SetAnimationLayout(1, 100.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo move_post =
      item_animator_->GetPostItemLayoutInfo(&move_target);

  MockAnimationTarget add_target("add");
  add_target.SetAnimationLayout(2, 0.f, 40.f, 10.f, 10.f);
  ItemLayoutInfo add_post = item_animator_->GetPostItemLayoutInfo(&add_target);

  // 1. Prepare each target and queue it without running the batch.
  remove_target.PrepareForAnimation(ItemAnimationType::kDisappearance);
  move_target.PrepareForAnimation(ItemAnimationType::kPersistence);
  add_target.PrepareForAnimation(ItemAnimationType::kAppearance);
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&remove_target, remove_pre));
  ASSERT_TRUE(
      item_animator_->AnimatePersistence(&move_target, move_pre, move_post));
  ASSERT_TRUE(item_animator_->AnimateAppearance(&add_target, add_post));

  // 2. Cancelling the pending queues finishes each target exactly once.
  item_animator_->CancelAnimations(false);

  EXPECT_EQ(remove_target.finish_animation_count(), 1);
  EXPECT_EQ(move_target.finish_animation_count(), 1);
  EXPECT_EQ(add_target.finish_animation_count(), 1);

  // 3. No presentation value or normal completion is emitted for animations
  // that never started.
  EXPECT_TRUE(remove_target.opacity_updates().empty());
  EXPECT_TRUE(move_target.position_updates().empty());
  EXPECT_TRUE(add_target.opacity_updates().empty());
  EXPECT_EQ(listener_.finished_count(), 0);
}

// Verifies that a batch completes when its pending target has already expired.
TEST_F(ItemAnimatorDefaultTest, FinishesBatchWhenPendingTargetWasDestroyed) {
  auto target = std::make_unique<MockAnimationTarget>("add");
  target->SetAnimationLayout(0, 100.f, 200.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(target.get());

  // 1. Prepare the target and queue Add.
  target->PrepareForAnimation(ItemAnimationType::kAppearance);
  ASSERT_TRUE(
      item_animator_->AnimateAppearance(target.get(), post_layout_info));

  // 2. Destroy the target before Run so its pending weak pointer expires.
  target.reset();

  // 3. Startup skips the expired target and completes the batch synchronously.
  item_animator_->RunPendingAnimations();
  EXPECT_EQ(listener_.finished_count(), 1);
}

// Verifies that normal cancellation restores a running animation's final state
// synchronously without dispatching normal batch completion.
TEST_F(ItemAnimatorDefaultTest, CancelsRunningAnimationAndRestoresFinalState) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("add");
  target.SetAnimationLayout(0, 100.f, 200.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Start Add and advance it to the midpoint.
  target.PrepareForAnimation(ItemAnimationType::kAppearance);
  ASSERT_TRUE(item_animator_->AnimateAppearance(&target, post_layout_info));
  item_animator_->RunPendingAnimations();
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);
  ASSERT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.5f);

  // 2. Normal cancellation destroys BasicAnimator and restores opacity=1.
  item_animator_->CancelAnimations(false);

  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 1.f);
  EXPECT_FALSE(target.opacity_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 1);

  // CancelAnimations(false)
  //   ├─ in_cancelling_animations_ = true
  //   ├─ DestroyAnimation()
  //   │  └─ Cancel callback
  //   │     └─ FinishRunningAnimation()
  //   │        └─ DispatchAnimationFinishedIfNeeded()
  //   |           `- in_cancelling_animations_ is true, so return
  //   └─ in_cancelling_animations_ = false
  // Explicit cancellation therefore does not notify the Listener of normal
  // batch completion.
  EXPECT_EQ(listener_.finished_count(), 0);
  EXPECT_FALSE(item_animator_->HasInvalidatedRunningAnimationLayout());

  // 3. A previously scheduled VSync cannot update the target after
  // cancellation.
  const std::size_t update_count_after_cancel = target.opacity_updates().size();
  TriggerFrameAfter(kAnimationDurationMs);
  EXPECT_EQ(target.opacity_updates().size(), update_count_after_cancel);
}

// Verifies that normal cancellation restores a running Remove to opacity=0.
TEST_F(ItemAnimatorDefaultTest, CancelsRunningRemoveToTransparentFinalState) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("remove");
  target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);

  // 1. Start Remove and advance it to the midpoint.
  target.PrepareForAnimation(ItemAnimationType::kDisappearance);
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&target, pre_layout_info));
  item_animator_->RunPendingAnimations();
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);
  ASSERT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.5f);

  // 2. Normal cancellation destroys BasicAnimator and restores opacity=0.
  item_animator_->CancelAnimations(false);

  EXPECT_FLOAT_EQ(target.opacity_updates().back().opacity, 0.f);
  EXPECT_FALSE(target.opacity_updates().back().flush_immediately);

  // 3. The target finishes once and normal batch completion is not dispatched.
  EXPECT_EQ(target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 0);
}

// Verifies that normal cancellation restores a running Move to the target's
// latest logical position.
TEST_F(ItemAnimatorDefaultTest, CancelsRunningMoveToLatestLogicalPosition) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("move");
  target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);
  target.SetAnimationLayout(0, 110.f, 220.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Start Move and advance its presentation halfway from PRE to POST.
  target.PrepareForAnimation(ItemAnimationType::kPersistence);
  ASSERT_TRUE(item_animator_->AnimatePersistence(&target, pre_layout_info,
                                                 post_layout_info));
  item_animator_->RunPendingAnimations();
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);
  ASSERT_FLOAT_EQ(target.position_updates().back().left, 60.f);
  ASSERT_FLOAT_EQ(target.position_updates().back().top, 120.f);

  // 2. Relayout changes the target's logical endpoint while Move is running.
  target.SetAnimationLayout(0, 160.f, 260.f, 30.f, 40.f);
  ASSERT_TRUE(item_animator_->HasInvalidatedRunningAnimationLayout());

  // 3. Normal cancellation restores that endpoint and finishes the target.
  item_animator_->CancelAnimations(false);

  EXPECT_FLOAT_EQ(target.position_updates().back().left, 160.f);
  EXPECT_FLOAT_EQ(target.position_updates().back().top, 260.f);
  EXPECT_FALSE(target.position_updates().back().flush_immediately);
  EXPECT_EQ(target.finish_animation_count(), 1);
  EXPECT_EQ(listener_.finished_count(), 0);
  EXPECT_FALSE(item_animator_->HasInvalidatedRunningAnimationLayout());
}

// Verifies that teardown cancellation does not access running targets.
TEST_F(ItemAnimatorDefaultTest, DestroyCancellationSkipsRunningTargetCleanup) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  MockAnimationTarget target("move");
  target.SetAnimationLayout(0, 10.f, 20.f, 30.f, 40.f);
  ItemLayoutInfo pre_layout_info =
      item_animator_->GetPreItemLayoutInfo(&target);
  target.SetAnimationLayout(0, 110.f, 220.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(&target);

  // 1. Start Move, advance to the midpoint, and record its update count.
  target.PrepareForAnimation(ItemAnimationType::kPersistence);
  ASSERT_TRUE(item_animator_->AnimatePersistence(&target, pre_layout_info,
                                                 post_layout_info));
  item_animator_->RunPendingAnimations();
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs / 2);
  const std::size_t update_count_before_destroy =
      target.position_updates().size();

  // 2. destroy=true replaces callbacks with no-ops before destroying the
  // animators and clearing running records.
  item_animator_->CancelAnimations(true);

  // Teardown neither restores presentation state nor calls FinishAnimation()
  // on a running target, which may no longer be safe to access.
  EXPECT_EQ(target.position_updates().size(), update_count_before_destroy);
  EXPECT_EQ(target.finish_animation_count(), 0);
  EXPECT_EQ(listener_.finished_count(), 0);

  // 3. Changing the endpoint cannot report invalidation after records clear.
  target.SetAnimationLayout(0, 160.f, 260.f, 30.f, 40.f);
  EXPECT_FALSE(item_animator_->HasInvalidatedRunningAnimationLayout());

  // A previously scheduled VSync also cannot update the target.
  TriggerFrameAfter(kAnimationDurationMs);
  EXPECT_EQ(target.position_updates().size(), update_count_before_destroy);
}

// Verifies that relayout invalidation considers only the position endpoint of
// a running Move animation.
TEST_F(ItemAnimatorDefaultTest, DetectsOnlyInvalidatedMoveEndpoint) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);

  MockAnimationTarget remove_target("remove");
  remove_target.SetAnimationLayout(0, 0.f, 0.f, 10.f, 10.f);
  ItemLayoutInfo remove_pre =
      item_animator_->GetPreItemLayoutInfo(&remove_target);

  MockAnimationTarget add_target("add");
  add_target.SetAnimationLayout(1, 0.f, 20.f, 10.f, 10.f);
  ItemLayoutInfo add_post = item_animator_->GetPostItemLayoutInfo(&add_target);

  MockAnimationTarget move_target("move");
  move_target.SetAnimationLayout(2, 0.f, 40.f, 10.f, 10.f);
  ItemLayoutInfo move_pre = item_animator_->GetPreItemLayoutInfo(&move_target);
  move_target.SetAnimationLayout(2, 100.f, 40.f, 10.f, 10.f);
  ItemLayoutInfo move_post =
      item_animator_->GetPostItemLayoutInfo(&move_target);

  // 1. Start Remove, Add, and Move; Move records its original POST endpoint.
  remove_target.PrepareForAnimation(ItemAnimationType::kDisappearance);
  add_target.PrepareForAnimation(ItemAnimationType::kAppearance);
  move_target.PrepareForAnimation(ItemAnimationType::kPersistence);
  ASSERT_TRUE(item_animator_->AnimateDisappearance(&remove_target, remove_pre));
  ASSERT_TRUE(item_animator_->AnimateAppearance(&add_target, add_post));
  ASSERT_TRUE(
      item_animator_->AnimatePersistence(&move_target, move_pre, move_post));

  item_animator_->RunPendingAnimations();

  // 2. Ignore Add/Remove layout changes and Move size-only changes. Only a
  // running Move's left/top endpoint can invalidate the recorded animation.
  remove_target.SetAnimationLayout(0, 50.f, 50.f, 10.f, 10.f);
  add_target.SetAnimationLayout(1, 60.f, 60.f, 10.f, 10.f);
  move_target.SetAnimationLayout(2, 100.f, 40.f, 20.f, 30.f);
  EXPECT_FALSE(item_animator_->HasInvalidatedRunningAnimationLayout());

  // 3. Remove occupies the first 100 ms, so at 150 ms Move is halfway from
  // PRE to POST.
  EstablishAnimationClock();
  TriggerFrameAfter(kAnimationDurationMs + kAnimationDurationMs / 2);
  ASSERT_FLOAT_EQ(move_target.position_updates().back().left, 50.f);
  ASSERT_FLOAT_EQ(move_target.position_updates().back().top, 40.f);
  EXPECT_FALSE(item_animator_->HasInvalidatedRunningAnimationLayout());

  // 4. Relayout changes the logical endpoint while the running animation still
  // interpolates toward its recorded POST, so invalidation is reported.
  move_target.SetAnimationLayout(2, 120.f, 80.f, 20.f, 30.f);
  ASSERT_TRUE(item_animator_->HasInvalidatedRunningAnimationLayout());
  EXPECT_FLOAT_EQ(move_target.position_updates().back().left, 50.f);
  EXPECT_FLOAT_EQ(move_target.position_updates().back().top, 40.f);

  // 5. Normal cancellation restores the latest logical endpoint and finishes
  // the target's animation lifecycle.
  const std::size_t move_update_count_before_cancel =
      move_target.position_updates().size();
  item_animator_->CancelAnimations(false);

  EXPECT_EQ(move_target.position_updates().size(),
            move_update_count_before_cancel + 1);
  EXPECT_FLOAT_EQ(move_target.position_updates().back().left, 120.f);
  EXPECT_FLOAT_EQ(move_target.position_updates().back().top, 80.f);
  EXPECT_FALSE(move_target.position_updates().back().flush_immediately);
  EXPECT_EQ(move_target.finish_animation_count(), 1);
  EXPECT_TRUE(item_animator_->running_animations_.empty());
  EXPECT_FALSE(item_animator_->HasInvalidatedRunningAnimationLayout());
  EXPECT_EQ(listener_.finished_count(), 0);
}

// Verifies that an expired running target does not prevent batch completion.
TEST_F(ItemAnimatorDefaultTest, FinishesBatchAfterRunningTargetIsDestroyed) {
  SetAnimationDurations(kAnimationDurationMs, kAnimationDurationMs,
                        kAnimationDurationMs);
  auto target = std::make_unique<MockAnimationTarget>("add");
  target->SetAnimationLayout(0, 100.f, 200.f, 30.f, 40.f);
  ItemLayoutInfo post_layout_info =
      item_animator_->GetPostItemLayoutInfo(target.get());

  // 1. Start the animation and establish its origin with the first non-dummy
  // VSync.
  target->PrepareForAnimation(ItemAnimationType::kAppearance);
  ASSERT_TRUE(
      item_animator_->AnimateAppearance(target.get(), post_layout_info));
  item_animator_->RunPendingAnimations();
  EstablishAnimationClock();

  // 2. Destroy the target while its running record still holds a weak pointer.
  target.reset();

  // 3. BasicAnimator still removes the record and completes the batch at End.
  TriggerFrameAfter(kAnimationDurationMs);
  EXPECT_EQ(listener_.finished_count(), 1);
}

}  // namespace
}  // namespace list
}  // namespace lynx
