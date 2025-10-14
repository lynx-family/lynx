// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_ANIMATION_TYPE_EVALUATOR_H_
#define CLAY_GFX_ANIMATION_TYPE_EVALUATOR_H_

#include "clay/gfx/geometry/box_shadow_operations.h"
#include "clay/gfx/geometry/filter_operations.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/gfx/style/color.h"

namespace clay {

template <typename T>
class TypeEvaluator {
 public:
  // This function returns the result of linearly interpolating the start and
  // end values. The calculation is a simple parametric calculation.
  // For non primitive types, such as transformation and color, a template
  // specialization should be defined.
  static T Evaluate(float fraction, T start_value, T end_value) {
    return start_value + (end_value - start_value) * fraction;
  }
};

template <>
class TypeEvaluator<Color> {
 public:
  static Color Evaluate(float fraction, Color start_value, Color end_value) {
    return Color::Lerp(start_value, end_value, fraction);
  }
};

template <>
class TypeEvaluator<TransformOperations> {
 public:
  static TransformOperations Evaluate(float fraction,
                                      const TransformOperations& start_value,
                                      const TransformOperations& end_value) {
    return end_value.Blend(start_value, fraction);
  }
};

template <>
class TypeEvaluator<FilterOperations> {
 public:
  static FilterOperations Evaluate(float fraction,
                                   const FilterOperations& start_value,
                                   const FilterOperations& end_value) {
    return end_value.Blend(start_value, fraction);
  }
};

template <>
class TypeEvaluator<BoxShadowOperations> {
 public:
  static BoxShadowOperations Evaluate(float fraction,
                                      const BoxShadowOperations& start_value,
                                      const BoxShadowOperations& end_value) {
    return end_value.Blend(start_value, fraction);
  }
};

}  // namespace clay

#endif  // CLAY_GFX_ANIMATION_TYPE_EVALUATOR_H_
