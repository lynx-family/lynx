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
  // Remove the platform executor while the logical animation continues in
  // C++ New Animator, without canceling the animation.
  kDetach,
};

// Shared across the animation and UI threads. Executors own neither the clock
// nor the lifecycle: rebuilding one must not restart the logical animation.
class AnimationPlaybackState {
 public:
  // Tracks claimed lifecycle events, independently of the timing phase.
  // A timing update can allow kFinished to transition back to kStarted.
  enum class EventState { kNotStarted, kStarted, kFinished, kCanceled };

  AnimationPlaybackState(fml::TimeDelta current_time, bool paused,
                         EventState event_state)
      : current_time_(current_time),
        paused_(paused),
        event_state_(event_state) {}

  // Return the animation's local playback position at reference_time, a point
  // on the system monotonic clock. The result and the argument have different
  // time origins. Inspector controls can query several animations at one point.
  fml::TimeDelta CurrentTime(
      fml::TimePoint reference_time = fml::TimePoint::Now()) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return CurrentTimeLocked(reference_time);
  }

  // Capture the local playback position at the supplied monotonic clock point
  // before changing pause state, so paused time does not advance the animation.
  void SetPaused(bool paused,
                 fml::TimePoint reference_time = fml::TimePoint::Now()) {
    std::lock_guard<std::mutex> lock(mutex_);
    current_time_ = CurrentTimeLocked(reference_time);
    reference_time_ = reference_time;
    paused_ = paused;
  }

  // Set the local playback position to current_time at the monotonic clock
  // point reference_time. This changes the position without changing pause
  // state.
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

  EventState event_state() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return event_state_;
  }

  bool ClaimEvent(uint64_t executor, EventState event_state,
                  bool allow_reentry = true) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Ignore callbacks from an executor that no longer owns the animation.
    if (executor != executor_) {
      return false;
    }
    // Do not emit the same lifecycle event again while its event state is
    // unchanged.
    if (event_state_ == event_state) {
      return false;
    }
    // Cancellation is terminal for this logical animation.
    if (event_state_ == EventState::kCanceled) {
      return false;
    }
    // Removing an already finished animation must not emit a cancel event.
    if (event_state == EventState::kCanceled &&
        event_state_ == EventState::kFinished) {
      return false;
    }
    // When reentry is disabled, allow only the initial start from kNotStarted.
    // Rebuilding a completed fill must not emit another start event.
    if (!allow_reentry && event_state == EventState::kStarted &&
        event_state_ != EventState::kNotStarted) {
      return false;
    }
    event_state_ = event_state;
    return true;
  }

 private:
  fml::TimeDelta CurrentTimeLocked(fml::TimePoint reference_time) const {
    return current_time_ + (paused_ ? fml::TimeDelta::Zero()
                                    : reference_time - reference_time_);
  }

  mutable std::mutex mutex_;
  // These fields form an anchor: at monotonic clock point reference_time_, the
  // animation's local playback position is current_time_. While running, add
  // elapsed monotonic time to that position; while paused, keep it unchanged.
  // current_time_ is a duration on the animation timeline, not a clock
  // timestamp.
  fml::TimeDelta current_time_;
  // System monotonic clock point corresponding to current_time_, not the
  // animation's start time or its local playback position.
  fml::TimePoint reference_time_{fml::TimePoint::Now()};
  bool paused_;
  EventState event_state_;
  // Execution ownership token, incremented when switching between New Animator
  // and platform execution. Commands and callbacks carrying an older token
  // must not act on this logical animation.
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

using AnimationEventCapabilityMask = uint8_t;
inline constexpr AnimationEventCapabilityMask kAnimationEventStart = 1 << 0;
inline constexpr AnimationEventCapabilityMask kAnimationEventEnd = 1 << 1;
inline constexpr AnimationEventCapabilityMask kAnimationEventCancel = 1 << 2;
inline constexpr AnimationEventCapabilityMask kAnimationEventIteration = 1 << 3;

struct AnimationEventCapability {
  AnimationKind kind{AnimationKind::kKeyframe};
  AnimationEventCapabilityMask events{0};
};

struct AnimationBackendCapabilities {
  AnimationBackendType backend{AnimationBackendType::kNone};
  std::vector<AnimationPropertyCapability> properties;
  // Event support belongs to the whole animation, independently of properties.
  std::vector<AnimationEventCapability> event_capabilities;

  bool SupportsEvents(AnimationKind kind,
                      AnimationEventCapabilityMask required) const {
    if (required == 0) {
      return true;
    }
    for (const auto& capability : event_capabilities) {
      if (capability.kind == kind) {
        return (required & ~capability.events) == 0;
      }
    }
    return false;
  }

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
  // Present for routed keyframe animations.
  std::shared_ptr<AnimationPlaybackState> playback;
  // Snapshot of playback's execution ownership token, not an animator or thread
  // ID. IsCurrentExecutor() rejects stale work after execution switches between
  // New Animator and the platform. Unlike generation, this token does not
  // change for updates that keep the same execution backend.
  uint64_t executor{0};
};

using PlatformAnimationCommandBatch = std::vector<PlatformAnimationCommand>;

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_PLATFORM_ANIMATION_H_
