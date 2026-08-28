// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_PLATFORM_ANIMATION_H_
#define GFX_ANIMATION_PLATFORM_ANIMATION_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/animation_types.h"

namespace lynx {
namespace gfx {

using AnimationId = uint64_t;

enum class AnimationKind : uint8_t {
  kKeyframe = 0,
  kTransition,
};

enum class AnimationBackendType : uint8_t {
  kNone = 0,
  kIOS,
};

enum class PlatformAnimationCommandType : uint8_t {
  kHandoff = 0,
  kUpdate,
  kCancel,
};

using TimingFunctionCapabilityMask = uint8_t;
inline constexpr TimingFunctionCapabilityMask kTimingFunctionLinear = 1 << 0;
inline constexpr TimingFunctionCapabilityMask kTimingFunctionCubicBezier = 1
                                                                           << 1;
inline constexpr TimingFunctionCapabilityMask kTimingFunctionSteps = 1 << 2;
inline constexpr TimingFunctionCapabilityMask kAllTimingFunctions =
    kTimingFunctionLinear | kTimingFunctionCubicBezier | kTimingFunctionSteps;

using TransformAxisCapabilityMask = uint8_t;
inline constexpr TransformAxisCapabilityMask kTransformAxisX = 1 << 0;
inline constexpr TransformAxisCapabilityMask kTransformAxisY = 1 << 1;
inline constexpr TransformAxisCapabilityMask kTransformAxisZ = 1 << 2;
inline constexpr TransformAxisCapabilityMask kTransformAxesXY =
    kTransformAxisX | kTransformAxisY;
inline constexpr TransformAxisCapabilityMask kTransformAxesXYZ =
    kTransformAxesXY | kTransformAxisZ;

using TransformUnitCapabilityMask = uint8_t;
inline constexpr TransformUnitCapabilityMask kTransformUnitNumber = 1 << 0;
inline constexpr TransformUnitCapabilityMask kTransformUnitPercent = 1 << 1;
inline constexpr TransformUnitCapabilityMask kAllTransformUnits =
    kTransformUnitNumber | kTransformUnitPercent;

using TransformMatrixCapabilityMask = uint8_t;
inline constexpr TransformMatrixCapabilityMask kTransformMatrix2D = 1 << 0;
inline constexpr TransformMatrixCapabilityMask kTransformMatrix3D = 1 << 1;
inline constexpr TransformMatrixCapabilityMask kAllTransformMatrices =
    kTransformMatrix2D | kTransformMatrix3D;

struct TransformValueCapability {
  TransformAxisCapabilityMask translate_axes{0};
  TransformUnitCapabilityMask translate_units{0};
  TransformAxisCapabilityMask rotate_axes{0};
  TransformAxisCapabilityMask scale_axes{0};
  TransformAxisCapabilityMask skew_axes{0};
  TransformMatrixCapabilityMask matrix_dimensions{0};
};

struct AnimationPropertyCapability {
  AnimationKind kind{AnimationKind::kKeyframe};
  AnimationPropertyType property{AnimationPropertyType::kNone};
  KeyframeValueType value_type{KeyframeValueType::kUnknown};
  bool supports_per_keyframe_timing{false};
  TimingFunctionCapabilityMask timing_functions{kAllTimingFunctions};
  TransformValueCapability transform;
};

struct AnimationBackendCapabilities {
  AnimationBackendType backend{AnimationBackendType::kNone};
  std::vector<AnimationPropertyCapability> properties;

  const AnimationPropertyCapability* FindProperty(
      AnimationKind kind, AnimationPropertyType property,
      KeyframeValueType value_type) const {
    for (const auto& capability : properties) {
      if (capability.kind == kind && capability.property == property &&
          capability.value_type == value_type) {
        return &capability;
      }
    }
    return nullptr;
  }
};

struct PlatformAnimationProperty {
  AnimationPropertyType property{AnimationPropertyType::kNone};
  // Parsed keyframes are immutable after handoff. Sharing them keeps the Core
  // routing state and queued commands on the same typed gfx representation.
  std::vector<std::shared_ptr<const Keyframe>> keyframes;
};

struct PlatformAnimationCommand {
  PlatformAnimationCommand() = default;
  PlatformAnimationCommand(PlatformAnimationCommand&&) = default;
  PlatformAnimationCommand& operator=(PlatformAnimationCommand&&) = default;
  PlatformAnimationCommand(const PlatformAnimationCommand&) = delete;
  PlatformAnimationCommand& operator=(const PlatformAnimationCommand&) = delete;

  PlatformAnimationCommandType type{PlatformAnimationCommandType::kHandoff};
  AnimationId animation_id{0};
  uint32_t generation{0};
  AnimationKind kind{AnimationKind::kKeyframe};
  std::string name;
  // A CSS keyframe animation effect is routed atomically. A CSS transition is
  // already represented as one independently routed property animation.
  std::vector<PlatformAnimationProperty> properties;
  AnimationData animation_data;
};

using PlatformAnimationCommandBatch = std::vector<PlatformAnimationCommand>;

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_PLATFORM_ANIMATION_H_
