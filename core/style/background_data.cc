// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/background_data.h"

#include "core/style/color.h"
#include "core/style/default_computed_style.h"

namespace lynx {
namespace starlight {

BackgroundData::BackgroundData()
    : color(DefaultColor::DEFAULT_COLOR),
      image_count(DefaultComputedStyle::DEFAULT_LONG) {}

bool BackgroundData::HasBackground() const {
  return color != DefaultColor::DEFAULT_COLOR ||
         image_count != DefaultComputedStyle::DEFAULT_LONG;
}

}  // namespace starlight
}  // namespace lynx
