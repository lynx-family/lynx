// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/animation/keyframe_manager.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/animation/keyframe_animator.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

KeyframeManager::KeyframeManager(UIBase* ui) : ui_(ui) {}

KeyframeManager::~KeyframeManager() { EndAllAnimation(); }

void KeyframeManager::SetAnimations(const std::vector<AnimationInfo>& infos) {
  infos_ = infos;
  NotifyAnimationUpdated();
}

void KeyframeManager::NotifyAnimationUpdated() {
  if (infos_.empty()) {
    if (!ui_) return;
  }

  std::unordered_map<std::string, std::unique_ptr<KeyframeAnimator>>
      new_animators;

  for (const auto& info : infos_) {
    if (info.GetName().empty()) {
      continue;
    }

    auto it = animators_.find(info.GetName());
    if (it != animators_.end()) {
      // Reuse existing animator
      new_animators[info.GetName()] = std::move(it->second);
      animators_.erase(it);
    } else {
      // Create new animator
      auto animator = std::make_unique<KeyframeAnimator>(ui_);
      new_animators[info.GetName()] = std::move(animator);
    }
  }

  animators_ = std::move(new_animators);

  // Apply info to animators
  for (const auto& info : infos_) {
    if (info.GetName().empty()) {
      continue;
    }
    auto it = animators_.find(info.GetName());
    if (it != animators_.end() && it->second) {
      it->second->Apply(info);
    }
  }
}

void KeyframeManager::EndAllAnimation() {
  animators_.clear();
  infos_.clear();
}

bool KeyframeManager::HasAnimationRunning() {
  for (const auto& pair : animators_) {
    if (pair.second->IsRunning()) {
      return true;
    }
  }
  return false;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
