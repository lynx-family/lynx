// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_GESTURE_DETECTOR_H_
#define CLAY_UI_GESTURE_HANDLER_GESTURE_DETECTOR_H_

#include <cstdint>
#include <unordered_map>

#include "core/gesture/gesture_types.h"

namespace clay {

using GestureMap =
    std::unordered_map<uint32_t, lynx::tasm::gesture::GestureDefinition>;

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_GESTURE_DETECTOR_H_
