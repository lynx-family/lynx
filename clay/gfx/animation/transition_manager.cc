// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/transition_manager.h"

#include <string>

#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/animator_listener_adapter.h"
#include "clay/gfx/animation/interpolator.h"
#include "clay/gfx/geometry/transform_operations.h"

namespace clay {

TransitionManager::TransitionManager(AnimatorTarget* target)
    : target_(target) {}
TransitionManager::~TransitionManager() { CancelAllAnimators(); }

void TransitionManager::AppendData(const TransitionData& data) {
  EndAllAnimators();
  data_.push_back(data);
}

void TransitionManager::UpdateData(const std::vector<TransitionData>& data) {
  EndAllAnimators();
  data_ = data;
}

bool TransitionManager::Enabled(ClayAnimationPropertyType type) const {
  for (auto& transition : data_) {
    if (static_cast<int>(transition.property) & static_cast<int>(type)) {
      return true;
    }
  }
  return false;
}

bool TransitionManager::HasAnimationRunning() const {
  for (const auto& it : active_transitions_) {
    if (it.second.first->IsStarted()) {
      return true;
    }
  }
  return false;
}

bool TransitionManager::IsAnimationRunning(ClayAnimationPropertyType type) {
  auto it = active_transitions_.find(type);
  if (it != active_transitions_.end()) {
    ValueAnimator* animator = it->second.first.get();
    return animator->IsStarted();
  }
  return false;
}

std::vector<ValueAnimator*> TransitionManager::GetRunningAnimators() {
  std::vector<ValueAnimator*> animators;
  for (const auto& [type, transition] : active_transitions_) {
    if (transition.first->IsRunning()) {
      animators.push_back(transition.first.get());
    }
  }
  return animators;
}

void TransitionManager::EndAllAnimators() {
  for (auto& it : active_transitions_) {
    ValueAnimator* animator = it.second.first.get();
    if (animator->IsStarted()) {
      animator->End();
    }
  }
  active_transitions_.clear();
}

void TransitionManager::EndAnimator(ClayAnimationPropertyType type) {
  auto it = active_transitions_.find(type);
  if (it != active_transitions_.end()) {
    ValueAnimator* animator = it->second.first.get();
    if (animator->IsStarted()) {
      animator->End();
    }
  }
}

void TransitionManager::CancelAnimator(ClayAnimationPropertyType type) {
  auto it = active_transitions_.find(type);
  if (it != active_transitions_.end()) {
    ValueAnimator* animator = it->second.first.get();
    if (animator->IsStarted()) {
      animator->Cancel();
    }
  }
}

void TransitionManager::CancelAllAnimators() {
  for (auto& it : active_transitions_) {
    ValueAnimator* animator = it.second.first.get();
    if (animator->IsStarted()) {
      animator->Cancel();
    }
  }
  active_transitions_.clear();
}

void TransitionManager::OnAnimationStart(const Animator& animation,
                                         ClayAnimationPropertyType type) {
  if (event_handler_) {
    const std::string& animation_name = animation.GetAnimationName();
    AnimationParams params{ClayEventType::kClayEventTypeTransitionStart,
                           animation_name.c_str()};
    event_handler_->OnTransitionEvent(params, type);
  }
}
void TransitionManager::OnAnimationEnd(const Animator& animation,
                                       ClayAnimationPropertyType type) {
  if (event_handler_) {
    const std::string& animation_name = animation.GetAnimationName();
    AnimationParams params{ClayEventType::kClayEventTypeTransitionEnd,
                           animation_name.c_str()};
    event_handler_->OnTransitionEvent(params, type);
  }
}

void TransitionManager::SetEventHandler(AnimationEventHandler* event_handler) {
  event_handler_ = event_handler;
}

bool TransitionManager::StartListenersNotified(
    ClayAnimationPropertyType type) const {
  if (auto iter = active_transitions_.find(type);
      iter != active_transitions_.end()) {
    return !iter->second.first->StartListenersCalled();
  }
  return true;
}

std::unique_ptr<TransitionManager> TransitionManager::CloneForRasterAnimation(
    ClayAnimationPropertyType type, AnimatorTarget* target) const {
  std::unique_ptr<TransitionManager> clone;
  if (auto iter = active_transitions_.find(type);
      iter != active_transitions_.end()) {
    clone = std::make_unique<TransitionManager>(target);
    std::unique_ptr<AnimatorListenerAdapter> listener;
    if (type == ClayAnimationPropertyType::kOpacity) {
      listener =
          static_cast<TransitionListener<float>*>(iter->second.second.get())
              ->CloneForRasterAnimation(clone.get());
    } else if (type == ClayAnimationPropertyType::kTransform) {
      listener = static_cast<TransitionListener<TransformOperations>*>(
                     iter->second.second.get())
                     ->CloneForRasterAnimation(clone.get());
    } else if (type == ClayAnimationPropertyType::kBackgroundColor ||
               type == ClayAnimationPropertyType::kColor) {
      listener =
          static_cast<TransitionListener<Color>*>(iter->second.second.get())
              ->CloneForRasterAnimation(clone.get());
    } else {
      return nullptr;
    }
    auto animator = iter->second.first->Clone();
    animator->AddListener(listener.get());
    animator->AddUpdateListener(listener.get());
    clone->active_transitions_.emplace(
        type, std::make_pair(std::move(animator), std::move(listener)));
  }
  return clone;
}

void TransitionManager::SyncProperties(TransitionManager* manager) {
  if (manager == nullptr) {
    return;
  }
  for (auto& transition : active_transitions_) {
    if (auto prev_transition =
            manager->active_transitions_.find(transition.first);
        prev_transition != active_transitions_.end()) {
      transition.second.first->SetStartListenersCalled(
          prev_transition->second.first->StartListenersCalled());
    }
  }
}

}  // namespace clay
