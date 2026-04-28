// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_STYLE_OUTLINE_DATA_H_
#define CORE_STYLE_OUTLINE_DATA_H_

#include "core/renderer/starlight/style/css_type.h"

namespace lynx {
namespace starlight {
struct OutLineData {
  OutLineData();
  ~OutLineData() = default;
  float width;
  unsigned int color;
  BorderStyleType style;
  bool operator==(const OutLineData& rhs) const {
    return width == rhs.width && style == rhs.style && color == rhs.color;
  }

  // A flag telling `base::flex_optional<>` to save memory.
  using AlwaysUseFlexOptionalMemSave = bool;
};
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_STYLE_OUTLINE_DATA_H_
