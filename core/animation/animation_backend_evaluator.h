// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_ANIMATION_BACKEND_EVALUATOR_H_
#define CORE_ANIMATION_ANIMATION_BACKEND_EVALUATOR_H_

#include <vector>

#include "gfx/animation/platform_animation.h"

namespace lynx {
namespace animation {

enum class AnimationFallbackReason {
  kNone = 0,
  kRequiresCoreLayout,
  kUnresolvedKeyframe,
  kUnsupportedProperty,
  kUnsupportedValue,
  kUnsupportedTimingFunction,
  kDynamicDependency,
  kBackendUnavailable,
};

struct AnimationBackendResult {
  bool can_run{false};
  AnimationFallbackReason fallback_reason{AnimationFallbackReason::kNone};

  bool CanRun() const { return can_run; }
};

struct AnimationBackendRequest {
  // The CSS animation builder supplies non-empty, typed, sorted and fully
  // materialized keyframes. Those are builder invariants, not routing policy.
  gfx::AnimationKind kind{gfx::AnimationKind::kKeyframe};
  gfx::AnimationPropertyType property{gfx::AnimationPropertyType::kNone};
  std::vector<const gfx::Keyframe*> keyframes;
  const gfx::AnimationData* animation_data{nullptr};
  bool has_dynamic_dependencies{false};
};

AnimationBackendResult EvaluateAnimationBackend(
    const AnimationBackendRequest& request,
    const gfx::AnimationBackendCapabilities& capabilities);

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_ANIMATION_BACKEND_EVALUATOR_H_
