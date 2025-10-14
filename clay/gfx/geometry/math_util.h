// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_MATH_UTIL_H_
#define CLAY_GFX_GEOMETRY_MATH_UTIL_H_

#include <cmath>
#include <limits>

namespace clay {

constexpr float linearInterpolate(float from, float to, float progress) {
  return from * (1 - progress) + to * progress;
}

constexpr double DegToRad(double deg) { return deg * M_PI / 180.0; }
constexpr float DegToRad(float deg) { return deg * M_PI / 180.0f; }

constexpr double RadToDeg(double rad) { return rad * 180.0 / M_PI; }
constexpr float RadToDeg(float rad) { return rad * 180.0f / M_PI; }

template <typename T>
constexpr bool IsApproximatelyEqual(T lhs, T rhs, T tolerance) {
  static_assert(std::is_arithmetic<T>::value, "Argument must be arithmetic");
  return std::abs(rhs - lhs) <= tolerance;
}

// Convenience function that returns true if the supplied value is in range
// for the destination type.
template <typename Dst, typename Src>
constexpr bool IsValueInRangeForNumericType(Src value) {
  static_assert(std::is_arithmetic<Src>::value, "Argument must be numeric.");
  static_assert(std::is_arithmetic<Dst>::value, "Result must be numeric.");
  static_assert(
      std::numeric_limits<Src>::lowest() < std::numeric_limits<Dst>::lowest(),
      "");
  static_assert(
      std::numeric_limits<Src>::max() > std::numeric_limits<Dst>::max(), "");
  return std::numeric_limits<Dst>::lowest() < value &&
         value < static_cast<Src>(std::numeric_limits<Dst>::max());
}

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_MATH_UTIL_H_
