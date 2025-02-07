// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_POSITION_LAYOUT_UTILS_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_POSITION_LAYOUT_UTILS_H_

#include <array>

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_global.h"
#include "core/renderer/starlight/layout/logic_direction_utils.h"
#include "core/renderer/starlight/types/layout_directions.h"
#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
namespace starlight {
class LayoutObject;

namespace position_utils {

void CalcRelativePosition(LayoutObject* item,
                          const Constraints& content_constraints);

void CalcAbsoluteOrFixedPosition(
    LayoutObject* absolute_or_fixed_item, LayoutObject* container,
    const Constraints& container_constraints,
    BoxPositions absolute_or_fixed_item_initial_position,
    std::array<Direction, 2> directions);

Constraints GetAbsoluteOrFixedItemSizeAndMode(
    LayoutObject* absolute_or_fixed_item, LayoutObject* container,
    const Constraints& content_constraints);

void CalcStartOffset(LayoutObject* absolute_or_fixed_item,
                     BoundType container_bound_type,
                     DimensionValue<Position> positions,
                     const Constraints& containing_block, Dimension dimension,
                     Direction direction, float offset = 0.f);

void UpdateStickyItemPosition(LayoutObject* sticky_item, float screen_width,
                              const Constraints& content_constraints);

Position ReversePosition(Position pos);

Constraints GetContainingBlockForAbsoluteAndFixed(
    LayoutObject* container, const Constraints& base_containing_block);

}  // namespace position_utils
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_POSITION_LAYOUT_UTILS_H_
