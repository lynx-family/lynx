// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_COMPOSE_LAYOUT_UTILS_H_
#define CORE_RENDERER_DOM_FIBER_COMPOSE_LAYOUT_UTILS_H_

#include <array>

#include "core/renderer/starlight/types/layout_constraints.h"

namespace lynx {
namespace starlight {
class LayoutObject;
}
namespace tasm {
namespace compose_layout_utils {

struct SizeRangeMeasureResult {
  FloatSize size;
  FloatSize child_offset;
};

// Measures a child under Compose preferred min/max constraints. Limits are
// logical pixels in min-width, min-height, max-width, max-height order, with -1
// representing positive infinity. The offset centers a child whose measured
// size needs coercion. Descendant layout policies handle their own children.
SizeRangeMeasureResult MeasureSizeRange(starlight::LayoutObject& child,
                                        const starlight::Constraints& incoming,
                                        const std::array<double, 4>& limits,
                                        double layouts_unit_per_px,
                                        bool final_measure);

}  // namespace compose_layout_utils
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_COMPOSE_LAYOUT_UTILS_H_
