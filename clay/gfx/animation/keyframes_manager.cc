// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/animation/keyframes_manager.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

#include "clay/gfx/animation/animation_properties_util.h"
#include "clay/gfx/animation/animator_target.h"
#include "clay/public/clay.h"

namespace clay {

namespace {

// Compare animation data properties except play_state
bool IsSame(const AnimationData& lhs, const AnimationData& rhs) {
  return std::tie(lhs.name, lhs.timing_func, lhs.iteration_count, lhs.fill_mode,
                  lhs.duration, lhs.delay, lhs.direction) ==
         std::tie(rhs.name, rhs.timing_func, rhs.iteration_count, rhs.fill_mode,
                  rhs.duration, rhs.delay, rhs.direction);
}

// Returns true when the view has both raster animation and UI animation
bool IsMixedAnimation(const KeyframesManager::KeyframeAnimation& animation) {
  bool has_raster_animation = false;
  bool has_ui_animation = false;
  for (auto& [type, keyframe] : animation.keyframes_map) {
    if (IsRasterAnimationProperty(type)) {
      has_raster_animation = true;
    } else {
      has_ui_animation = true;
    }
    if (has_raster_animation && has_ui_animation) {
      return true;
    }
  }
  return false;
}

std::vector<AnimationData> NormalizeAnimationData(
    const std::vector<AnimationData>& data) {
  std::vector<AnimationData> normalized;
  std::unordered_set<std::string> seen_names;
  for (auto it = data.rbegin(); it != data.rend(); ++it) {
    AnimationData item = *it;
    if (item.iteration_count < 0) {
      continue;
    }
    if (item.duration < 0) {
      item.duration = 0;
    }
    if (!seen_names.insert(item.name).second) {
      continue;
    }
    normalized.push_back(std::move(item));
  }
  std::reverse(normalized.begin(), normalized.end());
  return normalized;
}

}  // namespace

KeyframesManager::KeyframesManager(AnimatorTarget* target) : target_(target) {}

KeyframesManager::~KeyframesManager() { CancelAllAnimators(); }

KeyframesManager::UpdateDataResult KeyframesManager::UpdateData(
    const std::vector<AnimationData>& data) {
  std::vector<AnimationData> normalized_data = NormalizeAnimationData(data);
  bool data_has_changed = normalized_data.size() != animations_.size();
  for (size_t i = 0; i < normalized_data.size(); ++i) {
    if (data_has_changed) {
      break;
    }
    data_has_changed = !IsSame(normalized_data[i], animations_[i].data);
  }
  if (!data_has_changed) {
    // Check if play_state has changed.
    bool play_state_has_changed = false;
    for (size_t i = 0; i < normalized_data.size(); ++i) {
      if (normalized_data[i].play_state == animations_[i].data.play_state) {
        continue;
      }
      if (normalized_data[i].play_state ==
          ClayAnimationPlayStateType::kPaused) {
        animations_[i].animator->Pause();
      } else if (normalized_data[i].play_state ==
                 ClayAnimationPlayStateType::kRunning) {
        animations_[i].animator->Resume();
      }
      animations_[i].data.play_state = normalized_data[i].play_state;
      play_state_has_changed = true;
    }
    return {false, play_state_has_changed};
  }

  StartAnimations(normalized_data);
  return {true, true};
}

bool KeyframesManager::HasAnimationForType(
    ClayAnimationPropertyType type) const {
  for (auto& animation : animations_) {
    if (animation.keyframes_map.find(type) != animation.keyframes_map.end()) {
      return true;
    }
  }
  return false;
}

bool KeyframesManager::StartListenersNotified(
    ClayAnimationPropertyType type) const {
  for (auto& animation : animations_) {
    if (auto iter = animation.keyframes_map.find(type);
        iter != animation.keyframes_map.end()) {
      return !animation.animator->StartListenersCalled();
    }
  }
  return true;
}

void KeyframesManager::SyncProperties(KeyframesManager* manager) {
  if (manager == nullptr) {
    return;
  }
  // For every type of raster animation, `KeyframesManager` will be cloned each
  // time. And we don't need to compare type here. Besides when raster
  // animation get changed in its process (e.g. a new animation with a
  // longer duration) ,it won't invoke new animation event (e.g.
  // onAnimationStart).
  for (auto& animation : animations_) {
    for (auto& kv : animation.keyframes_map) {
      animation.animator->SetStartListenersCalled(
          manager->StartListenersNotified(kv.first));
    }
  }
}

std::unique_ptr<KeyframesManager> KeyframesManager::CloneForRasterAnimation(
    ClayAnimationPropertyType type, AnimatorTarget* target) const {
  std::unique_ptr<KeyframesManager> clone =
      std::make_unique<KeyframesManager>(target);
  for (auto& animation : animations_) {
    if (auto iter = animation.keyframes_map.find(type);
        iter != animation.keyframes_map.end()) {
      KeyframeAnimation clone_animation;
      clone_animation.animator = animation.animator->Clone();
      clone_animation.animator->SetAnimationTarget(target);
      std::unique_ptr<KeyframeSet> clone_keyframe_set =
          iter->second->Clone(clone.get());
      if (!IsMixedAnimation(animation)) {
        clone_animation.animator->AddListener(clone_keyframe_set.get());
      }
      clone_animation.listener =
          std::make_unique<KeyframeListener>(clone.get());
      clone_animation.animator->AddListener(clone_animation.listener.get());
      clone_animation.animator->AddUpdateListener(clone_keyframe_set.get());
      clone_animation.keyframes_map.emplace(type,
                                            std::move(clone_keyframe_set));
      clone->animations_.emplace_back(std::move(clone_animation));
    }
  }
  return clone;
}

void KeyframesManager::UpdateAnimator(ValueAnimator* animator,
                                      AnimationData data) {
  animator->SetAnimationData(data);
  if (data.play_state == ClayAnimationPlayStateType::kPaused) {
    animator->Pause();
  } else if (data.play_state == ClayAnimationPlayStateType::kRunning) {
    animator->Resume();
  }
  animator->AddAnimationCallback(0);
}

void KeyframesManager::StartAnimations(const std::vector<AnimationData>& data) {
  std::vector<KeyframeAnimation> old_animations = std::move(animations_);
  std::vector<KeyframeAnimation> new_animations;

  for (auto item : data) {
    bool add_new_animation = true;
    auto it = old_animations.begin();
    // For new animations and old animations of the same name we will only
    // update instead of reset,  for animations with different names we will
    // cancel the old animation and start the new one.
    while (it != old_animations.end()) {
      if (it->data.name == item.name) {
        it->SetAnimationData(item);
        UpdateAnimator(it->animator.get(), item);
        add_new_animation = false;
        new_animations.push_back(std::move(*it));
        it = old_animations.erase(it);
        break;
      } else {
        it++;
      }
    }

    if (add_new_animation) {
      KeyframeAnimation animation;
      animation.data = item;
      animation.animator = CreateAnimator(item);
      animation.listener = std::make_unique<KeyframeListener>(this);
      animation.animator->AddListener(animation.listener.get());
      if (!InitKeyframesMap(item, animation.animator.get(),
                            animation.keyframes_map,
                            &animation.has_percentage_values)) {
        continue;
      }
      new_animations.push_back(std::move(animation));
    }
  }

  animations_ = std::move(new_animations);

  for (const auto& animation : old_animations) {
    if (animation.animator->IsStarted()) {
      animation.animator->Cancel();
    }
  }

  for (const auto& animation : animations_) {
    if (animation.animator->IsStarted()) {
      continue;
    }
    animation.animator->Start();
    if (animation.data.play_state == ClayAnimationPlayStateType::kPaused) {
      animation.animator->Pause();
    }
  }
}

std::unique_ptr<ValueAnimator> KeyframesManager::CreateAnimator(
    const AnimationData& data) {
  std::unique_ptr<ValueAnimator> animator =
      std::make_unique<ValueAnimator>(data);
  animator->SetAnimationHandler(target_->GetAnimationHandler());
  animator->SetAnimationTarget(target_);
  animator->SetUseMonotonicFrameTime(true);
  return animator;
}

bool KeyframesManager::InitKeyframesMap(const AnimationData& data,
                                        ValueAnimator* animator,
                                        KeyframesMap& keyframes_map,
                                        bool* out_has_percentage_values) {
  const KeyframesMap* src_map = target_->GetKeyframesMap(data.name);
  if (!src_map) {
    return false;
  }

  for (const auto& keyframes : keyframes_map) {
    animator->RemoveUpdateListener(keyframes.second.get());
    animator->RemoveListener(keyframes.second.get());
  }
  keyframes_map.clear();

  bool has_percentage_values = false;
  for (const auto& keyframes : *src_map) {
    has_percentage_values |= keyframes.second->HasPercentageValues();
    std::unique_ptr<KeyframeSet> clone = keyframes.second->Clone(this);
    animator->AddUpdateListener(clone.get());
    animator->AddListener(clone.get());
    keyframes_map.emplace(keyframes.first, std::move(clone));
  }

  if (out_has_percentage_values) {
    *out_has_percentage_values = has_percentage_values;
  }

  return !keyframes_map.empty();
}

bool KeyframesManager::HasAnimationRunning() const {
  for (const auto& animation : animations_) {
    if (animation.animator->IsStarted()) {
      return true;
    }
  }
  return false;
}

void KeyframesManager::EndAllAnimators() {
  for (const auto& animation : animations_) {
    if (animation.animator->IsStarted()) {
      animation.animator->End();
    }
  }
}

void KeyframesManager::EndAnimator(const std::string& name) {
  for (const auto& animation : animations_) {
    if (animation.data.name == name && animation.animator->IsStarted()) {
      animation.animator->End();
    }
  }
}

void KeyframesManager::CancelAllAnimators() {
  for (const auto& animation : animations_) {
    if (animation.animator->IsStarted()) {
      animation.animator->Cancel();
    }
  }
}

void KeyframesManager::CancelAnimator(const std::string& name) {
  for (const auto& animation : animations_) {
    if (animation.data.name == name && animation.animator->IsStarted()) {
      animation.animator->Cancel();
    }
  }
}
void KeyframesManager::SetEventHandler(AnimationEventHandler* event_handler) {
  event_handler_ = event_handler;
}
void KeyframesManager::OnAnimationStart(const Animator& animation) {
  if (event_handler_) {
    const std::string& animation_name = animation.GetAnimationName();
    AnimationParams animation_params{
        ClayEventType::kClayEventTypeAnimationStart, animation_name.c_str()};
    event_handler_->OnAnimationEvent(animation_params);
  }
}
void KeyframesManager::OnAnimationRepeat(const Animator& animation) {
  if (event_handler_) {
    const std::string& animation_name = animation.GetAnimationName();
    AnimationParams animation_params{
        ClayEventType::kClayEventTypeAnimationRepeat, animation_name.c_str()};
    event_handler_->OnAnimationEvent(animation_params);
  }
}
void KeyframesManager::OnAnimationEnd(const Animator& animation) {
  if (event_handler_) {
    const std::string& animation_name = animation.GetAnimationName();
    AnimationParams animation_params{ClayEventType::kClayEventTypeAnimationEnd,
                                     animation_name.c_str()};
    event_handler_->OnAnimationEvent(animation_params);
  }
}
void KeyframesManager::OnAnimationCancel(const Animator& animation) {
  if (event_handler_) {
    const std::string& animation_name = animation.GetAnimationName();
    AnimationParams animation_params{
        ClayEventType::kClayEventTypeAnimationCancel, animation_name.c_str()};
    event_handler_->OnAnimationEvent(animation_params);
  }
}

void KeyframesManager::UpdateLayoutSize() {
  for (auto& animation : animations_) {
    if (animation.has_percentage_values) {
      // We should re-init the keyframes of the animation if it has percentage
      // values and the element size has changed
      InitKeyframesMap(animation.data, animation.animator.get(),
                       animation.keyframes_map);
    }
  }
}

}  // namespace clay
