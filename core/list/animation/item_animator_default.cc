// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animator_default.h"

#include <algorithm>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/list/decoupled_list_types.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace list {

std::shared_ptr<::lynx::animation::basic::LynxBasicAnimator>
ItemAnimatorDefault::CreateBasicAnimator(
    starlight::AnimationData animation_data) {
  return std::make_shared<::lynx::animation::basic::LynxBasicAnimator>(
      std::move(animation_data));
}

bool ItemAnimatorDefault::AnimateRemoveImpl(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info) {
  if (!target) {
    return false;
  }
  WeakAnimationTarget weak_target = target->GetWeakAnimationTarget();
  if (!weak_target) {
    return false;
  }
  pending_removals_.emplace_back(RemoveInfo{
      .target = std::move(weak_target), .pre_layout_info = pre_layout_info});
  return true;
}

bool ItemAnimatorDefault::AnimateAddImpl(
    AnimationTarget* target, const ItemLayoutInfo& post_layout_info) {
  if (!target) {
    return false;
  }
  WeakAnimationTarget weak_target = target->GetWeakAnimationTarget();
  if (!weak_target) {
    return false;
  }
  pending_adds_.emplace_back(AddInfo{
      .target = std::move(weak_target),
      .post_layout_info = post_layout_info,
  });
  return true;
}

bool ItemAnimatorDefault::AnimateMoveImpl(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info,
    const ItemLayoutInfo& post_layout_info) {
  if (!target || !pre_layout_info.PositionChanged(post_layout_info)) {
    return false;
  }
  WeakAnimationTarget weak_target = target->GetWeakAnimationTarget();
  if (!weak_target) {
    return false;
  }
  pending_moves_.emplace_back(MoveInfo{
      .target = std::move(weak_target),
      .pre_layout_info = pre_layout_info,
      .post_layout_info = post_layout_info,
  });
  return true;
}

bool ItemAnimatorDefault::AnimateChangeImpl(
    AnimationTarget* target, const ItemLayoutInfo& pre_layout_info,
    const ItemLayoutInfo& post_layout_info) {
  return false;
}

void ItemAnimatorDefault::RunPendingAnimations() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY,
              ITEM_ANIMATOR_DEFAULT_RUN_PENDING_ANIMATIONS);

  if (!has_pending_animations()) {
    // An empty pending set still completes synchronously so AnimationManager
    // can finish the transaction and recycle deferred ItemHolders.
    DispatchAnimationFinishedIfNeeded();
    return;
  }

  std::vector<RemoveInfo> pending_removals;
  std::vector<AddInfo> pending_adds;
  std::vector<MoveInfo> pending_moves;
  std::vector<ChangeInfo> pending_changes;
  pending_removals.swap(pending_removals_);
  pending_adds.swap(pending_adds_);
  pending_moves.swap(pending_moves_);
  pending_changes.swap(pending_changes_);

  const bool has_pending_removals = !pending_removals.empty();
  const bool has_pending_moves = !pending_moves.empty();
  const bool has_pending_changes = !pending_changes.empty();
  int32_t move_change_delay = 0;
  int32_t move_duration = 0;
  int32_t change_duration = 0;
  if (has_pending_removals) {
    move_change_delay = std::max(0, remove_duration_ms());
  }
  if (has_pending_moves) {
    move_duration = std::max(0, move_duration_ms());
  }
  if (has_pending_changes) {
    change_duration = std::max(0, change_duration_ms());
  }
  int32_t add_delay =
      move_change_delay + std::max(move_duration, change_duration);

  {
    // Start() can complete a zero-duration animation synchronously. Defer the
    // batch-completion notification until every pending item has been given a
    // chance to start.
    in_starting_animations_ = true;
    for (const RemoveInfo& info : pending_removals) {
      StartRemoveAnimation(info, 0);
    }
    for (const MoveInfo& info : pending_moves) {
      StartMoveAnimation(info, move_change_delay);
    }
    for (const ChangeInfo& info : pending_changes) {
      StartChangeAnimation(info, move_change_delay);
    }
    for (const AddInfo& info : pending_adds) {
      StartAddAnimation(info, add_delay);
    }
    in_starting_animations_ = false;
  }

  // After startup is unblocked, complete the batch if every animation finished
  // synchronously or no running animation was created.
  DispatchAnimationFinishedIfNeeded();
}

bool ItemAnimatorDefault::HasInvalidatedRunningAnimationLayout() const {
  for (const auto& entry : running_animations_) {
    const RunningAnimation& running_animation = entry.second;

    // Only Persistence currently interpolates between PRE and POST positions.
    // Add and Remove use only opacity, so List or item size changes do not
    // invalidate them.
    if (running_animation.type != ItemAnimationType::kPersistence) {
      continue;
    }

    const AnimationTarget* target = running_animation.target.get();
    if (!target) {
      continue;
    }

    const ItemLayoutInfo latest_layout_info = GetItemLayoutInfo(*target);

    // post_layout_info is the endpoint captured when this animation was
    // created. The target now exposes the latest logical position produced by
    // layout. Compare only position so size-only changes do not cancel the
    // transaction.
    if (running_animation.post_layout_info.PositionChanged(
            latest_layout_info)) {
      return true;
    }
  }

  return false;
}

void ItemAnimatorDefault::CancelAnimations(bool destroy) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ITEM_ANIMATOR_DEFAULT_CANCEL_ANIMATIONS);

  in_cancelling_animations_ = true;
  {
    /*
     * AnimationManager first calls AnimateXxx() to enqueue an accepted
     * animation. Only a true result is followed by PrepareForAnimation(),
     * which marks the target as being in its animation lifecycle.
     *
     * A pending animation has not written any presentation state. Cancelling
     * it therefore does not reset opacity or position; it only calls
     * FinishAnimation() for each live target and clears the pending queues.
     */
    CancelPendingAnimations();

    // The copy retains shared animators and copies weak targets and value
    // snapshots; it does not retain targets. DestroyAnimation() can dispatch
    // Cancel synchronously and erase the corresponding member-map entry, so
    // the member map cannot be iterated directly.
    auto running_animations = running_animations_;

    if (destroy) {
      for (auto& [target_key, running_animation] : running_animations) {
        if (running_animation.animator) {
          running_animation.animator->RegisterCustomCallback(
              [](float progress) {});

          running_animation.animator->RegisterEventCallback(
              []() {}, ::lynx::animation::basic::Animation::EventType::Start);

          running_animation.animator->RegisterEventCallback(
              []() {}, ::lynx::animation::basic::Animation::EventType::End);

          running_animation.animator->RegisterEventCallback(
              []() {}, ::lynx::animation::basic::Animation::EventType::Cancel);
          running_animation.animator->DestroyAnimation();
        }
      }

    } else {
      for (auto& [target_key, running_animation] : running_animations) {
        if (running_animation.animator) {
          running_animation.animator->DestroyAnimation();
        }
      }

      // DestroyAnimation() may remove a member record through its synchronous
      // Cancel callback. For each record left behind, restore its live target
      // to the final state and finish its animation lifecycle.
      for (auto& [target_key, running_animation] : running_animations_) {
        if (AnimationTarget* target = running_animation.target.get()) {
          ResetTargetToFinalState(target, running_animation.type);
          target->FinishAnimation();
        }
      }
    }

    // Drop records whose callbacks were disabled or did not erase them.
    running_animations_.clear();
  }
  in_cancelling_animations_ = false;
}

/*
 * Remove currently has zero delay. Before starting it, write the initial
 * visible state into the batch's deferred flush. LynxBasicAnimator::Start()
 * processes a dummy frame synchronously. When duration is positive and delay
 * is zero, the custom callback writes the same progress=0 state again. Both
 * writes occur while in_starting_animations_ is true and do not flush
 * individually. With zero duration, the dummy frame evaluates the final state
 * directly and may complete the animation inside Start().
 */
void ItemAnimatorDefault::StartRemoveAnimation(const RemoveInfo& info,
                                               int32_t delay_ms) {
  AnimationTarget* target = info.target.get();
  if (!target) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ItemAnimatorDefault::StartRemoveAnimation");

  starlight::AnimationData animation_data;
  animation_data.duration = std::max<int32_t>(0, remove_duration_ms());
  animation_data.delay = std::max<int32_t>(0, delay_ms);
  animation_data.fill_mode = starlight::AnimationFillModeType::kForwards;
  animation_data.timing_func.timing_func =
      starlight::TimingFunctionType::kEaseInEaseOut;
  auto animator = CreateBasicAnimator(std::move(animation_data));

  const AnimationTargetKey target_key =
      reinterpret_cast<AnimationTargetKey>(target);
  const AnimationId animation_id = ++next_animation_id_;
  fml::WeakPtr<ItemAnimatorDefault> weak_self = WeakFromThis();

  animator->RegisterCustomCallback(
      [weak_self, target_key, animation_id](float progress) {
        ItemAnimatorDefault* self = weak_self.get();
        if (self) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          AnimationTarget* target = nullptr;
          if (running_animation && (target = running_animation->target.get())) {
            self->ApplyRemoveFrame(target, progress);
            // TODO: send custom event.
          }
        }
      });

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        ItemAnimatorDefault* self = weak_self.get();
        if (self) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          AnimationTarget* target = nullptr;
          if (running_animation && (target = running_animation->target.get())) {
            // TODO: send start event.
          }
        }
      },
      ::lynx::animation::basic::Animation::EventType::Start);

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          self->FinishRunningAnimation(target_key, animation_id, false);
        }
      },
      ::lynx::animation::basic::Animation::EventType::End);

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          self->FinishRunningAnimation(target_key, animation_id, true);
        }
      },
      ::lynx::animation::basic::Animation::EventType::Cancel);

  running_animations_.insert_or_assign(
      target_key, RunningAnimation{
                      .animation_id = animation_id,
                      .type = ItemAnimationType::kDisappearance,
                      .target = info.target,
                      .pre_layout_info = info.pre_layout_info,
                      .animator = animator,
                  });

  ApplyRemoveFrame(target, 0.f);
  animator->Start();
}

void ItemAnimatorDefault::StartAddAnimation(const AddInfo& info,
                                            int32_t delay_ms) {
  AnimationTarget* target = info.target.get();
  if (!target) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ItemAnimatorDefault::StartAddAnimation");

  starlight::AnimationData animation_data;
  animation_data.duration = std::max<int32_t>(0, add_duration_ms());
  animation_data.delay = std::max<int32_t>(0, delay_ms);
  animation_data.fill_mode = starlight::AnimationFillModeType::kForwards;
  animation_data.timing_func.timing_func =
      starlight::TimingFunctionType::kEaseInEaseOut;
  auto animator = CreateBasicAnimator(std::move(animation_data));

  const AnimationTargetKey target_key =
      reinterpret_cast<AnimationTargetKey>(target);
  const AnimationId animation_id = ++next_animation_id_;
  fml::WeakPtr<ItemAnimatorDefault> weak_self = WeakFromThis();

  animator->RegisterCustomCallback(
      [weak_self, target_key, animation_id](float progress) {
        ItemAnimatorDefault* self = weak_self.get();
        if (self) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          AnimationTarget* target = nullptr;
          if (running_animation && (target = running_animation->target.get())) {
            self->ApplyAddFrame(target, progress);
            // TODO: send custom event.
          }
        }
      });

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        ItemAnimatorDefault* self = weak_self.get();
        if (self) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          AnimationTarget* target = nullptr;
          if (running_animation && (target = running_animation->target.get())) {
            // TODO: send start event.
          }
        }
      },
      ::lynx::animation::basic::Animation::EventType::Start);

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          self->FinishRunningAnimation(target_key, animation_id, false);
        }
      },
      ::lynx::animation::basic::Animation::EventType::End);

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          self->FinishRunningAnimation(target_key, animation_id, true);
        }
      },
      ::lynx::animation::basic::Animation::EventType::Cancel);

  running_animations_.insert_or_assign(
      target_key, RunningAnimation{
                      .animation_id = animation_id,
                      .type = ItemAnimationType::kAppearance,
                      .target = info.target,
                      .post_layout_info = info.post_layout_info,
                      .animator = animator,
                  });

  ApplyAddFrame(target, 0.f);
  animator->Start();
}

void ItemAnimatorDefault::StartMoveAnimation(const MoveInfo& info,
                                             int32_t delay_ms) {
  AnimationTarget* target = info.target.get();
  if (!target) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ItemAnimatorDefault::StartMoveAnimation");

  starlight::AnimationData animation_data;
  animation_data.duration = std::max<int32_t>(0, move_duration_ms());
  animation_data.delay = std::max<int32_t>(0, delay_ms);
  animation_data.fill_mode = starlight::AnimationFillModeType::kForwards;
  animation_data.timing_func.timing_func =
      starlight::TimingFunctionType::kEaseInEaseOut;
  auto animator = CreateBasicAnimator(std::move(animation_data));

  const AnimationTargetKey target_key =
      reinterpret_cast<AnimationTargetKey>(target);
  const AnimationId animation_id = ++next_animation_id_;
  fml::WeakPtr<ItemAnimatorDefault> weak_self = WeakFromThis();

  animator->RegisterCustomCallback(
      [weak_self, target_key, animation_id](float progress) {
        ItemAnimatorDefault* self = weak_self.get();
        if (self) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          AnimationTarget* target = nullptr;
          if (running_animation && (target = running_animation->target.get())) {
            self->ApplyMoveFrame(target, running_animation->pre_layout_info,
                                 running_animation->post_layout_info, progress);
            // TODO: send custom event.
          }
        }
      });

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        ItemAnimatorDefault* self = weak_self.get();
        if (self) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          AnimationTarget* target = nullptr;
          if (running_animation && (target = running_animation->target.get())) {
            // TODO: send start event.
          }
        }
      },
      ::lynx::animation::basic::Animation::EventType::Start);

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          self->FinishRunningAnimation(target_key, animation_id, false);
        }
      },
      ::lynx::animation::basic::Animation::EventType::End);

  animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          self->FinishRunningAnimation(target_key, animation_id, true);
        }
      },
      ::lynx::animation::basic::Animation::EventType::Cancel);

  running_animations_.insert_or_assign(
      target_key, RunningAnimation{
                      .animation_id = animation_id,
                      .type = ItemAnimationType::kPersistence,
                      .target = info.target,
                      .pre_layout_info = info.pre_layout_info,
                      .post_layout_info = info.post_layout_info,
                      .animator = animator,
                  });

  ApplyMoveFrame(target, info.pre_layout_info, info.post_layout_info, 0.f);
  animator->Start();
}

void ItemAnimatorDefault::ApplyRemoveFrame(AnimationTarget* target,
                                           float progress) {
  const bool flush_immediately =
      !in_starting_animations_ && !in_cancelling_animations_;
  target->UpdateAnimationOpacity(1.f - std::clamp(progress, 0.f, 1.f),
                                 flush_immediately);
}

void ItemAnimatorDefault::ApplyAddFrame(AnimationTarget* target,
                                        float progress) {
  const bool flush_immediately =
      !in_starting_animations_ && !in_cancelling_animations_;
  target->UpdateAnimationOpacity(std::clamp(progress, 0.f, 1.f),
                                 flush_immediately);
}

void ItemAnimatorDefault::ApplyMoveFrame(AnimationTarget* target,
                                         const ItemLayoutInfo& pre_layout_info,
                                         const ItemLayoutInfo& post_layout_info,
                                         float progress) {
  const float normalized_progress = std::clamp(progress, 0.f, 1.f);
  const float left =
      pre_layout_info.left_ +
      (post_layout_info.left_ - pre_layout_info.left_) * normalized_progress;
  const float top =
      pre_layout_info.top_ +
      (post_layout_info.top_ - pre_layout_info.top_) * normalized_progress;
  const bool flush_immediately =
      !in_starting_animations_ && !in_cancelling_animations_;
  target->UpdateAnimationPosition(left, top, flush_immediately);
}

ItemAnimatorDefault::RunningAnimation*
ItemAnimatorDefault::FindRunningAnimation(AnimationTargetKey target_key,
                                          AnimationId animation_id) {
  auto it = running_animations_.find(target_key);
  if (it == running_animations_.end() ||
      it->second.animation_id != animation_id) {
    return nullptr;
  }
  return &it->second;
}

void ItemAnimatorDefault::FinishRunningAnimation(AnimationTargetKey target_key,
                                                 AnimationId animation_id,
                                                 bool cancelled) {
  auto it = running_animations_.find(target_key);
  if (it == running_animations_.end() ||
      it->second.animation_id != animation_id) {
    return;
  }
  RunningAnimation animation = std::move(it->second);
  running_animations_.erase(it);
  AnimationTarget* target = animation.target.get();
  if (target) {
    // End normally follows a progress=1 sample, while Cancel can occur at any
    // progress. Both paths explicitly restore the final state before ending
    // the target's animation lifecycle.
    ResetTargetToFinalState(target, animation.type);
    target->FinishAnimation();
  }
  DispatchAnimationFinishedIfNeeded();
}

void ItemAnimatorDefault::ResetTargetToFinalState(
    AnimationTarget* target, ItemAnimationType animation_type) {
  const bool flush_immediately =
      !in_starting_animations_ && !in_cancelling_animations_;
  switch (animation_type) {
    case ItemAnimationType::kAppearance:
      target->UpdateAnimationOpacity(1.f, flush_immediately);
      break;
    case ItemAnimationType::kDisappearance:
      target->UpdateAnimationOpacity(0.f, flush_immediately);
      break;
    case ItemAnimationType::kPersistence:
    case ItemAnimationType::kChange:
      target->UpdateAnimationPosition(target->GetAnimationLeft(),
                                      target->GetAnimationTop(),
                                      flush_immediately);
      break;
  }
}

void ItemAnimatorDefault::CancelPendingAnimations() {
  auto cancel_pending_animation = [](const WeakAnimationTarget& weak_target) {
    if (AnimationTarget* target = weak_target.get()) {
      target->FinishAnimation();
    }
  };
  for (const RemoveInfo& info : pending_removals_) {
    cancel_pending_animation(info.target);
  }
  for (const AddInfo& info : pending_adds_) {
    cancel_pending_animation(info.target);
  }
  for (const MoveInfo& info : pending_moves_) {
    cancel_pending_animation(info.target);
  }
  for (const ChangeInfo& info : pending_changes_) {
    cancel_pending_animation(info.target);
  }
  pending_removals_.clear();
  pending_adds_.clear();
  pending_moves_.clear();
  pending_changes_.clear();
}

void ItemAnimatorDefault::DispatchAnimationFinishedIfNeeded() {
  // Do not dispatch normal batch completion during batch startup or explicit
  // cancellation.
  if (in_starting_animations_ || in_cancelling_animations_ ||
      has_pending_animations() || has_running_animations()) {
    return;
  }
  if (listener()) {
    listener()->OnAllAnimationsFinished();
  }
}

}  // namespace list
}  // namespace lynx
