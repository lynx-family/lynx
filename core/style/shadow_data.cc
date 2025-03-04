// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/shadow_data.h"

#include "base/include/float_comparison.h"
#include "core/style/color.h"

namespace lynx {
namespace starlight {
void ShadowData::Reset() {
  h_offset = 0;
  v_offset = 0;
  blur = 0;
  spread = 0;
  color = DefaultColor::DEFAULT_SHADOW_COLOR;
  option = ShadowOption::kNone;
}

}  // namespace starlight
}  // namespace lynx
