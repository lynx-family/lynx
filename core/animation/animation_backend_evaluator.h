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
  kUnsupportedEvent,
};

struct AnimationBackendResult {
  bool can_run{false};
  AnimationFallbackReason fallback_reason{AnimationFallbackReason::kNone};
  // The equivalent effect timing for a backend without segment timing.
  gfx::TimingFunctionData platform_timing;

  bool CanRun() const { return can_run; }
};

struct AnimationBackendRequest {
  gfx::AnimationKind kind{gfx::AnimationKind::kKeyframe};
  gfx::AnimationPropertyType property{gfx::AnimationPropertyType::kNone};
  // The builder supplies at least two non-null, same-typed keyframes, sorted
  // with endpoints at 0 and 1. Values and outgoing-interval timing are already
  // resolved; a null timing function means linear.
  std::vector<const gfx::Keyframe*> keyframes;
  bool has_dynamic_dependencies{false};
  // Only bound events constrain routing; unobserved events need no delivery.
  gfx::AnimationEventCapabilityMask required_events{0};
};

AnimationBackendResult EvaluateAnimationBackend(
    const AnimationBackendRequest& request,
    const gfx::AnimationBackendCapabilities& capabilities);

}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_ANIMATION_BACKEND_EVALUATOR_H_
