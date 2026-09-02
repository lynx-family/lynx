// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_TYPES_H_
#define GFX_ANIMATION_ANIMATION_TYPES_H_

#include <cstdint>

namespace lynx {
namespace gfx {

// Keep the numeric values aligned with the existing starlight and Clay
// property enums while animation users migrate to the gfx-facing type.
enum class AnimationPropertyType : uint32_t {
  kNone = 0,
  kOpacity = 1 << 0,
  kScaleX = 1 << 1,
  kScaleY = 1 << 2,
  kScaleXY = 1 << 3,
  kWidth = 1 << 4,
  kHeight = 1 << 5,
  kBackgroundColor = 1 << 6,
  kVisibility = 1 << 7,
  kLeft = 1 << 8,
  kTop = 1 << 9,
  kRight = 1 << 10,
  kBottom = 1 << 11,
  kTransform = 1 << 12,
  kColor = 1 << 13,
  kMaxWidth = 1 << 14,
  kMinWidth = 1 << 15,
  kMaxHeight = 1 << 16,
  kMinHeight = 1 << 17,
  kPaddingLeft,
  kPaddingRight,
  kPaddingTop,
  kPaddingBottom,
  kMarginLeft,
  kMarginRight,
  kMarginTop,
  kMarginBottom,
  kBorderLeftWidth,
  kBorderRightWidth,
  kBorderTopWidth,
  kBorderBottomWidth,
  kBorderTopColor,
  kBorderLeftColor,
  kBorderRightColor,
  kBorderBottomColor,
  kFlexBasis,
  kFlexGrow,
  kBorderWidth,
  kBorderColor,
  kMargin,
  kPadding,
  kFilter,
  kBoxShadow,
  kOffsetDistance,
  kBackgroundPosition,
  kTransformOrigin,
  kAll = 1 << 18,
};

enum class KeyframeValueType : uint8_t {
  kUnknown = 0,
  kFloat,
  kColor,
  kLength,
  kVec2,
  kFilter,
  kTransform,
  kBoxShadow,
  kEnum,
};

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
