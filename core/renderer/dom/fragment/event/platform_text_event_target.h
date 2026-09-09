// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_TEXT_EVENT_TARGET_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_TEXT_EVENT_TARGET_H_

#include <cstdint>

#include "base/include/vector.h"

namespace lynx::tasm {

struct PlatformTextEventTargetRange {
  int32_t sign{0};
  int32_t start{0};
  int32_t end{0};
  int32_t text_sign{0};
};

struct PlatformTextEventTargetRegion {
  int32_t sign{0};
  float left{0.f};
  float top{0.f};
  float width{0.f};
  float height{0.f};
};

using PlatformTextEventTargetRegions =
    base::Vector<PlatformTextEventTargetRegion>;

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_TEXT_EVENT_TARGET_H_
