// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/style/transition_data.h"

#include "core/renderer/starlight/style/css_type.h"
#include "core/style/default_computed_style.h"

namespace lynx {
namespace starlight {

TransitionData::TransitionData()
    : duration(DefaultComputedStyle::DEFAULT_LONG),
      delay(DefaultComputedStyle::DEFAULT_LONG),
      property(AnimationPropertyType::kNone) {}
}  // namespace starlight
}  // namespace lynx
