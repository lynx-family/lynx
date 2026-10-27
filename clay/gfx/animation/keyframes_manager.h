// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_KEYFRAMES_MANAGER_H_
#define CLAY_GFX_ANIMATION_KEYFRAMES_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/gfx/animation/animation_data.h"
#include "clay/gfx/animation/animation_event_handler.h"
#include "clay/gfx/animation/animator_listener.h"
#include "clay/gfx/animation/keyframe_set.h"
#include "clay/gfx/animation/value_animator.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class AnimatorTarget;

class KeyframesManager {
 public:
  explicit KeyframesManager(AnimatorTarget* target);
  ~KeyframesManager();

  struct UpdateDataResult {
    bool data_has_changed;
    bool state_has_changed;
  };

  // Return true if data has changed
  UpdateDataResult UpdateData(const std::vector<AnimationData>& data);
  bool HasAnimationRunning() const;
  void EndAllAnimators();
  void EndAnimator(const std::string& name);
  void CancelAllAnimators();
  void CancelAnimator(const std::string& name);

  AnimatorTarget* GetTarget() const { return target_; }
  void SetEventHandler(AnimationEventHandler* event_handler);

  void OnAnimationStart(const Animator& animation);
  void OnAnimationRepeat(const Animator& animation);
  void OnAnimationEnd(const Animator& animator);
  void OnAnimationCancel(const Animator& animator);

  // If there are some animations with percentage value, we need to recalculate
  // these values.
  void UpdateLayoutSize();

  struct KeyframeAnimation {
   public:
    void SetAnimationData(AnimationData animation_data) {
      data = animation_data;
    }
    AnimationData data;
    KeyframesMap keyframes_map;
    std::unique_ptr<ValueAnimator> animator;
    std::unique_ptr<AnimatorListener> listener;
    bool has_percentage_values;
  };

  bool HasAnimationForType(ClayAnimationPropertyType type) const;

  std::unique_ptr<KeyframesManager> CloneForRasterAnimation(
      ClayAnimationPropertyType type, AnimatorTarget* target) const;

  // whether the animator of a certain type has called its `OnAnimationStart`.
  bool StartListenersNotified(ClayAnimationPropertyType type) const;

  const std::vector<KeyframeAnimation>& animations() const {
    return animations_;
  }

  void UpdateAnimator(ValueAnimator* animator, AnimationData data);

  // Avoid access to KeyframesManager of AnimationLayer in UI Thread, may cause
  // multi-threading issues.
  void SyncProperties(KeyframesManager* manager);

 private:
  void StartAnimations(const std::vector<AnimationData>& data);
  std::unique_ptr<ValueAnimator> CreateAnimator(const AnimationData& data);
  bool InitKeyframesMap(const AnimationData& data, ValueAnimator* animator,
                        KeyframesMap& keyframes_map,
                        bool* out_has_percentage_values = nullptr);

  FRIEND_TEST(KeyFrameTest, DefaultStartAndEnd);
  FRIEND_TEST(KeyFrameTest, UpdateAnimation);
  FRIEND_TEST(KeyFrameTest, ChangeFillmode);
  FRIEND_TEST(KeyFrameTest, AnimationDelay);
  FRIEND_TEST(KeyFrameTest, AnimationDelayCombineForwards);
  FRIEND_TEST(KeyFrameTest, AnimationDelayCombineBackwards);
  FRIEND_TEST(KeyFrameTest, AnimationStartEvent);
  FRIEND_TEST(KeyFrameTest, AnimationCancelEvent);
  FRIEND_TEST(KeyFrameTest, AnimationEndEvent);

  AnimatorTarget* target_;
  AnimationEventHandler* event_handler_;
  std::vector<KeyframeAnimation> animations_;
};

class KeyframeListener : public AnimatorListenerAdapter {
 public:
  explicit KeyframeListener(KeyframesManager* mgr) : mgr_(mgr) {}
  void OnAnimationStart(Animator& animation) override {
    mgr_->OnAnimationStart(animation);
  }
  void OnAnimationEnd(Animator& animation) override {
    mgr_->OnAnimationEnd(animation);
  }
  void OnAnimationCancel(Animator& animation) override {
    mgr_->OnAnimationCancel(animation);
  }
  void OnAnimationRepeat(Animator& animation) override {
    mgr_->OnAnimationRepeat(animation);
  }

 private:
  KeyframesManager* mgr_;
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_KEYFRAMES_MANAGER_H_
