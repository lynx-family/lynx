// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animator_default.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "base/include/float_comparison.h"
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
  pending_removals_.emplace_back(PendingAnimationInfo{
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
  pending_adds_.emplace_back(PendingAnimationInfo{
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
  pending_moves_.emplace_back(PendingAnimationInfo{
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

  std::vector<PendingAnimationInfo> pending_removals;
  std::vector<PendingAnimationInfo> pending_adds;
  std::vector<PendingAnimationInfo> pending_moves;
  std::vector<PendingAnimationInfo> pending_changes;
  pending_removals.swap(pending_removals_);
  pending_adds.swap(pending_adds_);
  pending_moves.swap(pending_moves_);
  pending_changes.swap(pending_changes_);

  // Discard expired targets; only types with live targets contribute to stage
  // timing.
  const auto remove_invalid_targets = [](auto& pending) {
    pending.erase(std::remove_if(pending.begin(), pending.end(),
                                 [](const auto& info) { return !info.target; }),
                  pending.end());
  };
  remove_invalid_targets(pending_removals);
  remove_invalid_targets(pending_moves);
  remove_invalid_targets(pending_changes);
  remove_invalid_targets(pending_adds);

  // Initialize per-type state.
  animation_type_states_ = {
      {ItemAnimationType::kDisappearance, {}},
      {ItemAnimationType::kPersistence, {}},
      {ItemAnimationType::kChange, {}},
      {ItemAnimationType::kAppearance, {}},
  };

  // No running records exist yet; use local pending queues to determine stage
  // participation.
  const auto has_pending_target = [&pending_removals, &pending_moves,
                                   &pending_changes,
                                   &pending_adds](ItemAnimationType type) {
    switch (type) {
      case ItemAnimationType::kDisappearance:
        return !pending_removals.empty();
      case ItemAnimationType::kPersistence:
        return !pending_moves.empty();
      case ItemAnimationType::kChange:
        return !pending_changes.empty();
      case ItemAnimationType::kAppearance:
        return !pending_adds.empty();
      default:
        return false;
    }
  };
  // Entries share a stage delay; the next stage waits for the longest
  // participating animation.
  // Stages without targets add no delay, avoiding a removal wait for add-only
  // batches.
  int64_t stage_delay_ms = 0;
  for (const auto& animation_stage : animation_stages()) {
    // animation_stage, eg:
    // {animations:['move', 'add'], durations: [100, 200]}
    int32_t stage_duration_ms = 0;
    for (const auto& entry : animation_stage) {
      if (!has_pending_target(entry.type)) {
        continue;
      }
      auto& animation_type_state = animation_type_states_.at(entry.type);
      animation_type_state.delay_ms = static_cast<int32_t>(stage_delay_ms);
      animation_type_state.duration_ms =
          std::max<int32_t>(0, entry.duration_ms);
      DLIST_LOGD("[" << this
                     << "] ItemAnimatorDefault::RunPendingAnimations: type="
                     << ItemAnimationTypeToString(entry.type)
                     << ", delay_ms=" << animation_type_state.delay_ms
                     << ", duration_ms=" << animation_type_state.duration_ms);
      stage_duration_ms =
          std::max(stage_duration_ms, animation_type_state.duration_ms);
    }
    stage_delay_ms = std::min<int64_t>(std::numeric_limits<int32_t>::max(),
                                       stage_delay_ms + stage_duration_ms);
  }

  {
    // Start() can complete a zero-duration animation synchronously. Defer the
    // batch-completion notification until every pending item has been given a
    // chance to start.
    in_starting_animations_ = true;
    for (const PendingAnimationInfo& info : pending_removals) {
      StartRemoveAnimation(info);
    }
    for (const PendingAnimationInfo& info : pending_moves) {
      StartMoveAnimation(info);
    }
    for (const PendingAnimationInfo& info : pending_changes) {
      StartChangeAnimation(info);
    }
    for (const PendingAnimationInfo& info : pending_adds) {
      StartAddAnimation(info);
    }
    in_starting_animations_ = false;
  }

  // Zero-duration animations without delay can finish synchronously in Start().
  // End notifications are deferred during startup because later items of the
  // same type may not exist yet. Recheck now to send those deferred
  // notifications.
  for (const auto& [type, state] : animation_type_states_) {
    DispatchAnimationEndIfNeeded(type);
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ITEM_ANIMATOR_DEFAULT_CANCEL_ANIMATIONS,
              "destroy", destroy);
  in_cancelling_animations_ = true;
  {
    // AnimationManager first calls AnimateXxx() to enqueue an accepted
    // animation. Only a true result is followed by PrepareForAnimation(),
    // which marks the target as being in its animation lifecycle.
    // A pending animation has not written any presentation state. Cancelling
    // it therefore does not reset opacity or position; it only calls
    // FinishAnimation() for each live target and clears the pending queues.
    CancelPendingAnimations();

    // The copy retains shared animators and copies weak targets and value
    // snapshots; it does not retain targets. DestroyAnimation() can dispatch
    // Cancel synchronously and erase the corresponding member-map entry, so
    // the member map cannot be iterated directly.
    auto running_animations = running_animations_;

    if (destroy) {
      for (auto& [target_key, running_animation] : running_animations) {
        if (running_animation.animator) {
          running_animation.animator->RegisterCustomCallback({});
          running_animation.animator->RegisterEventCallback(
              {}, ::lynx::animation::basic::Animation::EventType::Start);
          running_animation.animator->RegisterEventCallback(
              {}, ::lynx::animation::basic::Animation::EventType::End);
          running_animation.animator->RegisterEventCallback(
              {}, ::lynx::animation::basic::Animation::EventType::Cancel);
          running_animation.animator->DestroyAnimation();
        }
      }
      running_animations_.clear();
    } else {
      // Cleanup decrements remaining_items; first record started, unfinished
      // types. Record each type once; delayed and normally finished types do
      // not send cancel.
      std::vector<ItemAnimationType> cancelled_types;
      for (const auto& [type, state] : animation_type_states_) {
        if (state.has_started && state.remaining_items > 0) {
          cancelled_types.push_back(type);
        }
      }
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
      // Clear running_animations_ last because non-destroy cancellation needs
      // it inside cancel callbacks.
      running_animations_.clear();
      // Item Cancel callbacks only clean up; notify the frontend after batch
      // cleanup.
      DispatchAnimationCancelIfNeeded(cancelled_types);
    }
  }
  in_cancelling_animations_ = false;
}

void ItemAnimatorDefault::StartRemoveAnimation(
    const PendingAnimationInfo& info) {
  const auto& state =
      animation_type_states_.at(ItemAnimationType::kDisappearance);
  StartAnimation(
      RunningAnimation{
          .type = ItemAnimationType::kDisappearance,
          .target = info.target,
          .pre_layout_info = info.pre_layout_info,
      },
      state.duration_ms, state.delay_ms);
}

void ItemAnimatorDefault::StartAddAnimation(const PendingAnimationInfo& info) {
  const auto& state = animation_type_states_.at(ItemAnimationType::kAppearance);
  StartAnimation(
      RunningAnimation{
          .type = ItemAnimationType::kAppearance,
          .target = info.target,
          .post_layout_info = info.post_layout_info,
      },
      state.duration_ms, state.delay_ms);
}

void ItemAnimatorDefault::StartMoveAnimation(const PendingAnimationInfo& info) {
  const auto& state =
      animation_type_states_.at(ItemAnimationType::kPersistence);
  StartAnimation(
      RunningAnimation{
          .type = ItemAnimationType::kPersistence,
          .target = info.target,
          .pre_layout_info = info.pre_layout_info,
          .post_layout_info = info.post_layout_info,
      },
      state.duration_ms, state.delay_ms);
}

void ItemAnimatorDefault::StartAnimation(RunningAnimation animation,
                                         int32_t duration_ms,
                                         int32_t delay_ms) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, ITEM_ANIMATOR_DEFAULT_START_ANIMATION,
              "type", static_cast<int>(animation.type), "duration_ms",
              duration_ms, "delay_ms", delay_ms);
  AnimationTarget* target = animation.target.get();
  if (!target) {
    return;
  }
  // create BasicAnimator.
  starlight::AnimationData animation_data;
  animation_data.duration = std::max<int32_t>(0, duration_ms);
  animation_data.delay = std::max<int32_t>(0, delay_ms);
  animation_data.fill_mode = starlight::AnimationFillModeType::kForwards;
  animation_data.timing_func.timing_func =
      starlight::TimingFunctionType::kEaseInEaseOut;
  auto basic_animator = CreateBasicAnimator(std::move(animation_data));

  const AnimationTargetKey target_key =
      reinterpret_cast<AnimationTargetKey>(target);
  const AnimationId animation_id = ++next_animation_id_;
  DLIST_LOGD(
      "[" << this << "] ItemAnimatorDefault::StartAnimation: animation_id="
          << animation_id << ", target=" << target
          << ", type=" << ItemAnimationTypeToString(animation.type)
          << ", duration_ms=" << duration_ms << ", delay_ms=" << delay_ms);
  fml::WeakPtr<ItemAnimatorDefault> weak_self = WeakFromThis();
  using EventType = ::lynx::animation::basic::Animation::EventType;

  // custom callback
  basic_animator->RegisterCustomCallback(
      [weak_self, target_key, animation_id](float progress) {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          if (running_animation && running_animation->target) {
            self->ApplyAnimationFrame(*running_animation, progress);
            self->DispatchAnimationIterationIfNeeded(running_animation->type,
                                                     progress);
          }
        }
      });

  // start callback
  basic_animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          if (running_animation && running_animation->target) {
            self->DispatchAnimationStartIfNeeded(running_animation->type);
          }
        }
      },
      EventType::Start);

  // end callback
  basic_animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          if (running_animation) {
            self->FinishRunningAnimation(target_key,
                                         std::move(*running_animation), false);
            running_animation = nullptr;
          }
        }
      },
      EventType::End);

  // cancel callback
  basic_animator->RegisterEventCallback(
      [weak_self, target_key, animation_id]() {
        if (ItemAnimatorDefault* self = weak_self.get()) {
          RunningAnimation* running_animation =
              self->FindRunningAnimation(target_key, animation_id);
          if (running_animation) {
            self->FinishRunningAnimation(target_key,
                                         std::move(*running_animation), true);
            running_animation = nullptr;
          }
        }
      },
      EventType::Cancel);

  // Register the record before Start(), which can invoke callbacks and finish
  // a zero-duration animation synchronously. The local animator keeps it alive
  // even if a synchronous callback removes the running record.
  animation.animation_id = animation_id;
  animation.animator = basic_animator;
  auto result =
      running_animations_.insert_or_assign(target_key, std::move(animation));

  ++animation_type_states_.at(result.first->second.type).remaining_items;
  ApplyAnimationFrame(result.first->second, 0.f);
  basic_animator->Start();
}

void ItemAnimatorDefault::ApplyAnimationFrame(const RunningAnimation& animation,
                                              float progress) {
  AnimationTarget* target = animation.target.get();
  if (target) {
    switch (animation.type) {
      case ItemAnimationType::kDisappearance:
        ApplyRemoveFrame(target, progress);
        break;
      case ItemAnimationType::kAppearance:
        ApplyAddFrame(target, progress);
        break;
      case ItemAnimationType::kPersistence:
        ApplyMoveFrame(target, animation.pre_layout_info,
                       animation.post_layout_info, progress);
        break;
      case ItemAnimationType::kChange:
        // TODO: impl change animation
        break;
    }
  }
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
                                                 RunningAnimation animation,
                                                 bool cancelled) {
  running_animations_.erase(target_key);
  auto state_it = animation_type_states_.find(animation.type);
  if (state_it != animation_type_states_.end() &&
      state_it->second.remaining_items > 0) {
    --state_it->second.remaining_items;
  }
  AnimationTarget* target = animation.target.get();
  if (target) {
    // End normally follows a progress=1 sample, while Cancel can occur at any
    // progress. Both paths explicitly restore the final state before ending
    // the target's animation lifecycle.
    ResetTargetToFinalState(target, animation.type);
    target->FinishAnimation();
  }
  if (!cancelled) {
    DispatchAnimationEndIfNeeded(animation.type);
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
  for (const PendingAnimationInfo& info : pending_removals_) {
    cancel_pending_animation(info.target);
  }
  for (const PendingAnimationInfo& info : pending_adds_) {
    cancel_pending_animation(info.target);
  }
  for (const PendingAnimationInfo& info : pending_moves_) {
    cancel_pending_animation(info.target);
  }
  for (const PendingAnimationInfo& info : pending_changes_) {
    cancel_pending_animation(info.target);
  }
  pending_removals_.clear();
  pending_adds_.clear();
  pending_moves_.clear();
  pending_changes_.clear();
}

void ItemAnimatorDefault::DispatchAnimationStartIfNeeded(
    ItemAnimationType type) {
  auto& state = animation_type_states_.at(type);
  if (!state.has_started) {
    state.has_started = true;
    if (listener()) {
      listener()->OnAnimationStart(this, type);
    }
  }
}

void ItemAnimatorDefault::DispatchAnimationIterationIfNeeded(
    ItemAnimationType type, float progress) {
  if (in_cancelling_animations_) {
    return;
  }
  auto& state = animation_type_states_.at(type);
  // Allow iteration only after the underlying Start callback reports this type.
  // Discard pre-start notifications without recording progress for
  // deduplication.
  if (!state.has_started) {
    return;
  }
  if (state.last_iteration_progress.has_value()) {
    const float last_progress = *state.last_iteration_progress;
    // Merge notifications when progress for the same type differs by less than
    // 1e-6.
    if (base::DoublesEqual(last_progress, progress)) {
      return;
    }
  }
  // Deduplicate notifications only; the caller has already applied the item
  // sample. Save before the callback to filter later progress within the
  // tolerance.
  state.last_iteration_progress = progress;
  if (listener()) {
    listener()->OnAnimationUpdate(this, type, progress);
  }
}

void ItemAnimatorDefault::DispatchAnimationEndIfNeeded(ItemAnimationType type) {
  // During startup, later items may not exist yet, so zero does not mean
  // completion. During cancellation, reaching zero must not send a normal end
  // event.
  if (in_starting_animations_ || in_cancelling_animations_) {
    return;
  }
  const auto& state = animation_type_states_.at(type);
  if (!state.has_started || state.remaining_items != 0) {
    return;
  }
  if (listener()) {
    listener()->OnAnimationEnd(this, type);
  }
}

void ItemAnimatorDefault::DispatchAnimationCancelIfNeeded(
    const std::vector<ItemAnimationType>& cancelled_types) {
  if (listener()) {
    for (ItemAnimationType type : cancelled_types) {
      listener()->OnAnimationCancel(this, type);
    }
  }
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
