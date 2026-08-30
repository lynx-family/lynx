// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_TRANSITION_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_TRANSITION_MANAGER_H_

#include <arkui/native_animate.h>

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/value/base_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/animation/animation_info.h"

namespace lynx {
namespace tasm {
namespace harmony {

class UIBase;

class TransitionAnimator;

class TransitionManager {
 public:
  explicit TransitionManager(UIBase* ui);
  ~TransitionManager();

  bool InitializeFromConfig(const lepus::Value& config);
  void ApplyPropertyTransition(starlight::AnimationPropertyType property,
                               const lepus::Value& value);
  void ApplyLayoutTransition(float left, float top, float width, float height,
                             const float* paddings, const float* margins,
                             const float* sticky, float max_height,
                             uint32_t node_index);
  void StartTransitions();
  bool HasLayoutTransition();
  bool ContainsTransition(starlight::AnimationPropertyType property) const;

 private:
  void EndTransitionAnimator(starlight::AnimationPropertyType prop);
  void RemoveDuplicateAnimation(starlight::AnimationPropertyType lhs,
                                starlight::AnimationPropertyType rhs);
  UIBase* ui_;
  std::unordered_map<starlight::AnimationPropertyType, AnimationInfo>
      animation_infos_;
  std::unordered_map<starlight::AnimationPropertyType,
                     std::unique_ptr<TransitionAnimator>>
      running_animators_;
  std::unordered_map<starlight::AnimationPropertyType,
                     std::unique_ptr<TransitionAnimator>>
      idle_animators_;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_TRANSITION_MANAGER_H_
