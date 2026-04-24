// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_TYPES_H_
#define GFX_ANIMATION_ANIMATION_TYPES_H_

#include <cstdint>

namespace lynx {
namespace gfx {

enum class TimingFunctionType : uint8_t {
  kLinear = 0,
  kEaseIn = 1,
  kEaseOut = 2,
  kEaseInEaseOut = 3,
  kSquareBezier = 4,
  kCubicBezier = 5,
  kSteps = 6,
};

enum class StepsType : uint8_t {
  kInvalid = 0,
  kStart = 1,
  kEnd = 2,
  kJumpBoth = 3,
  kJumpNone = 4,
};

struct TimingFunctionData {
  float x1{0.0f};
  float y1{0.0f};
  float x2{0.0f};
  float y2{0.0f};
  TimingFunctionType timing_func{TimingFunctionType::kLinear};
  StepsType steps_type{StepsType::kInvalid};
};

enum class AnimationFillModeType : uint8_t {
  kNone = 0,
  kForwards = 1,
  kBackwards = 2,
  kBoth = 3,
};

enum class AnimationDirectionType : uint8_t {
  kNormal = 0,
  kReverse = 1,
  kAlternate = 2,
  kAlternateReverse = 3,
};

enum class AnimationPlayStateType : uint8_t {
  kPaused = 0,
  kRunning = 1,
};

struct AnimationData {
  long duration{0};
  long delay{0};
  TimingFunctionData timing_func;
  int iteration_count{1};
  AnimationFillModeType fill_mode{AnimationFillModeType::kNone};
  AnimationDirectionType direction{AnimationDirectionType::kNormal};
  AnimationPlayStateType play_state{AnimationPlayStateType::kRunning};
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_TYPES_H_
