// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_ANIMATION_ANIMATION_UTILS_H_
#define GFX_ANIMATION_ANIMATION_UTILS_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

#include "gfx/animation/animation_keyframe.h"
#include "gfx/animation/timing_function.h"

namespace lynx {
namespace gfx {

enum class DiscreteFallback : uint8_t {
  kUseStart = 0,
  kUseEnd,
};

enum class ColorInterpolation : uint8_t {
  kAuto = 0,
  kLinearRGB,
  kSRGB,
};

struct KeyframedProgress {
  bool valid{false};
  fml::TimeDelta effective_time;
  size_t index{0};
  double progress{0.0};
};

KeyframedProgress ComputeKeyframedProgress(
    const std::vector<std::unique_ptr<Keyframe>>& keyframes,
    const TimingFunction* curve_timing_function, double scaled_duration,
    fml::TimeDelta time);

inline constexpr TaggedNumber MakeNumber(double v) {
  return TaggedNumber{v, UnitTag::kNumber};
}

inline constexpr TaggedNumber MakePercent(double v) {
  return TaggedNumber{v, UnitTag::kPercent};
}

inline constexpr Vec2Tagged MakeVec2(TaggedNumber x, TaggedNumber y) {
  return Vec2Tagged{x, y};
}

inline constexpr FilterValue MakeFilter(uint32_t function, double v,
                                        UnitTag unit) {
  return FilterValue{function, v, unit};
}

using ColorARGB32 = uint32_t;

double InterpolateNumber(double start, double end, double progress);

ColorARGB32 InterpolateColorARGB32(ColorARGB32 start, ColorARGB32 end,
                                   double progress,
                                   ColorInterpolation color_interp);

FilterValue InterpolateFilterValue(FilterValue start, FilterValue end,
                                   double progress, DiscreteFallback fallback);

Vec2Tagged InterpolateVec2Tagged(Vec2Tagged start, Vec2Tagged end,
                                 double progress, DiscreteFallback fallback);

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_ANIMATION_ANIMATION_UTILS_H_
