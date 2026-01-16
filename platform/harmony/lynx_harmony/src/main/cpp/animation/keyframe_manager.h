// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_KEYFRAME_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_KEYFRAME_MANAGER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/animation/animation_info.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/animation/keyframe_animator.h"

namespace lynx {
namespace tasm {

namespace harmony {
class UIBase;

class KeyframeManager {
 public:
  explicit KeyframeManager(UIBase* ui);
  ~KeyframeManager();

  void SetAnimations(const std::vector<AnimationInfo>& infos);

  void NotifyAnimationUpdated();
  void EndAllAnimation();

  bool HasAnimationRunning();

 private:
  UIBase* ui_;
  std::vector<AnimationInfo> infos_;
  std::unordered_map<std::string, std::unique_ptr<KeyframeAnimator>> animators_;
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_ANIMATION_KEYFRAME_MANAGER_H_
