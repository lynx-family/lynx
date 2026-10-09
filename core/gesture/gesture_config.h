// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_GESTURE_CONFIG_H_
#define CORE_GESTURE_GESTURE_CONFIG_H_

#include <optional>
#include <vector>

#include "core/gesture/gesture_types.h"

namespace lynx::tasm::gesture {

std::optional<GestureDefinition> SnapshotGestureDetector(
    const GestureDetector& detector);

std::vector<GestureDefinition> NormalizeGestureDetectors(
    const std::vector<const GestureDetector*>& detectors);

std::vector<GestureDefinition> NormalizeGestureDefinitions(
    std::vector<GestureDefinition> definitions);

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_GESTURE_CONFIG_H_
