// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/lynx_unit_utils.h"

#include <limits>
#include <vector>

#include "base/include/string/string_number_convert.h"
#include "core/renderer/tasm/config.h"

namespace lynx {
namespace tasm {
namespace harmony {

float LynxUnitUtils::StringToFloat(const std::string& digit, const float scale,
                                   const float default_value) {
  if (float value = 0; base::StringToFloat(digit, value, true)) {
    return value * scale;
  } else {
    return default_value;
  }
}

float LynxUnitUtils::ToVPFromUnitValue(
    const std::string& value, const float screen_width,
    const float physical_pixels_per_layout_unit, const float default_value) {
  const std::size_t size = value.size();
  if (const size_t rpx_pos = value.rfind("rpx"); rpx_pos == size - 3) {
    return StringToFloat(value.substr(0, rpx_pos), screen_width / 750.0,
                         default_value);
  } else if (const size_t ppx_pos = value.rfind("ppx"); ppx_pos == size - 3) {
    return StringToFloat(value.substr(0, ppx_pos),
                         1.0 / physical_pixels_per_layout_unit, default_value);
  } else if (const size_t px_pos = value.rfind("px"); px_pos == size - 2) {
    return StringToFloat(value.substr(0, px_pos), 1.0, default_value);
  }
  return default_value;
}
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
