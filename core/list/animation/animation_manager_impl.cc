// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/animation_manager_impl.h"

#include <utility>
#include <vector>

#include "core/list/animation/item_animator_default.h"
#include "core/list/decoupled_item_holder.h"
#include "core/list/decoupled_list_adapter.h"
#include "core/list/decoupled_list_children_helper.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

void AnimationManagerImpl::SetUpdateAnimationConfig(
    const UpdateAnimationConfig& config) {
  const bool need_disable_animations =
      enable_update_animation_ && !config.enable;
  // Compare stage order, types, and durations; identical configs preserve
  // animations.
  const bool stages_have_changed =
      update_animation_config_.stages != config.stages;

  // Save the config first so future transactions use it after cancellation.
  update_animation_config_ = config;
  enable_update_animation_ = config.enable;

  // Disabling or changing stages cancels pending and running animations
  // immediately.
  if (need_disable_animations) {
    CancelAnimationTransaction(AnimationCancelReason::kAnimationDisabled);
  } else if (stages_have_changed) {
    CancelAnimationTransaction(AnimationCancelReason::kAnimationConfigChanged);
  }
}

std::unique_ptr<ItemAnimator>
AnimationManagerImpl::CreateItemAnimatorForTransaction() {
  auto item_animator = std::make_unique<ItemAnimatorDefault>();
  item_animator->SetAnimationStages(update_animation_config_.stages);
  item_animator->SetListener(this);
  return item_animator;
}

// Ignore disabled or invalid updates. Otherwise reuse an eligible PRE snapshot
// transaction, or cancel it and create a replacement if this update can
// animate.
void AnimationManagerImpl::BeforeDataUpdate(bool has_valid_diff,
                                            bool has_expected_diff_animation,
                                            bool has_completed_first_layout) {
  // Release transactions that have left their completion callback stacks before
  // starting a new data update.
  ReleaseRetiredTransactionsIfSafe();

  // Each transaction owns an ItemAnimator, so animator existence does not
  // represent whether update animations are enabled.
  if (!has_valid_diff || !enable_update_animation_) {
    return;
  }

  const bool can_run_new_animation =
      has_expected_diff_animation && has_completed_first_layout;

  // Handle an existing active animation transaction.
  if (active_transaction_) {
    // Preserve the first attached-target snapshot across pre-layout updates.
    if (can_run_new_animation &&
        active_transaction_->state() ==
            TransactionState::kHasPreChildrenSnapshot) {
      // Neither cancel the transaction nor start a new one.
      return;
    }
    // Cancel transactions that cannot be reused before creating a replacement.
    CancelAnimationTransaction(AnimationCancelReason::kNewDataUpdate);
  }

  if (can_run_new_animation) {
    BeginAnimationTransaction();
  }
}

void AnimationManagerImpl::BeginAnimationTransaction() {
  // Capture weak targets for the children attached before the data update.
  const auto& attached_children =
      list_container_->list_children_helper()->attached_children();
  std::vector<WeakAnimationTarget> pre_targets;
  pre_targets.reserve(attached_children.size());
  for (ItemHolder* item_holder : attached_children) {
    if (item_holder) {
      pre_targets.emplace_back(item_holder->GetWeakAnimationTarget());
    }
  }
  ++next_transaction_id_;
  DLIST_LOGI("[AM] AnimationManagerImpl::BeginAnimationTransaction: id="
             << next_transaction_id_);

  // Each transaction owns an ItemAnimator to isolate pending and running state.
  active_transaction_ = std::make_unique<AnimationTransaction>(
      next_transaction_id_, std::move(pre_targets),
      CreateItemAnimatorForTransaction());
}

void AnimationManagerImpl::BeforeLayout() {
  if (!active_transaction_ || active_transaction_->state() !=
                                  TransactionState::kHasPreChildrenSnapshot) {
    return;
  }

  ItemAnimator& item_animator = active_transaction_->item_animator();
  ItemAnimationRecorder& recorder = active_transaction_->recorder();
  std::vector<WeakAnimationTarget> pre_targets =
      active_transaction_->TakePreTargets();
  for (const WeakAnimationTarget& weak_target : pre_targets) {
    AnimationTarget* target = weak_target.get();
    if (target) {
      recorder.RecordPreItemLayoutInfo(
          target, item_animator.GetPreItemLayoutInfo(target));
    }
  }
  active_transaction_->SetState(TransactionState::kPreLayoutInfoRecorded);
}

void AnimationManagerImpl::AfterLayoutBeforeFlush() {
  if (!active_transaction_) {
    return;
  }

  if (active_transaction_->state() == TransactionState::kRunning) {
    ItemAnimator& item_animator = active_transaction_->item_animator();

    // Relayout cancels running animations only when a recorded endpoint differs
    // from its target's latest logical position.
    if (item_animator.HasInvalidatedRunningAnimationLayout()) {
      CancelAnimationTransaction(AnimationCancelReason::kLayoutInvalidated);
    }
    return;
  }

  if (active_transaction_->state() !=
      TransactionState::kPreLayoutInfoRecorded) {
    return;
  }

  // Record POST layout bounds for the currently attached targets.
  ItemAnimator& item_animator = active_transaction_->item_animator();
  ItemAnimationRecorder& recorder = active_transaction_->recorder();
  const auto& attached_children =
      list_container_->list_children_helper()->attached_children();
  for (ItemHolder* item_holder : attached_children) {
    if (item_holder) {
      AnimationTarget* target = static_cast<AnimationTarget*>(item_holder);
      recorder.RecordPostLayoutInfo(
          target, item_animator.GetPostItemLayoutInfo(target));
    }
  }
  active_transaction_->SetState(TransactionState::kPostLayoutInfoRecorded);

  // Transfer removed-holder ownership before LayoutManager recycles them. The
  // transaction keeps these targets alive until completion or cancellation so
  // asynchronous remove-animation callbacks cannot access dangling objects.
  active_transaction_->SetDeferredItemHolders(
      list_container_->list_adapter()->TakeRemovedItemHoldersForAnimation());

  recorder.Process(*this);
  active_transaction_->SetState(TransactionState::kHasPrepared);
}

void AnimationManagerImpl::AfterFlush() {
  if (!active_transaction_ ||
      active_transaction_->state() != TransactionState::kHasPrepared) {
    return;
  }

  // Change state before starting the animator so AfterFlush() starts this
  // transaction at most once.
  active_transaction_->SetState(TransactionState::kRunning);
  ItemAnimator& item_animator = active_transaction_->item_animator();

  // Synchronous completion may retire active_transaction_; do not access it
  // after RunPendingAnimations().
  item_animator.RunPendingAnimations();

  // Flush accumulated initial property updates after the entire batch starts.
  if (list_container_) {
    list_container_->FlushPatching();
  }
}

void AnimationManagerImpl::CancelAnimationTransaction(
    AnimationCancelReason reason) {
  if (active_transaction_) {
    DLIST_LOGI("cancel list animation transaction: id="
               << active_transaction_->id()
               << ", reason=" << static_cast<int>(reason));
    active_transaction_->SetState(TransactionState::kCancel);
    // Detach the transaction from the manager first. CancelAnimations() may
    // synchronously invoke a BasicAnimator cancel callback; reentrant animator
    // or listener callbacks must not process the same transaction again.
    std::unique_ptr<AnimationTransaction> transaction =
        std::move(active_transaction_);

    bool destroy = reason == AnimationCancelReason::kManagerCleared;
    ItemAnimator& item_animator = transaction->item_animator();
    // CancelAnimations() may synchronously invoke BasicAnimator cancel
    // callbacks. Detach the completion listener first so an explicit
    // cancellation cannot reenter AnimationManager.
    item_animator.SetListener(nullptr);
    item_animator.CancelAnimations(destroy);
    if (!destroy) {
      RecycleDeferredItemHolders(*transaction);
    }
  }
}

void AnimationManagerImpl::Destroy() {
  // Stop animations without restoring presentation state during teardown.
  CancelAnimationTransaction(AnimationCancelReason::kManagerCleared);

  // Release retired transactions only after their callback stacks unwind.
  ReleaseRetiredTransactionsIfSafe();
}

void AnimationManagerImpl::ReleaseRetiredTransactionsIfSafe() {
  // Completion cleanup may reenter the manager. Do not release a transaction
  // that is still on an outer animator callback stack.
  if (item_animator_callback_depth_ != 0) {
    return;
  }
  retired_transactions_.clear();
}

void AnimationManagerImpl::RecycleDeferredItemHolders(
    AnimationTransaction& transaction) {
  std::vector<std::unique_ptr<ItemHolder>> deferred_item_holders =
      transaction.TakeDeferredItemHolders();
  if (!list_container_ || !list_container_->list_adapter()) {
    return;
  }
  bool is_cancel = transaction.state() == TransactionState::kCancel;
  for (const auto& item_holder : deferred_item_holders) {
    if (item_holder) {
      // Restore opacity before recycling can detach ItemElementDelegate,
      // ensuring the node is visible when reused.
      item_holder->UpdateAnimationOpacity(1.f, !is_cancel);
      list_container_->list_adapter()->RecycleItemHolder(item_holder.get(),
                                                         !is_cancel);
    }
  }
}

void AnimationManagerImpl::PrepareTargetIfAnimated(
    AnimationTarget* target, ItemAnimationType animation_type, bool animated) {
  if (target && animated && active_transaction_) {
    target->PrepareForAnimation(animation_type);
  }
}

void AnimationManagerImpl::ProcessDisappeared(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info) {
  if (!active_transaction_) {
    return;
  }
  ItemAnimator& item_animator = active_transaction_->item_animator();
  PrepareTargetIfAnimated(
      target, ItemAnimationType::kDisappearance,
      item_animator.AnimateDisappearance(target, pre_layout_info));
}

void AnimationManagerImpl::ProcessAppeared(
    AnimationTarget* target, const ItemLayoutInfo& post_layout_info) {
  if (!active_transaction_) {
    return;
  }
  ItemAnimator& item_animator = active_transaction_->item_animator();
  PrepareTargetIfAnimated(
      target, ItemAnimationType::kAppearance,
      item_animator.AnimateAppearance(target, post_layout_info));
}

void AnimationManagerImpl::ProcessPersistent(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info,
    const ItemLayoutInfo& post_layout_info) {
  if (!active_transaction_) {
    return;
  }
  ItemAnimator& item_animator = active_transaction_->item_animator();
  PrepareTargetIfAnimated(target, ItemAnimationType::kPersistence,
                          item_animator.AnimatePersistence(
                              target, pre_layout_info, post_layout_info));
}

void AnimationManagerImpl::ProcessChanged(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info,
    const ItemLayoutInfo& post_layout_info) {
  if (!active_transaction_) {
    return;
  }
  ItemAnimator& item_animator = active_transaction_->item_animator();
  PrepareTargetIfAnimated(
      target, ItemAnimationType::kChange,
      item_animator.AnimateChange(target, pre_layout_info, post_layout_info));
}

void AnimationManagerImpl::ProcessNone(AnimationTarget* target) {}

void AnimationManagerImpl::OnAllAnimationsFinished() {
  // Track callback depth to cover nested callbacks during completion cleanup.
  ++item_animator_callback_depth_;

  if (active_transaction_) {
    std::unique_ptr<AnimationTransaction> transaction =
        std::move(active_transaction_);
    // The transaction completed naturally. Detach its listener first so any
    // unexpected later notification cannot access the manager again.
    transaction->item_animator().SetListener(nullptr);
    RecycleDeferredItemHolders(*transaction);
    // This method runs from the ItemAnimator completion callback. Do not
    // destroy the transaction before returning, because that would destroy the
    // ItemAnimator while one of its own member functions is still on the stack.
    retired_transactions_.emplace_back(std::move(transaction));
  }

  --item_animator_callback_depth_;
}

std::unique_ptr<AnimationManager> CreateAnimationManager(
    ListContainerImpl* list_container) {
  return std::make_unique<AnimationManagerImpl>(list_container);
}

}  // namespace list
}  // namespace lynx
