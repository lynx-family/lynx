// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/outline_data.h"

#include "core/style/color.h"
#include "core/style/default_computed_style.h"

namespace lynx {
namespace starlight {

OutLineData::OutLineData()
    : width(DefaultComputedStyle::DEFAULT_FLOAT),
      style(DefaultComputedStyle::DEFAULT_OUTLINE_STYLE),
      color(DefaultColor::DEFAULT_OUTLINE_COLOR) {}
}  // namespace starlight
}  // namespace lynx
