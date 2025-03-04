// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EFFECT_TIMING_H_
#define CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EFFECT_TIMING_H_

#include <memory>
#include <optional>
#include <utility>

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "core/animation/utils/timing_function.h"

namespace lynx {
namespace animation {
namespace basic {

struct OptionalAnimationEffectTiming;
class AnimationEffectTiming {
 public:
  enum class FillMode { kNone = 0, kForwards, kBackwards, kBoth };
  enum class PlaybackDirection {
    kNormal = 0,
    kReverse,
    kAlternate,
    kAlternateReverse
  };

  static std::unique_ptr<AnimationEffectTiming> Create(
      const fml::TimeDelta& delay, FillMode fill, int64_t iterations,
      const fml::TimeDelta& duration, PlaybackDirection direction,
      std::unique_ptr<TimingFunction> easing) {
    return std::unique_ptr<AnimationEffectTiming>(new AnimationEffectTiming(
        delay, fill, iterations, duration, direction, std::move(easing)));
  }

  static std::unique_ptr<AnimationEffectTiming> Create(
      std::unique_ptr<OptionalAnimationEffectTiming> timing) {
    return std::unique_ptr<AnimationEffectTiming>(
        new AnimationEffectTiming(std::move(timing)));
  }

  static std::unique_ptr<AnimationEffectTiming> Create() {
    return std::unique_ptr<AnimationEffectTiming>(new AnimationEffectTiming());
  }

  const fml::TimeDelta& delay() const { return delay_; }

  FillMode fill() const { return fill_; }

  double iterations() const { return iterations_; }

  const fml::TimeDelta& duration() const { return duration_; }

  PlaybackDirection direction() const { return direction_; }

  std::unique_ptr<TimingFunction>& easing() { return easing_; }

  double playback_rate() const { return playback_rate_; };

  void UpdateTiming(std::unique_ptr<OptionalAnimationEffectTiming> timing);

 private:
  AnimationEffectTiming(const fml::TimeDelta& delay, FillMode fill,
                        int64_t iterations, const fml::TimeDelta& duration,
                        PlaybackDirection direction,
                        std::unique_ptr<TimingFunction> easing)
      : delay_(delay),
        fill_(fill),
        iterations_(iterations),
        duration_(duration),
        direction_(direction),
        easing_(std::move(easing)) {}

  explicit AnimationEffectTiming(
      std::unique_ptr<OptionalAnimationEffectTiming> timing) {
    UpdateTiming(std::move(timing));
  }

  AnimationEffectTiming() = default;

  fml::TimeDelta delay_{fml::TimeDelta::Zero()};
  AnimationEffectTiming::FillMode fill_{FillMode::kNone};
  double iterations_{1.0};
  fml::TimeDelta duration_{fml::TimeDelta::Zero()};
  AnimationEffectTiming::PlaybackDirection direction_{
      PlaybackDirection::kNormal};
  std::unique_ptr<TimingFunction> easing_{LinearTimingFunction::Create()};
  double playback_rate_{1.0};
};

struct OptionalAnimationEffectTiming {
  static std::unique_ptr<OptionalAnimationEffectTiming> Create() {
    return std::make_unique<OptionalAnimationEffectTiming>();
  }
  std::optional<fml::TimeDelta> delay_;
  std::optional<AnimationEffectTiming::FillMode> fill_;
  std::optional<double> iterations_;
  std::optional<fml::TimeDelta> duration_;
  std::optional<AnimationEffectTiming::PlaybackDirection> direction_;
  std::unique_ptr<TimingFunction> easing_;
  double playback_rate_{1.0};
};

}  // namespace basic
}  // namespace animation
}  // namespace lynx

#endif  // CORE_ANIMATION_BASIC_ANIMATION_ANIMATION_EFFECT_TIMING_H_
