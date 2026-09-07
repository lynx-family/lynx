// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ITEM_ANIMATOR_DEFAULT_H_
#define CORE_LIST_ANIMATION_ITEM_ANIMATOR_DEFAULT_H_

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "core/list/animation/animation_types.h"
#include "core/list/animation/item_animator_base.h"

namespace lynx {
namespace list {

// The default queueing and execution policy for list item animations. Delays
// arrange the effective intervals as removals, then moves and changes, then
// additions; all accepted animations are started in one scheduling pass.
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
  struct RemoveInfo {
    WeakAnimationTarget target;
    ItemLayoutInfo pre_layout_info;
  };

  struct AddInfo {
    WeakAnimationTarget target;
    ItemLayoutInfo post_layout_info;
  };

  struct MoveInfo {
    WeakAnimationTarget target;
    ItemLayoutInfo pre_layout_info;
    ItemLayoutInfo post_layout_info;
  };

  // ChangeInfo intentionally contains one target. This implementation does not
  // support cross-animation between separate old and new targets.
  struct ChangeInfo {
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

  // Centralizes LynxBasicAnimator creation. Production does not inject a
  // VSyncMonitor; BasicAnimatorFrameCallbackProvider obtains a thread-local
  // monitor when it first requests a frame. Tests override this method to
  // inject a manually advanced monitor.
  virtual std::shared_ptr<::lynx::animation::basic::LynxBasicAnimator>
  CreateBasicAnimator(starlight::AnimationData animation_data);

  void StartRemoveAnimation(const RemoveInfo& info, int32_t delay_ms);
  void StartAddAnimation(const AddInfo& info, int32_t delay_ms);
  void StartMoveAnimation(const MoveInfo& info, int32_t delay_ms);
  void StartChangeAnimation(const ChangeInfo& info, int32_t delay_ms) {
    // TODO: impl change animation
  }
  void ApplyRemoveFrame(AnimationTarget* target, float progress);
  void ApplyAddFrame(AnimationTarget* target, float progress);
  void ApplyMoveFrame(AnimationTarget* target,
                      const ItemLayoutInfo& pre_layout_info,
                      const ItemLayoutInfo& post_layout_info, float progress);
  void ApplyChangeFrame(const ChangeInfo& info, float progress) {
    // TODO: impl change animation
  }

  RunningAnimation* FindRunningAnimation(AnimationTargetKey target_key,
                                         AnimationId animation_id);
  void FinishRunningAnimation(AnimationTargetKey target_key,
                              AnimationId animation_id, bool cancelled);
  void ResetTargetToFinalState(AnimationTarget* target,
                               ItemAnimationType animation_type);

  void CancelPendingAnimations();

  void DispatchAnimationFinishedIfNeeded();

  bool has_pending_animations() const {
    return !pending_removals_.empty() || !pending_adds_.empty() ||
           !pending_moves_.empty() || !pending_changes_.empty();
  }
  bool has_running_animations() const { return !running_animations_.empty(); }

 private:
  bool in_starting_animations_{false};
  bool in_cancelling_animations_{false};

  std::vector<RemoveInfo> pending_removals_;
  std::vector<AddInfo> pending_adds_;
  std::vector<MoveInfo> pending_moves_;
  std::vector<ChangeInfo> pending_changes_;

  // The target-address key permits at most one running record per target.
  // animation_id prevents callbacks for a replaced record from operating on
  // its replacement.
  std::unordered_map<AnimationTargetKey, RunningAnimation> running_animations_;
  AnimationId next_animation_id_{0};
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ITEM_ANIMATOR_DEFAULT_H_
