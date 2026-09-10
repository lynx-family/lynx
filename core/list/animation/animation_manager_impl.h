// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ANIMATION_MANAGER_IMPL_H_
#define CORE_LIST_ANIMATION_ANIMATION_MANAGER_IMPL_H_

#include <memory>
#include <vector>

#include "core/list/animation/animation_manager.h"
#include "core/list/animation/animation_transaction.h"
#include "core/list/animation/animation_types.h"
#include "core/list/animation/item_animation_recorder.h"
#include "core/list/animation/item_animator.h"

namespace lynx {
namespace list {

// Coordinates list update animation transactions from snapshots to recycling.
class AnimationManagerImpl final
    : public AnimationManager,
      private ItemAnimationRecorder::ProcessCallback,
      private ItemAnimator::Listener {
 public:
  AnimationManagerImpl(ListContainerImpl* list_container)
      : list_container_(list_container) {}
  ~AnimationManagerImpl() override = default;

  AnimationManagerImpl(const AnimationManagerImpl&) = delete;
  AnimationManagerImpl& operator=(const AnimationManagerImpl&) = delete;

  void SetUpdateAnimationConfig(const UpdateAnimationConfig& config) override;
  void BeforeDataUpdate(bool has_valid_diff, bool has_expected_diff_animation,
                        bool has_completed_first_layout) override;
  void BeforeLayout() override;
  void AfterLayoutBeforeFlush() override;
  void AfterFlush() override;
  void CancelAnimationTransaction(AnimationCancelReason reason) override;
  void Destroy() override;

 private:
  std::unique_ptr<ItemAnimator> CreateItemAnimatorForTransaction();
  void ApplyAnimationConfig(ItemAnimator& item_animator) const;
  void BeginAnimationTransaction();
  void ReleaseRetiredTransactionsIfSafe();
  void RecycleDeferredItemHolders(AnimationTransaction& transaction);
  void PrepareTargetIfAnimated(AnimationTarget* target,
                               ItemAnimationType animation_type, bool animated);

  // ItemAnimationRecorder::ProcessCallback
  void ProcessDisappeared(AnimationTarget* target,
                          const ItemLayoutInfo& pre_layout_info) override;
  void ProcessAppeared(AnimationTarget* target,
                       const ItemLayoutInfo& post_layout_info) override;
  void ProcessPersistent(AnimationTarget* target,
                         const ItemLayoutInfo& pre_layout_info,
                         const ItemLayoutInfo& post_layout_info) override;
  void ProcessChanged(AnimationTarget* target,
                      const ItemLayoutInfo& pre_layout_info,
                      const ItemLayoutInfo& post_layout_info) override;
  void ProcessNone(AnimationTarget* target) override;

  // ItemAnimator::Listener
  void OnAllAnimationsFinished() override;

 private:
  bool enable_update_animation_{false};
  UpdateAnimationConfig update_animation_config_;
  ListContainerImpl* list_container_{nullptr};
  std::unique_ptr<AnimationTransaction> active_transaction_;
  // A naturally completed transaction cannot be destroyed from within its own
  // ItemAnimator completion callback. Keep it alive until the next safe manager
  // entry point. A vector also accommodates nested completion caused by
  // reentrant callbacks.
  std::vector<std::unique_ptr<AnimationTransaction>> retired_transactions_;
  uint32_t item_animator_callback_depth_{0};
  TransactionId next_transaction_id_{kInvalidTransactionId};
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ANIMATION_MANAGER_IMPL_H_
