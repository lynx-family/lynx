// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_TRANSITION_MANAGER_H_
#define CLAY_GFX_ANIMATION_TRANSITION_MANAGER_H_

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "clay/gfx/animation/animation_event_handler.h"
#include "clay/gfx/animation/animator_listener_adapter.h"
#include "clay/gfx/animation/animator_target.h"
#include "clay/gfx/animation/transition_data.h"
#include "clay/gfx/animation/type_evaluator.h"
#include "clay/gfx/animation/value_animator.h"

namespace clay {

class TransitionManager {
 public:
  explicit TransitionManager(AnimatorTarget* target);
  ~TransitionManager();
  void AppendData(const TransitionData& data);
  void UpdateData(const std::vector<TransitionData>& data);
  bool Enabled(ClayAnimationPropertyType type) const;
  bool HasAnimationRunning() const;
  bool IsAnimationRunning(ClayAnimationPropertyType type);
  template <typename T>
  void UpdateAnimationValue(ClayAnimationPropertyType type, const T& value);
  void EndAllAnimators();
  void EndAnimator(ClayAnimationPropertyType type);
  void CancelAnimator(ClayAnimationPropertyType type);
  void CancelAllAnimators();
  void OnAnimationStart(const Animator& animation,
                        ClayAnimationPropertyType type);
  void OnAnimationEnd(const Animator& animation,
                      ClayAnimationPropertyType type);
  template <typename T>
  bool TransitionTo(ClayAnimationPropertyType type, const T& value);
  AnimatorTarget* GetTarget() const { return target_; }
  void SetEventHandler(AnimationEventHandler* event_handler);
  std::vector<ValueAnimator*> GetRunningAnimators();

  bool StartListenersNotified(ClayAnimationPropertyType type) const;
  std::unique_ptr<TransitionManager> CloneForRasterAnimation(
      ClayAnimationPropertyType type, AnimatorTarget* target) const;

  // See `SyncProperties` in `KeyframesManager`.
  void SyncProperties(TransitionManager* manager);

 private:
  AnimatorTarget* target_;
  AnimationEventHandler* event_handler_{};
  std::vector<TransitionData> data_;
  using ActiveTransition = std::pair<std::unique_ptr<ValueAnimator>,
                                     std::unique_ptr<AnimatorListenerAdapter>>;
  std::map<ClayAnimationPropertyType, ActiveTransition> active_transitions_;
};

template <typename T>
class TransitionListener : public AnimatorListenerAdapter {
 public:
  TransitionListener(TransitionManager* mgr, ClayAnimationPropertyType type,
                     T old_value, T new_value,
                     bool is_raster_transition = false)
      : mgr_(mgr), type_(type), old_value_(old_value), new_value_(new_value) {}

  void OnAnimationUpdate(ValueAnimator& animation) override {
    float current_fraction = animation.GetAnimatedFraction();
    T animated_value =
        TypeEvaluator<T>::Evaluate(current_fraction, old_value_, new_value_);
    mgr_->GetTarget()->SetProperty(type_, animated_value, true);
  }
  void OnAnimationStart(Animator& animation) override {
    mgr_->OnAnimationStart(animation, type_);
  }
  void OnAnimationEnd(Animator& animation) override {
    mgr_->GetTarget()->SetProperty(type_, new_value_, false);
    mgr_->OnAnimationEnd(animation, type_);
  }
  void OnAnimationCancel(Animator& animation) override {}
  void SetNewValue(const T& value) { new_value_ = value; }

  std::unique_ptr<TransitionListener> CloneForRasterAnimation(
      TransitionManager* manager) const {
    return std::make_unique<TransitionListener>(manager, type_, old_value_,
                                                new_value_, true);
  }

 private:
  TransitionManager* mgr_;
  ClayAnimationPropertyType type_;
  T old_value_;
  T new_value_;
};

template <typename T>
bool TransitionManager::TransitionTo(ClayAnimationPropertyType type,
                                     const T& value) {
  // End current transition animation if there is one
  CancelAnimator(type);
  for (auto& transition : data_) {
    if (static_cast<int>(transition.property) & static_cast<int>(type)) {
      T old_value;
      target_->GetProperty(type, old_value);
      active_transitions_[type].second =
          std::make_unique<TransitionListener<T>>(this, type, old_value, value);
      active_transitions_[type].first = std::make_unique<ValueAnimator>();
      auto& animator = active_transitions_[type].first;
      animator->SetDuration(transition.duration);
      animator->SetStartDelay(transition.delay);
      animator->SetInterpolator(Interpolator::Create(transition.timing_func));
      animator->AddUpdateListener(active_transitions_[type].second.get());
      animator->AddListener(active_transitions_[type].second.get());
      animator->SetAnimationHandler(target_->GetAnimationHandler());
      animator->Start();
      return true;
    }
  }
  return false;
}

template <typename T>
void TransitionManager::UpdateAnimationValue(ClayAnimationPropertyType type,
                                             const T& value) {
  auto it = active_transitions_.find(ClayAnimationPropertyType::kTransform);
  if (it != active_transitions_.end()) {
    static_cast<TransitionListener<T>*>(it->second.second.get())
        ->SetNewValue(value);
  }
}

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_TRANSITION_MANAGER_H_
