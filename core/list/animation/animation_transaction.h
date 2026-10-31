// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ANIMATION_TRANSACTION_H_
#define CORE_LIST_ANIMATION_ANIMATION_TRANSACTION_H_

#include <memory>
#include <vector>

#include "core/list/animation/animation_types.h"
#include "core/list/animation/item_animation_recorder.h"
#include "core/list/animation/item_animator.h"

namespace lynx {
namespace list {

class ItemHolder;

// Represents one list update animation transaction, from capture of the PRE
// target snapshot until the animation batch completes or is cancelled. The
// transaction owns resources shared across stages, while AnimationManagerImpl
// coordinates those stages.
class AnimationTransaction {
 public:
  AnimationTransaction(TransactionId id,
                       std::vector<WeakAnimationTarget> pre_targets,
                       std::unique_ptr<ItemAnimator> item_animator);
  ~AnimationTransaction();

  AnimationTransaction(const AnimationTransaction&) = delete;
  AnimationTransaction& operator=(const AnimationTransaction&) = delete;

  std::vector<WeakAnimationTarget> TakePreTargets();

  void SetDeferredItemHolders(
      std::vector<std::unique_ptr<ItemHolder>> item_holders);

  std::vector<std::unique_ptr<ItemHolder>> TakeDeferredItemHolders();

  void SetState(TransactionState state) { state_ = state; }

  TransactionState state() const { return state_; }

  TransactionId id() const { return id_; }

  ItemAnimationRecorder& recorder() { return recorder_; }

  ItemAnimator& item_animator() { return *item_animator_; }

 private:
  // A monotonically increasing identifier assigned by AnimationManager for
  // diagnostics and for distinguishing consecutive transactions.
  TransactionId id_{kInvalidTransactionId};

  // The current transaction stage, used by AnimationManagerImpl to decide
  // whether each lifecycle hook should proceed. AnimationTransaction stores the
  // state but does not validate transition order.
  TransactionState state_;

  // A snapshot of weak animation targets corresponding to the children attached
  // before the data update. BeforeLayout() consumes this collection and records
  // PRE layout information for each live target. The snapshot does not keep
  // targets alive.
  std::vector<WeakAnimationTarget> pre_targets_;

  // Removed ItemHolders whose ownership was transferred from ListAdapter. The
  // unique_ptrs keep remove-animation targets alive. On normal completion or
  // cancellation outside destroy mode, AnimationManagerImpl returns them to
  // ListAdapter for recycling. During manager destruction, they are released
  // with the transaction instead.
  std::vector<std::unique_ptr<ItemHolder>> deferred_item_holders_;

  // C++ destroys members in reverse declaration order. Declaring
  // item_animator_ last ensures that it is destroyed before
  // deferred_item_holders_, keeping remove targets alive during animator
  // teardown.
  ItemAnimationRecorder recorder_;
  std::unique_ptr<ItemAnimator> item_animator_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ANIMATION_TRANSACTION_H_
