// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_VALUE_H_
#define CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_VALUE_H_

#include <math.h>

#include <array>

#include "core/services/starlight_standalone/core/include/starlight_enums.h"

namespace starlight {

template <typename T>
using SLDimensionValue = std::array<T, kSLDimensionCount>;

struct SLOneSideConstraint {
  float size_;
  StarlightMeasureMode mode_;
};

using SLConstraints = SLDimensionValue<SLOneSideConstraint>;

struct SLSize {
  float width_ = 0.0f;
  float height_ = 0.0f;

  SLSize() : width_(0.0f), height_(0.0f) {}
  SLSize(float width, float height) : width_(width), height_(height) {}
};

struct SLLength {
  float value_{0.0f};
  SLLengthType type_{SLLengthType::kSLLengthPPX};

  SLLength() : value_(0.0f), type_(SLLengthType::kSLLengthPPX) {}
  constexpr SLLength(float value, SLLengthType type)
      : value_(value), type_(type) {}
};

struct SLPoint {
  float x_{0.0f};
  float y_{0.0f};

  SLPoint() : x_(0.0f), y_(0.0f) {}
  SLPoint(float x, float y) : x_(x), y_(y) {}
};

inline constexpr SLLength kSLUndefinedLength{NAN, kSLLengthPPX};
inline constexpr SLLength kSLAutoLength{0, kSLLengthAuto};
constexpr float kSLUndefinedValue = NAN;

}  // namespace starlight

#endif  // CORE_SERVICES_STARLIGHT_STANDALONE_CORE_INCLUDE_STARLIGHT_VALUE_H_
