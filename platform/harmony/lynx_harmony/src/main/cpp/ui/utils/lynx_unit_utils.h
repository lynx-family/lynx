// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UNIT_UTILS_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UNIT_UTILS_H_

#include <string>

namespace lynx {
namespace tasm {
namespace harmony {

class LynxUnitUtils {
 public:
  static float ToVPFromUnitValue(const std::string& value, float screen_width,
                                 float physical_pixels_per_layout_unit,
                                 float default_value = 0.0);

 private:
  static float StringToFloat(const std::string& value, float scale,
                             float default_value);
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_LYNX_UNIT_UTILS_H_
