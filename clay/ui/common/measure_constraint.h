// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_MEASURE_CONSTRAINT_H_
#define CLAY_UI_COMMON_MEASURE_CONSTRAINT_H_

#include <optional>

namespace clay {

enum class MeasureMode {
  kIndefinite = 0,  // 不指定宽高
  kDefinite,        // 指定宽高
  kAtMost,          // 排版结果最大为指定的宽高
};

struct MeasureConstraint {
  std::optional<float> width;
  MeasureMode width_mode;
  std::optional<float> height;
  MeasureMode height_mode;

  bool IsValid() const {
    return (width_mode == MeasureMode::kIndefinite ||
            (width.has_value() && *width > 0.0)) &&
           (height_mode == MeasureMode::kIndefinite ||
            (height.has_value() && *height > 0.0));
  }

  bool operator==(const MeasureConstraint& constraint) const {
    return this->width == constraint.width &&
           this->width_mode == constraint.width_mode &&
           this->height == constraint.height &&
           this->height_mode == constraint.height_mode;
  }
};

struct MeasureResult {
  float width = 0.f;
  float height = 0.f;
  float baseline = 0.f;
};

}  // namespace clay

namespace std {
template <>
struct hash<clay::MeasureConstraint> {
  std::size_t operator()(
      const clay::MeasureConstraint& measure_constraint) const {
    int prime = 31;
    size_t result = 1;
    if (measure_constraint.width.has_value()) {
      result = result * prime + std::hash<float>()(*measure_constraint.width);
    }
    if (measure_constraint.height.has_value()) {
      result = result * prime + std::hash<float>()(*measure_constraint.height);
    }
    result = result * prime +
             std::hash<clay::MeasureMode>()(measure_constraint.width_mode);
    result = result * prime +
             std::hash<clay::MeasureMode>()(measure_constraint.height_mode);
    return result;
  }
};
}  // namespace std

#endif  // CLAY_UI_COMMON_MEASURE_CONSTRAINT_H_
