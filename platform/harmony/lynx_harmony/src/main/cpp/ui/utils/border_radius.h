// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BORDER_RADIUS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BORDER_RADIUS_H_

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"

namespace lynx {
namespace tasm {
namespace harmony {

class BorderRadius {
 public:
  enum class CornerPosition : uint8_t {
    kTopLeft = 0,
    kTopRight,
    kBottomRight,
    kBottomLeft,
  };
  /**
   * This method verify radius on each axis are identical via computed values.
   * Call compute with corresponding, size first.
   * @return true if radius on x-axis and y-axis are identical.
   */
  bool IsIdentical() const;
  bool IsZero() const;
  void Compute(float width, float height);
  const float* ComputedValue() const { return computed_value_; }

  /**
   * Set radius value at the corner.
   * @param position Corner position of the radius.
   * @param value The value of the radius, should be a lepus::Array contains the
   * values for the corner. The array should be arranged follow [x val, x val
   * type, y val, y val type]. Value type is one of [ 0 - Number, 1 -
   * Percentage, 2 - Calc].
   * @param offset The offset from the first element of the radius value to
   * array's head.
   */
  void SetRadius(CornerPosition position, const lepus::Value& value,
                 int offset = 0);

 private:
  PlatformLength radius_[8];
  float computed_value_[8] = {0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_BORDER_RADIUS_H_
