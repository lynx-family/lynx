// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_CONSTANTS_H_
#define CORE_ANIMATION_CONSTANTS_H_

namespace lynx {
namespace animation {

static constexpr const char kKeyframeAnimationName[] = "keyframe-animation";
static constexpr const char kTransitionAnimationName[] = "transition-animation";
static constexpr const char* kKeyframeStartEventName = "animationstart";
static constexpr const char* kTransitionStartEventName = "transitionstart";
static constexpr const char* kKeyframeEndEventName = "animationend";
static constexpr const char* kTransitionEndEventName = "transitionend";
static constexpr const char* kKeyframeCancelEventName = "animationcancel";
static constexpr const char* kTransitionCancelEventName = "transitioncancel";
static constexpr const char* kKeyframeIterationEventName = "animationiteration";

}  // namespace animation
}  // namespace lynx
#endif  // CORE_ANIMATION_CONSTANTS_H_
