// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_ELEMENT_POINT_CONVERTER_H_
#define CORE_RENDERER_DOM_ELEMENT_POINT_CONVERTER_H_

#include <cstdint>

#include "base/include/flex_optional.h"
#include "base/include/geometry/point.h"
#include "base/include/geometry/rect.h"

namespace lynx {
namespace lepus {
class Value;
}  // namespace lepus

namespace tasm {

class Element;

enum class CoordinateSpace : int32_t {
  kElement = 0,
  kWindow = 1,
  kScreen = 2,
};

// Converts a point between Element-local border-box coordinate systems using
// Element-owned layout state. Returns no value when Layout-in-Element is not
// enabled or the Elements do not share an attached layout tree.
base::flex_optional<base::geometry::FloatPoint> ConvertPointBetweenElements(
    const base::geometry::FloatPoint& point, Element* from, Element* to);

// Converts a rect between Element-local border-box coordinate systems. The
// result is the axis-aligned bounding box of the four converted corners. When
// clip_bounds is true, the result is additionally intersected with clipping
// layout ancestors in the target coordinate system.
base::flex_optional<base::geometry::FloatRect> ConvertRectBetweenElements(
    const base::geometry::FloatRect& rect, Element* from, Element* to,
    bool clip_bounds);

// Converts Element-local geometry to page window or screen coordinates. Input,
// output, and the stored Page offset all use layout units.
base::flex_optional<base::geometry::FloatPoint> ConvertPointToPageCoordinate(
    const base::geometry::FloatPoint& point, Element* from,
    CoordinateSpace space);

base::flex_optional<base::geometry::FloatRect> ConvertRectToPageCoordinate(
    const base::geometry::FloatRect& rect, Element* from, bool clip_bounds,
    CoordinateSpace space);

// Converts Element API CSS-pixel inputs to the corresponding LEPUS values.
lepus::Value ConvertPointForElementAPI(double x, double y, Element* from,
                                       Element* to, CoordinateSpace space);
lepus::Value ConvertRectForElementAPI(double left, double top, double right,
                                      double bottom, Element* from, Element* to,
                                      bool clip_bounds, CoordinateSpace space);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_ELEMENT_POINT_CONVERTER_H_
