// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_LAYOUT_LOGIC_DIRECTION_UTILS_H_
#define CORE_RENDERER_STARLIGHT_LAYOUT_LOGIC_DIRECTION_UTILS_H_

#include "core/renderer/starlight/layout/box_info.h"
#include "core/renderer/starlight/layout/layout_object.h"

namespace lynx {
namespace starlight {
class LayoutObject;
namespace logic_direction_utils {

float GetMarginBoundDimensionSize(const LayoutObject* item, Dimension axis);

float GetPaddingBoundDimensionSize(const LayoutObject* item, Dimension axis);

float GetContentBoundDimensionSize(const LayoutObject* item, Dimension axis);

float GetBorderBoundDimensionSize(const LayoutObject* item, Dimension axis);

void ResolveAutoMargins(LayoutObject* item, float content_size, Dimension axis);
void ResolveAlignContent(const LayoutComputedStyle* css_style,
                         int32_t sub_item_count, float available_space,
                         float& axis_interval, float& axis_start,
                         bool negative_space_with_gap = false);
void ResolveJustifyContent(const LayoutComputedStyle* css_style,
                           int32_t sub_item_count, float available_space,
                           float& axis_interval, float& axis_start,
                           bool negative_space_with_gap = false);

void SetBoundOffsetFrom(LayoutObject* item, Direction front,
                        BoundType bound_type, BoundType parent_bound_type,
                        float offset);

float GetBoundOffsetFrom(const LayoutObject* item, Dimension axis,
                         BoundType bound_type, BoundType parent_bound_type);

float GetPaddingAndBorderDimensionSize(const LayoutObject* item,
                                       Dimension axis);

const NLength& GetCSSDimensionSize(const LayoutComputedStyle* cssStyle,
                                   Dimension axis);

float ClampExactSize(const LayoutObject* item, float size, Dimension axis);

LinearGravityType GetLogicGravityType(LinearGravityType logic_gravity_type,
                                      Direction main_front);

const NLength GetSurroundOffset(const LayoutComputedStyle* cssStyle,
                                Direction direction);
const NLength GetMargin(const LayoutComputedStyle* cssStyle,
                        Direction direction);
const NLength GetPadding(const LayoutComputedStyle* cssStyle,
                         Direction direction);

inline float& SizeDimension(FloatSize& size, Dimension dimension) {
  return dimension == kHorizontal ? size.width_ : size.height_;
}

inline const float& SizeDimension(const FloatSize& size, Dimension dimension) {
  return dimension == kHorizontal ? size.width_ : size.height_;
}

inline Direction DimensionPhysicalStart(Dimension dimension) {
  return dimension == kHorizontal ? kLeft : kTop;
}

inline Direction DimensionPhysicalEnd(Dimension dimension) {
  return dimension == kHorizontal ? kRight : kBottom;
}

}  // namespace logic_direction_utils
}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_LAYOUT_LOGIC_DIRECTION_UTILS_H_
