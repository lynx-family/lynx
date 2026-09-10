// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ITEM_ANIMATOR_DEFAULT_H_
#define CORE_LIST_ANIMATION_ITEM_ANIMATOR_DEFAULT_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "core/list/animation/animation_types.h"
#include "core/list/animation/item_animator_base.h"

namespace lynx {
namespace list {

// The default queueing and execution policy for list item animations.
// Configured stages determine animation delays; all accepted animations are
// started in one scheduling pass.
class ItemAnimatorDefault
    : public ItemAnimatorBase,
      public fml::EnableWeakFromThis<ItemAnimatorDefault> {
 public:
  ItemAnimatorDefault() = default;
  ~ItemAnimatorDefault() override = default;

  void RunPendingAnimations() override;
  bool HasInvalidatedRunningAnimationLayout() const override;
  void CancelAnimations(bool destroy) override;

 protected:
  bool AnimateRemoveImpl(AnimationTarget* target,
                         const ItemLayoutInfo& pre_layout_info) override;
  bool AnimateAddImpl(AnimationTarget* target,
                      const ItemLayoutInfo& post_layout_info) override;
  bool AnimateMoveImpl(AnimationTarget* target,
                       const ItemLayoutInfo& pre_layout_info,
                       const ItemLayoutInfo& post_layout_info) override;
  bool AnimateChangeImpl(AnimationTarget* target,
                         const ItemLayoutInfo& pre_layout_info,
                         const ItemLayoutInfo& post_layout_info) override;

 private:
  // Remove uses PRE, Add uses POST, and Move uses both snapshots. Change also
  // uses one target; cross-animation between separate targets is unsupported.
  struct PendingAnimationInfo {
    WeakAnimationTarget target;
    ItemLayoutInfo pre_layout_info;
    ItemLayoutInfo post_layout_info;
  };

  struct RunningAnimation {
    AnimationId animation_id{0};
    ItemAnimationType type{ItemAnimationType::kPersistence};
    WeakAnimationTarget target;
    ItemLayoutInfo pre_layout_info;
    ItemLayoutInfo post_layout_info;
    std::shared_ptr<::lynx::animation::basic::LynxBasicAnimator> animator;
  };

  struct AnimationTypeState {
    // Created but unfinished animations of this type, including delayed
    // animations but excluding pending items.
    std::size_t remaining_items{0};
    int32_t delay_ms{0};
    int32_t duration_ms{0};
    // Whether this type has started in the current transaction.
    bool has_started{false};
    // The last sampled progress for this type in the current transaction.
    std::optional<float> last_iteration_progress{};
  };

  // Centralizes LynxBasicAnimator creation. Production does not inject a
  // VSyncMonitor; BasicAnimatorFrameCallbackProvider obtains a thread-local
  // monitor when it first requests a frame. Tests override this method to
  // inject a manually advanced monitor.
  virtual std::shared_ptr<::lynx::animation::basic::LynxBasicAnimator>
  CreateBasicAnimator(starlight::AnimationData animation_data);

  void StartAnimation(RunningAnimation animation, int32_t duration_ms,
                      int32_t delay_ms);
  void ApplyAnimationFrame(const RunningAnimation& animation, float progress);
  void StartRemoveAnimation(const PendingAnimationInfo& info);
  void StartAddAnimation(const PendingAnimationInfo& info);
  void StartMoveAnimation(const PendingAnimationInfo& info);
  void StartChangeAnimation(const PendingAnimationInfo& info) {
    // TODO: impl change animation
  }

  void ApplyRemoveFrame(AnimationTarget* target, float progress);
  void ApplyAddFrame(AnimationTarget* target, float progress);
  void ApplyMoveFrame(AnimationTarget* target,
                      const ItemLayoutInfo& pre_layout_info,
                      const ItemLayoutInfo& post_layout_info, float progress);
  void ApplyChangeFrame(const PendingAnimationInfo& info, float progress) {
    // TODO: impl change animation
  }

  RunningAnimation* FindRunningAnimation(AnimationTargetKey target_key,
                                         AnimationId animation_id);
  void FinishRunningAnimation(AnimationTargetKey target_key,
                              RunningAnimation animation, bool cancelled);
  void ResetTargetToFinalState(AnimationTarget* target,
                               ItemAnimationType animation_type);
  void CancelPendingAnimations();

  // Dispatches animation event.
  void DispatchAnimationStartIfNeeded(ItemAnimationType type);
  void DispatchAnimationIterationIfNeeded(ItemAnimationType type,
                                          float progress);
  void DispatchAnimationEndIfNeeded(ItemAnimationType type);
  void DispatchAnimationCancelIfNeeded(
      const std::vector<ItemAnimationType>& cancelled_types);
  void DispatchAnimationFinishedIfNeeded();

  bool has_pending_animations() const {
    return !pending_removals_.empty() || !pending_adds_.empty() ||
           !pending_moves_.empty() || !pending_changes_.empty();
  }
  bool has_running_animations() const { return !running_animations_.empty(); }

 private:
  bool in_starting_animations_{false};
  bool in_cancelling_animations_{false};
  std::vector<PendingAnimationInfo> pending_removals_;
  std::vector<PendingAnimationInfo> pending_adds_;
  std::vector<PendingAnimationInfo> pending_moves_;
  std::vector<PendingAnimationInfo> pending_changes_;
  // The target-address key permits at most one running record per target.
  // animation_id prevents callbacks for a replaced record from operating on
  // its replacement.
  std::unordered_map<AnimationTargetKey, RunningAnimation> running_animations_;
  std::unordered_map<ItemAnimationType, AnimationTypeState>
      animation_type_states_;
  AnimationId next_animation_id_{0};
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ITEM_ANIMATOR_DEFAULT_H_
