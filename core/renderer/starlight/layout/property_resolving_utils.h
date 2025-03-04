// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_PROPERTY_RESOLVING_UTILS_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_PROPERTY_RESOLVING_UTILS_H_

#include <stdio.h>

#include "core/renderer/starlight/types/layout_types.h"

namespace lynx {
namespace starlight {
class LayoutComputedStyle;
class BoxInfo;
namespace property_utils {

void HandleBoxSizing(const LayoutComputedStyle& style, const BoxInfo& box_info,
                     DimensionValue<LayoutUnit>& size,
                     const LayoutConfigs& layout_config);

DimensionValue<LayoutUnit> ComputePreferredSize(
    const LayoutObject& item, const Constraints& container_constraint);

void ApplyAspectRatio(const LayoutObject* layout_object, Constraints& size);

Constraints GenerateDefaultConstraints(const LayoutObject& item,
                                       const Constraints& container_constraint);

void ApplyMinMaxToConstraints(Constraints& constraints,
                              const LayoutObject& item);

float ApplyMinMaxToSpecificSize(float size, const LayoutObject* item,
                                Dimension dimension);

float StripMargins(float value, const LayoutObject& obj, Dimension dimension);
}  // namespace property_utils

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_PROPERTY_RESOLVING_UTILS_H_
