// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_PLATFORM_ANIMATION_H_
#define GFX_ANIMATION_PLATFORM_ANIMATION_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/time/time_point.h"
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
  // Remove an executor while the logical animation continues elsewhere.
  kDetach,
};

// Shared across the animation and UI threads. Executors own neither the clock
// nor the lifecycle: rebuilding one must not restart the logical animation.
class AnimationPlaybackState {
 public:
  enum class Phase { kBefore, kActive, kAfter, kCanceled };

  AnimationPlaybackState(fml::TimeDelta current_time, bool paused, Phase phase)
      : current_time_(current_time), paused_(paused), phase_(phase) {}

  // Inspector controls use one reference time for all animations in a request.
  fml::TimeDelta CurrentTime(
      fml::TimePoint reference_time = fml::TimePoint::Now()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return CurrentTimeLocked(reference_time);
  }

  void SetPaused(bool paused,
                 fml::TimePoint reference_time = fml::TimePoint::Now()) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_time_ = CurrentTimeLocked(reference_time);
    reference_time_ = reference_time;
    paused_ = paused;
  }

  void SetCurrentTime(fml::TimeDelta current_time,
                      fml::TimePoint reference_time = fml::TimePoint::Now()) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_time_ = current_time;
    reference_time_ = reference_time;
  }

  uint64_t ChangeExecutor() {
    std::lock_guard<std::mutex> lock(mutex_);
    return ++executor_;
  }

  bool IsCurrentExecutor(uint64_t executor) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return executor == executor_;
  }

  Phase phase() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return phase_;
  }

  bool ClaimEvent(uint64_t executor, Phase phase, bool allow_reentry = true) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (executor != executor_ || phase_ == phase ||
        phase_ == Phase::kCanceled ||
        (phase == Phase::kCanceled && phase_ == Phase::kAfter) ||
        (!allow_reentry && phase == Phase::kActive &&
         phase_ != Phase::kBefore)) {
      return false;
    }
    phase_ = phase;
    return true;
  }

 private:
  fml::TimeDelta CurrentTimeLocked(fml::TimePoint reference_time) const {
    return current_time_ + (paused_ ? fml::TimeDelta::Zero()
                                    : reference_time - reference_time_);
  }

  mutable std::mutex mutex_;
  fml::TimeDelta current_time_;
  fml::TimePoint reference_time_{fml::TimePoint::Now()};
  bool paused_;
  Phase phase_;
  uint64_t executor_{0};
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
  // Units are declared per axis; a zero mask means the axis is unsupported.
  TransformUnitCapabilityMask translate_x_units{0};
  TransformUnitCapabilityMask translate_y_units{0};
  TransformUnitCapabilityMask translate_z_units{0};
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
  // An update may reuse the previous keyframes when only playback or list order
  // changed. The executor must still rebuild size-dependent platform values.
  bool reuse_keyframes{false};
  // A CSS keyframe animation effect is routed atomically. A CSS transition is
  // already represented as one independently routed property animation.
  std::vector<PlatformAnimationProperty> properties;
  AnimationData animation_data;
  // Present for routed keyframe animations. The executor token changes only
  // when execution moves, whereas generation changes on each native update.
  std::shared_ptr<AnimationPlaybackState> playback;
  uint64_t executor{0};
};

using PlatformAnimationCommandBatch = std::vector<PlatformAnimationCommand>;

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_PLATFORM_ANIMATION_H_
