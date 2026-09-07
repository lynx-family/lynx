// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/animation_manager_default.h"

#include <memory>

namespace lynx {
namespace list {

AnimationManagerDefault::~AnimationManagerDefault() = default;

void AnimationManagerDefault::SetUpdateAnimationConfig(
    const UpdateAnimationConfig& config) {}

void AnimationManagerDefault::BeforeDataUpdate(
    bool has_valid_diff, bool has_expected_diff_animation,
    bool has_completed_first_layout) {}

void AnimationManagerDefault::BeforeLayout() {}

void AnimationManagerDefault::AfterLayoutBeforeFlush() {}

void AnimationManagerDefault::AfterFlush() {}

void AnimationManagerDefault::CancelAnimationTransaction(
    AnimationCancelReason reason) {}

void AnimationManagerDefault::Destroy() {}

std::unique_ptr<AnimationManager> CreateAnimationManager(
    ListContainerImpl* list_container) {
  return std::make_unique<AnimationManagerDefault>();
}

}  // namespace list
}  // namespace lynx
