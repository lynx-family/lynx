// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_STYLE_TRANSFORM_RAW_DATA_H_
#define CORE_STYLE_TRANSFORM_RAW_DATA_H_

#include <array>
#include <tuple>

#include "core/renderer/css/css_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/renderer/starlight/types/nlength.h"

namespace lynx {
namespace starlight {
struct TransformRawData {
  static constexpr int INDEX_FUNC = 0;
  static constexpr int INDEX_TRANSLATE_0 = 1;
  static constexpr int INDEX_TRANSLATE_0_UNIT = 2;
  static constexpr int INDEX_TRANSLATE_1 = 3;
  static constexpr int INDEX_TRANSLATE_1_UNIT = 4;
  static constexpr int INDEX_TRANSLATE_2 = 5;
  static constexpr int INDEX_TRANSLATE_2_UNIT = 6;
  static constexpr int INDEX_ROTATE_ANGLE = 1;
  static constexpr int INDEX_SCALE_0 = 1;
  static constexpr int INDEX_SCALE_1 = 2;
  static constexpr int INDEX_SKEW_0 = 1;
  static constexpr int INDEX_SKEW_1 = 2;
  static constexpr int INDEX_MATRIX_UNIT = 0;

  static constexpr int INDEX_2D_TO_3D_MATRIX_ID[6] = {0, 1, 4, 5, 12, 13};

  static constexpr int INDEX_3D_MATRIX_ID[16] = {0, 1, 2,  3,  4,  5,  6,  7,
                                                 8, 9, 10, 11, 12, 13, 14, 15};

  TransformRawData();
  ~TransformRawData() = default;

  TransformType type;
  NLength p0;
  NLength p1;
  NLength p2;
  tasm::CSSValuePattern unit_type0 = tasm::CSSValuePattern::EMPTY;
  tasm::CSSValuePattern unit_type1 = tasm::CSSValuePattern::EMPTY;
  tasm::CSSValuePattern unit_type2 = tasm::CSSValuePattern::EMPTY;

  std::array<double, 16> matrix = {1, 0, 0, 0,   // {{1, 0, 0, 0}
                                   0, 1, 0, 0,   //  {0, 1, 0, 0}
                                   0, 0, 1, 0,   //  {0, 0, 1, 0}
                                   0, 0, 0, 1};  //  {0, 0, 0, 1}}

  bool matrix_empty = true;

  bool operator==(const TransformRawData& rhs) const {
    return std::tie(type, p0, p1, p2, matrix) ==
           std::tie(rhs.type, rhs.p0, rhs.p1, rhs.p2, rhs.matrix);
  }

  bool operator!=(const TransformRawData& rhs) const { return !(*this == rhs); }

  void Reset();

  bool Empty() {
    return (p0.GetRawValue() - 0 < 0.0001f) &&
           (p1.GetRawValue() - 0 < 0.0001f) &&
           (p2.GetRawValue() - 0 < 0.0001f) && matrix_empty;
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_TRANSFORM_RAW_DATA_H_
