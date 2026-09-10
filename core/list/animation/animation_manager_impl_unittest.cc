// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/list/animation/animation_manager_impl.h"

#undef protected
#undef private

#include <memory>
#include <utility>
#include <vector>

#include "core/list/decoupled_list_container_impl.h"
#include "core/list/testing/mock_animation_target.h"
#include "core/list/testing/mock_item_animator.h"
#include "core/list/testing/mock_list_element.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace list {
namespace {

constexpr TransactionId kInjectedTransactionId = 100;

class AnimationManagerImplTest : public ::testing::Test {
 public:
  AnimationManagerImplTest() = default;
  ~AnimationManagerImplTest() override = default;

  void SetUp() override {
    value_factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    mock_list_element_ = std::make_unique<MockListElement>();
    list_container_ = std::make_unique<ListContainerImpl>(
        mock_list_element_.get(), value_factory_);
    animation_manager_ = static_cast<AnimationManagerImpl*>(
        list_container_->animation_manager());
  }

  void TearDown() override {
    // Clean up any transaction injected by the test so fixture destruction is
    // deterministic. A second Destroy() call from the ListContainer destructor
    // must remain idempotent.
    animation_manager_->Destroy();
  }

  MockItemAnimator* CreateActiveTransaction(
      TransactionState state, std::vector<WeakAnimationTarget> pre_targets = {},
      std::shared_ptr<MockItemAnimatorObserver> lifecycle_observer = nullptr) {
    auto item_animator =
        std::make_unique<MockItemAnimator>(std::move(lifecycle_observer));
    MockItemAnimator* item_animator_ptr = item_animator.get();
    item_animator->SetListener(animation_manager_);

    auto transaction = std::make_unique<AnimationTransaction>(
        kInjectedTransactionId, std::move(pre_targets),
        std::move(item_animator));
    transaction->SetState(state);
    animation_manager_->active_transaction_ = std::move(transaction);
    return item_animator_ptr;
  }

 protected:
  std::shared_ptr<pub::PubValueFactoryDefault> value_factory_;
  std::unique_ptr<MockListElement> mock_list_element_;
  std::unique_ptr<ListContainerImpl> list_container_;
  AnimationManagerImpl* animation_manager_{nullptr};
};

// Verifies that the manager creates a transaction only when animations are
// enabled, the diff is valid and expected to animate, and at least one layout
// has completed.
TEST_F(AnimationManagerImplTest, CreatesTransactionOnlyWhenAnimationCanRun) {
  // 1. With animations disabled, valid diff and layout conditions do not create
  // a transaction.
  animation_manager_->BeforeDataUpdate(true, true, true);
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);

  UpdateAnimationConfig config;
  config.enable = true;
  animation_manager_->SetUpdateAnimationConfig(config);

  // 2. An invalid diff does not create an animation transaction.
  animation_manager_->BeforeDataUpdate(false, true, true);
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);

  // 3. No transaction is created when the diff is not expected to animate or
  // the first layout has not completed.
  animation_manager_->BeforeDataUpdate(true, false, true);
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  animation_manager_->BeforeDataUpdate(true, true, false);
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);

  // 4. Once every condition is met, the transaction starts in the PRE target
  // snapshot state.
  animation_manager_->BeforeDataUpdate(true, true, true);
  ASSERT_NE(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(animation_manager_->active_transaction_->id(), 1u);
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kHasPreChildrenSnapshot);
}

// Verifies that consecutive data updates before BeforeLayout() reuse the same
// active transaction.
TEST_F(AnimationManagerImplTest, KeepsFirstTransactionBeforeLayout) {
  UpdateAnimationConfig config;
  config.enable = true;
  animation_manager_->SetUpdateAnimationConfig(config);

  // 1. The first data update creates a transaction.
  animation_manager_->BeforeDataUpdate(true, true, true);
  AnimationTransaction* first_transaction =
      animation_manager_->active_transaction_.get();
  ASSERT_NE(first_transaction, nullptr);

  // 2. A second update before layout preserves both the transaction address and
  // its ID.
  animation_manager_->BeforeDataUpdate(true, true, true);
  EXPECT_EQ(animation_manager_->active_transaction_.get(), first_transaction);
  EXPECT_EQ(animation_manager_->active_transaction_->id(), 1u);
  EXPECT_EQ(animation_manager_->next_transaction_id_, 1u);
}

// Verifies that a data update arriving after layout starts cancels the existing
// transaction and creates a new PRE snapshot transaction.
TEST_F(AnimationManagerImplTest, ReplacesTransactionAfterLayoutStarts) {
  UpdateAnimationConfig config;
  config.enable = true;
  animation_manager_->SetUpdateAnimationConfig(config);

  // 1. Create the first transaction and consume its PRE target snapshot through
  // BeforeLayout().
  animation_manager_->BeforeDataUpdate(true, true, true);
  animation_manager_->BeforeLayout();
  ASSERT_NE(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kPreLayoutInfoRecorded);

  // 2. Another data update during layout invalidates the old transaction and
  // replaces it with one whose ID has advanced.
  animation_manager_->BeforeDataUpdate(true, true, true);
  ASSERT_NE(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(animation_manager_->active_transaction_->id(), 2u);
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kHasPreChildrenSnapshot);
}

// Verifies that lifecycle hooks still advance state in order with no targets
// and that repeated calls are idempotent.
TEST_F(AnimationManagerImplTest, AdvancesTransactionThroughLayoutAndFlush) {
  MockItemAnimator* item_animator =
      CreateActiveTransaction(TransactionState::kHasPreChildrenSnapshot);

  // 1. BeforeLayout() consumes the empty PRE snapshot and advances state. A
  // repeated call performs no further work.
  animation_manager_->BeforeLayout();
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kPreLayoutInfoRecorded);
  animation_manager_->BeforeLayout();
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kPreLayoutInfoRecorded);

  // 2. AfterLayoutBeforeFlush() processes the empty recorder and advances
  // state. A repeated call performs no further work.
  animation_manager_->AfterLayoutBeforeFlush();
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kHasPrepared);
  animation_manager_->AfterLayoutBeforeFlush();
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kHasPrepared);

  // 3. AfterFlush() invokes RunPendingAnimations() once and flushes the patch
  // queue once.
  EXPECT_EQ(mock_list_element_->flush_patching_count(), 0);
  animation_manager_->AfterFlush();
  ASSERT_NE(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kRunning);
  EXPECT_EQ(item_animator->run_pending_animations_count(), 1);
  EXPECT_EQ(mock_list_element_->flush_patching_count(), 1);

  // 4. Repeating AfterFlush() for a running transaction neither starts the
  // animations again nor performs another flush.
  animation_manager_->AfterFlush();
  EXPECT_EQ(item_animator->run_pending_animations_count(), 1);
  EXPECT_EQ(mock_list_element_->flush_patching_count(), 1);
}

// Verifies that the manager enters a target into its animation lifecycle after
// the recorder identifies a disappearance and the animator accepts it.
TEST_F(AnimationManagerImplTest, PreparesTargetWhenAnimatorAcceptsAnimation) {
  auto target = std::make_unique<MockAnimationTarget>("removed");
  target->SetAnimationLayout(0, 10.f, 20.f, 100.f, 50.f);
  std::vector<WeakAnimationTarget> pre_targets{
      target->GetWeakAnimationTarget()};

  MockItemAnimator* item_animator = CreateActiveTransaction(
      TransactionState::kHasPreChildrenSnapshot, std::move(pre_targets));
  item_animator->SetAnimateResults(true, false, false, false);

  // 1. Record the target during PRE. Its absence from POST dispatches a
  // disappearance animation.
  animation_manager_->BeforeLayout();
  animation_manager_->AfterLayoutBeforeFlush();
  EXPECT_EQ(item_animator->animate_disappearance_count(), 1);
  EXPECT_EQ(item_animator->last_target(), target.get());

  // 2. Once the animator accepts the animation, the target enters the matching
  // animation lifecycle.
  ASSERT_EQ(target->prepared_animation_types().size(), 1u);
  EXPECT_EQ(target->prepared_animation_types()[0],
            ItemAnimationType::kDisappearance);
  EXPECT_TRUE(
      animation_manager_->active_transaction_->recorder().records_.empty());
}

// Verifies that the manager's four Process callbacks invoke the corresponding
// animator APIs and only accepted targets enter an animation lifecycle.
TEST_F(AnimationManagerImplTest,
       DispatchesAnimationTypesAndSkipsRejectedTargets) {
  MockAnimationTarget appeared_target("appeared");
  MockAnimationTarget persistent_target("persistent");
  MockAnimationTarget changed_target("changed");
  MockAnimationTarget rejected_target("rejected");
  ItemLayoutInfo pre_layout_info;
  ItemLayoutInfo post_layout_info;

  MockItemAnimator* item_animator =
      CreateActiveTransaction(TransactionState::kPostLayoutInfoRecorded);
  item_animator->SetAnimateResults(false, true, true, true);

  // 1. Invoke the Process callbacks for appearance, persistence, change, and
  // disappearance.
  animation_manager_->ProcessAppeared(&appeared_target, post_layout_info);
  animation_manager_->ProcessPersistent(&persistent_target, pre_layout_info,
                                        post_layout_info);
  animation_manager_->ProcessChanged(&changed_target, pre_layout_info,
                                     post_layout_info);
  animation_manager_->ProcessDisappeared(&rejected_target, pre_layout_info);

  EXPECT_EQ(item_animator->animate_appearance_count(), 1);
  EXPECT_EQ(item_animator->animate_persistence_count(), 1);
  EXPECT_EQ(item_animator->animate_change_count(), 1);
  EXPECT_EQ(item_animator->animate_disappearance_count(), 1);

  // 2. The three accepted targets enter their corresponding animation
  // lifecycles.
  ASSERT_EQ(appeared_target.prepared_animation_types().size(), 1u);
  EXPECT_EQ(appeared_target.prepared_animation_types()[0],
            ItemAnimationType::kAppearance);
  ASSERT_EQ(persistent_target.prepared_animation_types().size(), 1u);
  EXPECT_EQ(persistent_target.prepared_animation_types()[0],
            ItemAnimationType::kPersistence);
  ASSERT_EQ(changed_target.prepared_animation_types().size(), 1u);
  EXPECT_EQ(changed_target.prepared_animation_types()[0],
            ItemAnimationType::kChange);

  // 3. A target rejected by the animator does not enter an animation lifecycle.
  EXPECT_TRUE(rejected_target.prepared_animation_types().empty());
}

// Verifies that the manager cancels a running transaction only when the
// animator reports invalidated layout endpoints.
TEST_F(AnimationManagerImplTest,
       CancelsRunningTransactionOnlyWhenLayoutIsInvalidated) {
  auto lifecycle_observer = std::make_shared<MockItemAnimatorObserver>();
  MockItemAnimator* item_animator = CreateActiveTransaction(
      TransactionState::kRunning, {}, lifecycle_observer);
  AnimationTransaction* transaction =
      animation_manager_->active_transaction_.get();

  // 1. The active transaction keeps running while the animator reports valid
  // endpoints.
  item_animator->SetHasInvalidatedRunningAnimationLayout(false);
  animation_manager_->AfterLayoutBeforeFlush();
  EXPECT_EQ(animation_manager_->active_transaction_.get(), transaction);
  EXPECT_EQ(animation_manager_->active_transaction_->state(),
            TransactionState::kRunning);
  EXPECT_EQ(lifecycle_observer->cancel_animations_count, 0);

  // 2. An invalidated endpoint cancels the running transaction.
  item_animator->SetHasInvalidatedRunningAnimationLayout(true);
  animation_manager_->AfterLayoutBeforeFlush();
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(lifecycle_observer->cancel_animations_count, 1);
  EXPECT_FALSE(lifecycle_observer->last_cancel_destroy);
  EXPECT_TRUE(lifecycle_observer->listener_detached_when_cancelled);
  EXPECT_TRUE(lifecycle_observer->animator_destroyed);
  EXPECT_TRUE(lifecycle_observer->cancel_observed_before_destruction);
}

// Verifies that a valid data update which cannot animate cancels an existing
// transaction without creating a replacement.
TEST_F(AnimationManagerImplTest,
       CancelsActiveTransactionWhenNextUpdateCannotAnimate) {
  UpdateAnimationConfig config;
  config.enable = true;
  animation_manager_->SetUpdateAnimationConfig(config);

  auto lifecycle_observer = std::make_shared<MockItemAnimatorObserver>();
  CreateActiveTransaction(TransactionState::kRunning, {}, lifecycle_observer);

  // 1. While a transaction is running, process a valid diff that is explicitly
  // not expected to animate.
  animation_manager_->BeforeDataUpdate(true, false, true);

  // 2. The old transaction follows normal cancellation and no new PRE snapshot
  // transaction is created.
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(animation_manager_->next_transaction_id_, kInvalidTransactionId);
  EXPECT_EQ(lifecycle_observer->cancel_animations_count, 1);
  EXPECT_FALSE(lifecycle_observer->last_cancel_destroy);
  EXPECT_TRUE(lifecycle_observer->animator_destroyed);
}

// Verifies that synchronous animator completion removes the transaction from
// active state and defers destruction until a safe manager entry point.
TEST_F(AnimationManagerImplTest, RetiresSynchronouslyFinishedTransaction) {
  MockItemAnimator* item_animator =
      CreateActiveTransaction(TransactionState::kHasPrepared);
  item_animator->SetNotifyFinishedOnRun(true);

  // 1. RunPendingAnimations() synchronously reports completion and moves the
  // transaction from active to retired storage.
  animation_manager_->AfterFlush();
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  ASSERT_EQ(animation_manager_->retired_transactions_.size(), 1u);
  EXPECT_EQ(item_animator->run_pending_animations_count(), 1);
  EXPECT_EQ(item_animator->listener(), nullptr);
  EXPECT_EQ(animation_manager_->item_animator_callback_depth_, 0u);

  // 2. At the next data-update entry point the animator callback stack has
  // unwound, so the retired transaction can be released safely.
  animation_manager_->BeforeDataUpdate(false, false, false);
  EXPECT_TRUE(animation_manager_->retired_transactions_.empty());
}

// Verifies that item_animator_callback_depth_ gates release of retired
// transactions.
TEST_F(AnimationManagerImplTest,
       KeepsRetiredTransactionWhileInsideAnimatorCallback) {
  auto lifecycle_observer = std::make_shared<MockItemAnimatorObserver>();
  animation_manager_->retired_transactions_.emplace_back(
      std::make_unique<AnimationTransaction>(
          kInjectedTransactionId, std::vector<WeakAnimationTarget>{},
          std::make_unique<MockItemAnimator>(lifecycle_observer)));

  // 1. Simulate nonzero callback depth, which prevents transaction release.
  animation_manager_->item_animator_callback_depth_ = 1;
  animation_manager_->ReleaseRetiredTransactionsIfSafe();
  EXPECT_EQ(animation_manager_->retired_transactions_.size(), 1u);
  EXPECT_FALSE(lifecycle_observer->animator_destroyed);

  // 2. Once callback depth returns to zero, cleanup destroys the transaction
  // and its animator.
  animation_manager_->item_animator_callback_depth_ = 0;
  animation_manager_->ReleaseRetiredTransactionsIfSafe();
  EXPECT_TRUE(animation_manager_->retired_transactions_.empty());
  EXPECT_TRUE(lifecycle_observer->animator_destroyed);
}

// Verify that configs apply to future transactions and identical configs
// preserve active ones.
TEST_F(AnimationManagerImplTest, AppliesStagesAndPreservesIdenticalConfig) {
  UpdateAnimationConfig config;
  config.enable = true;
  config.stages = MakeDefaultAnimationStages(22, 33, 11, 44);
  animation_manager_->SetUpdateAnimationConfig(config);

  // 1. Enabling alone creates no transaction; eligible data updates create
  // animators.
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  animation_manager_->BeforeDataUpdate(true, true, true);
  ASSERT_NE(animation_manager_->active_transaction_, nullptr);
  auto* transaction = animation_manager_->active_transaction_.get();
  EXPECT_EQ(transaction->item_animator().animation_stages(), config.stages);

  // 2. Compare config copies by content and preserve transaction identity.
  const UpdateAnimationConfig identical = config;
  animation_manager_->SetUpdateAnimationConfig(identical);
  ASSERT_EQ(animation_manager_->active_transaction_.get(), transaction);
  EXPECT_EQ(animation_manager_->next_transaction_id_, 1u);

  // 3. Disabling cancels the transaction; re-enabling waits for the next data
  // update.
  config.enable = false;
  animation_manager_->SetUpdateAnimationConfig(config);
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  config.enable = true;
  animation_manager_->SetUpdateAnimationConfig(config);
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  animation_manager_->BeforeDataUpdate(true, true, true);
  ASSERT_NE(animation_manager_->active_transaction_, nullptr);
  EXPECT_EQ(animation_manager_->active_transaction_->id(), 2u);
  EXPECT_EQ(animation_manager_->active_transaction_->item_animator()
                .animation_stages(),
            config.stages);
}

// Verifies that Destroy() clears the active transaction and existing retired
// transactions.
TEST_F(AnimationManagerImplTest, DestroyClearsActiveAndRetiredTransactions) {
  auto lifecycle_observer = std::make_shared<MockItemAnimatorObserver>();
  CreateActiveTransaction(TransactionState::kRunning, {}, lifecycle_observer);
  animation_manager_->retired_transactions_.emplace_back(
      std::make_unique<AnimationTransaction>(
          kInjectedTransactionId + 1, std::vector<WeakAnimationTarget>{},
          std::make_unique<MockItemAnimator>()));

  // 1. Destroy() cancels the active transaction in destroy mode and clears
  // retired transactions that are safe to release.
  animation_manager_->Destroy();
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  EXPECT_TRUE(animation_manager_->retired_transactions_.empty());

  // 2. The manager detaches the listener, cancels in destroy mode, and then
  // destroys the animator.
  EXPECT_EQ(lifecycle_observer->cancel_animations_count, 1);
  EXPECT_TRUE(lifecycle_observer->last_cancel_destroy);
  EXPECT_TRUE(lifecycle_observer->listener_detached_when_cancelled);
  EXPECT_TRUE(lifecycle_observer->animator_destroyed);
  EXPECT_TRUE(lifecycle_observer->cancel_observed_before_destruction);

  // 3. A repeated Destroy() call does not process already cleared transactions.
  animation_manager_->Destroy();
  EXPECT_EQ(animation_manager_->active_transaction_, nullptr);
  EXPECT_TRUE(animation_manager_->retired_transactions_.empty());
  EXPECT_EQ(lifecycle_observer->cancel_animations_count, 1);
}

}  // namespace
}  // namespace list
}  // namespace lynx
