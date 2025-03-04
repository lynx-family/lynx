// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FLOAT_COMPARISON_H_
#define BASE_INCLUDE_FLOAT_COMPARISON_H_

#include <math.h>

namespace lynx {
namespace base {

// the error range in floating-point number comparison, with a value of 0.01f.
constexpr float EPSILON = 0.01f;
// a more precise error range in floating-point number comparison, with a value
// of 0.001f.
constexpr float EPSILON_PRECISE = 1e-3;
// the error range in double-precision floating-point number comparison, with a
// value of 0.000001.
constexpr double EPSILON_DOUBLE = 1e-6;

inline bool DoublesEqual(const double first, const double second) {
  return fabs(first - second) < EPSILON_DOUBLE;
}

inline bool IsZero(const double d) { return DoublesEqual(d, 0.00000L); }

inline bool FloatsEqual(const float first, const float second) {
  return fabs(first - second) < EPSILON;
}

inline bool FloatsEqualPrecise(const float first, const float second) {
  return fabs(first - second) < EPSILON_PRECISE;
}

inline bool FloatsNotEqual(const float first, const float second) {
  return fabs(first - second) >= EPSILON;
}

inline bool FloatsLarger(const float first, const float second) {
  return fabs(first - second) >= EPSILON && first > second;
}

inline bool FloatsLargerPrecise(const float first, const float second) {
  return fabs(first - second) >= EPSILON_PRECISE && first > second;
}

inline bool FloatsLargerOrEqual(const float first, const float second) {
  return first > second || fabs(first - second) < EPSILON;
}

inline bool IsZero(const float f) { return FloatsEqual(f, 0.0f); }

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_FLOAT_COMPARISON_H_
