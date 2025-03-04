// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/flex_info.h"

#include <algorithm>

namespace lynx {
namespace starlight {

void FlexInfo::Reset() {
  std::fill(flex_base_size_.begin(), flex_base_size_.end(), 0);
  std::fill(hypothetical_main_size_.begin(), hypothetical_main_size_.end(), 0);
  std::fill(hypothetical_cross_size_.begin(), hypothetical_cross_size_.end(),
            0);
  std::fill(flex_main_size_.begin(), flex_main_size_.end(), 0);
  std::fill(flex_cross_size_.begin(), flex_cross_size_.end(), 0);
  std::fill(apply_stretch_later_.begin(), apply_stretch_later_.end(), false);

  line_info_.clear();
}
}  // namespace starlight
}  // namespace lynx
