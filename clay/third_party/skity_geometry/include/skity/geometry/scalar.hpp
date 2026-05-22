// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>

#ifndef INCLUDE_SKITY_GEOMETRY_SCALAR_HPP
#define INCLUDE_SKITY_GEOMETRY_SCALAR_HPP

namespace skity {

#define Float1 1.0f
#define FloatHalf 0.5f
#define FloatNaN std::numeric_limits<float>::quiet_NaN()
#define FloatInfinity std::numeric_limits<float>::infinity()
#define FloatRoot2Over2 0.707106781f
#define FloatSqrt2 1.41421356f

constexpr static float kNearlyZero = (Float1 / (1 << 12));

/**
 *  Returns -1 || 0 || 1 depending on the sign of value:
 *  -1 if x < 0
 *   0 if x == 0
 *   1 if x > 0
 */
static inline int FloatSignAsInt(float x) { return x < 0 ? -1 : (x > 0); }

static inline int RoundToInt(float x) {
  return static_cast<int>(std::round(x));
}

static inline bool FloatNearlyZero(float x, float tolerance = kNearlyZero) {
  return std::abs(x) <= tolerance;
}

static inline float FloatFract(float x) { return x - std::floor(x); }

static inline float FloatInterp(float A, float B, float t) {
  return A + (B - A) * t;
}

static inline float FloatInterpFunc(float search_key, const float keys[],
                                    const float values[], int length) {
  int right = 0;
  while (right < length && keys[right] < search_key) {
    ++right;
  }
  if (right == length) {
    return values[length - 1];
  }
  if (right == 0) {
    return values[0];
  }
  // Otherwise, interpolate between right - 1 and right.
  float range_left = keys[right - 1];
  float range_right = keys[right];
  float t = (search_key - range_left) / (range_right - range_left);
  return FloatInterp(values[right - 1], values[right], t);
}

static inline float SkityFloatHalf(float v) { return v * FloatHalf; }

static inline bool FloatIsNan(float x) { return x != x; }

[[clang::no_sanitize("float-divide-by-zero")]] static inline float
SkityIEEEFloatDivided(float number, float denom) {
  return number / denom;
}

#define FloatInvert(x) SkityIEEEFloatDivided(Float1, (x))

static inline bool FloatIsFinite(float x) { return !std::isinf(x); }

static inline float FloatSinSnapToZero(float radians) {
  float v = std::sin(radians);
  return FloatNearlyZero(v) ? 0.f : v;
}

static inline float FloatCosSnapToZero(float radians) {
  float v = std::cos(radians);
  return FloatNearlyZero(v) ? 0.f : v;
}

static inline float FloatTanSnapToZero(float radians) {
  float v = std::tan(radians);
  return FloatNearlyZero(v) ? 0.f : v;
}

static inline float FloatCopySign(float v1, float v2) {
  return std::copysignf(v1, v2);
}

static inline float FloatRadiansToDegrees(float radians) {
  return radians * static_cast<float>(57.295779513082320876798154814105);
}

static inline float FloatDegreesToRadians(float degrees) {
  return degrees * static_cast<float>(0.01745329251994329576923690768489);
}

}  // namespace skity

#endif  // INCLUDE_SKITY_GEOMETRY_SCALAR_HPP
