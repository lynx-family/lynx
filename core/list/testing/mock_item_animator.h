// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_TESTING_MOCK_ITEM_ANIMATOR_H_
#define CORE_LIST_TESTING_MOCK_ITEM_ANIMATOR_H_

#include <memory>
#include <utility>

#include "core/list/animation/item_animator.h"

namespace lynx {
namespace list {

// Retains lifecycle observations after a transaction destroys its
// MockItemAnimator.
struct MockItemAnimatorObserver {
  int cancel_animations_count{0};
  bool last_cancel_destroy{false};
  bool listener_detached_when_cancelled{false};
  bool animator_destroyed{false};
  bool cancel_observed_before_destruction{false};
};

class MockItemAnimator final : public ItemAnimator {
 public:
  MockItemAnimator() = default;
  explicit MockItemAnimator(
      std::shared_ptr<MockItemAnimatorObserver> lifecycle_observer)
      : lifecycle_observer_(std::move(lifecycle_observer)) {}
  ~MockItemAnimator() override {
    if (lifecycle_observer_) {
      lifecycle_observer_->animator_destroyed = true;
      lifecycle_observer_->cancel_observed_before_destruction =
          lifecycle_observer_->cancel_animations_count > 0;
    }
  }

  bool AnimateDisappearance(AnimationTarget* target,
                            const ItemLayoutInfo&) override {
    ++animate_disappearance_count_;
    last_target_ = target;
    return animate_disappearance_result_;
  }

  bool AnimateAppearance(AnimationTarget* target,
                         const ItemLayoutInfo&) override {
    ++animate_appearance_count_;
    last_target_ = target;
    return animate_appearance_result_;
  }

  bool AnimatePersistence(AnimationTarget* target, const ItemLayoutInfo&,
                          const ItemLayoutInfo&) override {
    ++animate_persistence_count_;
    last_target_ = target;
    return animate_persistence_result_;
  }

  bool AnimateChange(AnimationTarget* target, const ItemLayoutInfo&,
                     const ItemLayoutInfo&) override {
    ++animate_change_count_;
    last_target_ = target;
    return animate_change_result_;
  }

  void RunPendingAnimations() override {
    ++run_pending_animations_count_;
    if (notify_finished_on_run_ && listener()) {
      listener()->OnAllAnimationsFinished();
    }
  }
  bool HasInvalidatedRunningAnimationLayout() const override {
    return has_invalidated_running_animation_layout_;
  }
  void CancelAnimations(bool destroy) override {
    ++cancel_animations_count_;
    last_cancel_destroy_ = destroy;
    if (lifecycle_observer_) {
      ++lifecycle_observer_->cancel_animations_count;
      lifecycle_observer_->last_cancel_destroy = destroy;
      lifecycle_observer_->listener_detached_when_cancelled =
          listener() == nullptr;
    }
  }

  void SetAnimateResults(bool disappearance, bool appearance, bool persistence,
                         bool change) {
    animate_disappearance_result_ = disappearance;
    animate_appearance_result_ = appearance;
    animate_persistence_result_ = persistence;
    animate_change_result_ = change;
  }
  void SetHasInvalidatedRunningAnimationLayout(bool invalidated) {
    has_invalidated_running_animation_layout_ = invalidated;
  }
  void SetNotifyFinishedOnRun(bool notify) { notify_finished_on_run_ = notify; }

  int animate_disappearance_count() const {
    return animate_disappearance_count_;
  }
  int animate_appearance_count() const { return animate_appearance_count_; }
  int animate_persistence_count() const { return animate_persistence_count_; }
  int animate_change_count() const { return animate_change_count_; }
  int run_pending_animations_count() const {
    return run_pending_animations_count_;
  }
  int cancel_animations_count() const { return cancel_animations_count_; }
  bool last_cancel_destroy() const { return last_cancel_destroy_; }
  AnimationTarget* last_target() const { return last_target_; }

 private:
  bool animate_disappearance_result_{false};
  bool animate_appearance_result_{false};
  bool animate_persistence_result_{false};
  bool animate_change_result_{false};
  bool has_invalidated_running_animation_layout_{false};
  bool notify_finished_on_run_{false};
  int animate_disappearance_count_{0};
  int animate_appearance_count_{0};
  int animate_persistence_count_{0};
  int animate_change_count_{0};
  int run_pending_animations_count_{0};
  int cancel_animations_count_{0};
  bool last_cancel_destroy_{false};
  AnimationTarget* last_target_{nullptr};
  std::shared_ptr<MockItemAnimatorObserver> lifecycle_observer_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_TESTING_MOCK_ITEM_ANIMATOR_H_
