// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_STYLE_OUTLINE_DATA_H_
#define CORE_STYLE_OUTLINE_DATA_H_

#include <tuple>

#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {
struct OutLineData {
  OutLineData();
  ~OutLineData() = default;
  float width;
  BorderStyleType style;
  unsigned int color;
  bool operator==(const OutLineData& rhs) const {
    return std::tie(width, style, color) ==
           std::tie(rhs.width, rhs.style, rhs.color);
  }
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_OUTLINE_DATA_H_
