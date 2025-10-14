// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_MEASURE_UTILS_H_
#define CLAY_UI_SHADOW_MEASURE_UTILS_H_

namespace clay {

class MeasureUtils {
 public:
  static bool isUndefined(float value) {
    return (value >= static_cast<float>(10E8) ||
            value <= static_cast<float>(-10E8));
  }
};

enum class TextUpdateFlag {
  // None means either there is no update or the text view's width/height
  // changed.
  // In this case, we don't need to re-create the text builder.
  kUpdateFlagNone = 0,
  kUpdateFlagStyle = 1,
  kUpdateFlagChildren = 1 << 1,
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_MEASURE_UTILS_H_
