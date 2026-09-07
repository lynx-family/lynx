// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ITEM_ANIMATOR_H_
#define CORE_LIST_ANIMATION_ITEM_ANIMATOR_H_

#include <cstdint>

#include "core/list/animation/animation_target.h"
#include "core/list/animation/animation_types.h"

namespace lynx {
namespace list {

// Defines the snapshot, scheduling, cancellation, and lifecycle contract for
// list item animations.
class ItemAnimator {
 public:
  class Listener {
   public:
    virtual ~Listener() = default;

    virtual void OnAllAnimationsFinished() = 0;
  };

  ItemAnimator() = default;
  virtual ~ItemAnimator() = default;

  ItemAnimator(const ItemAnimator&) = delete;
  ItemAnimator& operator=(const ItemAnimator&) = delete;

  // 1. Layout snapshot capture
  // These methods read the target's current logical geometry and return a
  // value snapshot. They do not retain snapshots, compare layouts, or create
  // animations.
  virtual ItemLayoutInfo GetPreItemLayoutInfo(const AnimationTarget* target);
  virtual ItemLayoutInfo GetPostItemLayoutInfo(const AnimationTarget* target);

  // 2. High-level animation operations
  // Only a pre-layout snapshot exists for the target.
  virtual bool AnimateDisappearance(AnimationTarget* target,
                                    const ItemLayoutInfo& pre_info) = 0;
  // Only a post-layout snapshot exists for the target.
  virtual bool AnimateAppearance(AnimationTarget* target,
                                 const ItemLayoutInfo& post_info) = 0;
  // The same target exists before and after layout.
  virtual bool AnimatePersistence(AnimationTarget* target,
                                  const ItemLayoutInfo& pre_info,
                                  const ItemLayoutInfo& post_info) = 0;
  // A single target changed between pre-layout and post-layout. This interface
  // intentionally does not expose old/new target cross-animation.
  virtual bool AnimateChange(AnimationTarget* target,
                             const ItemLayoutInfo& pre_info,
                             const ItemLayoutInfo& post_info) = 0;

  // 3. Scheduling and cancellation
  // Each AnimationTransaction owns one ItemAnimator, so no transaction ID is
  // required. An implementation schedules only the pending animations it owns.
  virtual void RunPendingAnimations() = 0;
  // Returns whether a new layout invalidated an endpoint used by a running
  // position-interpolating animation. A layout pass alone does not invalidate
  // an animation.
  virtual bool HasInvalidatedRunningAnimationLayout() const = 0;
  // Both modes clear the pending queues and end the animation lifecycle of
  // each live pending target. With destroy=false, running targets are also
  // restored to their final state and have their lifecycle ended. The
  // destroy=true teardown path avoids accessing running targets. Neither mode
  // dispatches the normal batch-completion notification.
  virtual void CancelAnimations(bool destroy) = 0;

  // 4. Listener and duration configuration
  void SetListener(Listener* listener) { listener_ = listener; }
  void SetAddDuration(int32_t duration_ms) { add_duration_ms_ = duration_ms; }
  void SetRemoveDuration(int32_t duration_ms) {
    remove_duration_ms_ = duration_ms;
  }
  void SetMoveDuration(int32_t duration_ms) { move_duration_ms_ = duration_ms; }
  void SetChangeDuration(int32_t duration_ms) {
    change_duration_ms_ = duration_ms;
  }

  Listener* listener() { return listener_; }
  int32_t add_duration_ms() const { return add_duration_ms_; }
  int32_t remove_duration_ms() const { return remove_duration_ms_; }
  int32_t move_duration_ms() const { return move_duration_ms_; }
  int32_t change_duration_ms() const { return change_duration_ms_; }

 protected:
  // Captures the target's current logical layout as an independent value.
  static ItemLayoutInfo GetItemLayoutInfo(const AnimationTarget& target);

 private:
  int32_t add_duration_ms_{kDefaultAddAnimationDurationMs};
  int32_t remove_duration_ms_{kDefaultRemoveAnimationDurationMs};
  int32_t move_duration_ms_{kDefaultMoveAnimationDurationMs};
  int32_t change_duration_ms_{kDefaultChangeAnimationDurationMs};

  // Non-owning. AnimationManager installs the listener and must clear it before
  // the transaction is released.
  Listener* listener_{nullptr};
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ITEM_ANIMATOR_H_
