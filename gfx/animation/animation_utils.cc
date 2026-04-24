// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "gfx/animation/animation_utils.h"

#include <cmath>
#include <limits>

namespace lynx {
namespace gfx {

namespace {

fml::TimeDelta TransformedAnimationTime(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    const TimingFunction* timing_function, double scaled_duration,
    fml::TimeDelta time) {
  if (timing_function) {
    fml::TimeDelta start_time = keyframes.front()->Time() * scaled_duration;
    fml::TimeDelta duration =
        (keyframes.back()->Time() - keyframes.front()->Time()) *
        scaled_duration;
    double progress = static_cast<double>(time.ToMicroseconds() -
                                          start_time.ToMicroseconds()) /
                      static_cast<double>(duration.ToMicroseconds());
    time = (duration * timing_function->GetValue(progress)) + start_time;
  }
  return time;
}

size_t GetActiveKeyframe(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time) {
  size_t i = 0;
  for (; i < keyframes.size() - 2; ++i) {
    if (time < (keyframes[i + 1]->Time() * scaled_duration)) {
      break;
    }
  }
  return i;
}

double TransformedKeyframeProgress(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    double scaled_duration, fml::TimeDelta time, size_t i) {
  double in_time = time.ToNanosecondsF();
  double time1 = keyframes[i]->Time().ToNanosecondsF() * scaled_duration;
  double time2 = keyframes[i + 1]->Time().ToNanosecondsF() * scaled_duration;
  if (std::fabs(time2 - time1) < std::numeric_limits<double>::epsilon()) {
    return 1.0;
  }
  double progress = (in_time - time1) / (time2 - time1);
  if (keyframes[i]->timing_function()) {
    progress = keyframes[i]->timing_function()->GetValue(progress);
  }
  return progress;
}

double Lerp(double a, double b, double p) { return a + (b - a) * p; }

uint8_t ClampToByte(double v) {
  if (v < 0.0) {
    return 0;
  }
  if (v > 255.0) {
    return 255;
  }
  return static_cast<uint8_t>(std::lround(v));
}

ColorARGB32 InterpColor(ColorARGB32 a, ColorARGB32 b, double p,
                        ColorInterpolation mode) {
  double gamma = 1.0;
  if (mode == ColorInterpolation::kSRGB || mode == ColorInterpolation::kAuto) {
    gamma = 2.2;
  }

  auto aA = ((a >> 24) & 0xff) / 255.0;
  auto aR = ((a >> 16) & 0xff) / 255.0;
  auto aG = ((a >> 8) & 0xff) / 255.0;
  auto aB = ((a) & 0xff) / 255.0;
  auto bA = ((b >> 24) & 0xff) / 255.0;
  auto bR = ((b >> 16) & 0xff) / 255.0;
  auto bG = ((b >> 8) & 0xff) / 255.0;
  auto bB = ((b) & 0xff) / 255.0;

  if (mode == ColorInterpolation::kLinearRGB ||
      mode == ColorInterpolation::kSRGB || mode == ColorInterpolation::kAuto) {
    aR = std::pow(aR, gamma);
    aG = std::pow(aG, gamma);
    aB = std::pow(aB, gamma);
    bR = std::pow(bR, gamma);
    bG = std::pow(bG, gamma);
    bB = std::pow(bB, gamma);
  }

  double oA = Lerp(aA, bA, p);
  double oR = Lerp(aR, bR, p);
  double oG = Lerp(aG, bG, p);
  double oB = Lerp(aB, bB, p);

  if (mode == ColorInterpolation::kLinearRGB ||
      mode == ColorInterpolation::kSRGB || mode == ColorInterpolation::kAuto) {
    oR = std::pow(oR, 1.0 / gamma);
    oG = std::pow(oG, 1.0 / gamma);
    oB = std::pow(oB, 1.0 / gamma);
  }

  uint32_t A = ClampToByte(oA * 255.0);
  uint32_t R = ClampToByte(oR * 255.0);
  uint32_t G = ClampToByte(oG * 255.0);
  uint32_t B = ClampToByte(oB * 255.0);
  return (A << 24) | (R << 16) | (G << 8) | B;
}

template <typename T>
T DiscretePick(const T& start, const T& end, DiscreteFallback fallback) {
  return (fallback == DiscreteFallback::kUseEnd) ? end : start;
}

}  // namespace

KeyframedProgress ComputeKeyframedProgress(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    const TimingFunction* curve_timing_function, double scaled_duration,
    fml::TimeDelta time) {
  KeyframedProgress out;
  if (keyframes.size() < 2) {
    return out;
  }
  out.valid = true;
  out.effective_time = TransformedAnimationTime(
      keyframes, curve_timing_function, scaled_duration, time);
  out.index = GetActiveKeyframe(keyframes, scaled_duration, out.effective_time);
  out.progress = TransformedKeyframeProgress(keyframes, scaled_duration,
                                             out.effective_time, out.index);
  return out;
}

double InterpolateNumber(double start, double end, double progress) {
  return Lerp(start, end, progress);
}

ColorARGB32 InterpolateColorARGB32(ColorARGB32 start, ColorARGB32 end,
                                   double progress,
                                   ColorInterpolation color_interp) {
  return InterpColor(start, end, progress, color_interp);
}

FilterValue InterpolateFilterValue(FilterValue start, FilterValue end,
                                   double progress, DiscreteFallback fallback) {
  if (start.function != end.function || start.unit != end.unit) {
    return DiscretePick(start, end, fallback);
  }
  return FilterValue{start.function, Lerp(start.value, end.value, progress),
                     start.unit};
}

Vec2Tagged InterpolateVec2Tagged(Vec2Tagged start, Vec2Tagged end,
                                 double progress, DiscreteFallback fallback) {
  if (start.x.tag != end.x.tag || start.y.tag != end.y.tag) {
    return DiscretePick(start, end, fallback);
  }
  return Vec2Tagged{
      TaggedNumber{Lerp(start.x.value, end.x.value, progress), start.x.tag},
      TaggedNumber{Lerp(start.y.value, end.y.value, progress), start.y.tag}};
}

}  // namespace gfx
}  // namespace lynx
