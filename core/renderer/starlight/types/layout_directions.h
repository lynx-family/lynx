// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_DIRECTIONS_H_
#define CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_DIRECTIONS_H_

#include <array>

namespace lynx {
namespace starlight {

enum Dimension : int { kHorizontal = 0, kVertical = 1, kDimensionCount = 2 };

enum Direction : int {
  kLeft = 0,
  kRight = 1,
  kTop = 2,
  kBottom = 3,
  kDirectionCount = 4,
};

enum class Position : int {
  kStart = -1,
  kCenter = 0,
  kEnd = 1,
};

using BoxPositions = std::array<Position, 2>;

template <typename T>
using DimensionValue = std::array<T, kDimensionCount>;

template <typename T>
using DirectionValue = std::array<T, kDirectionCount>;

}  // namespace starlight
}  // namespace lynx

#endif  // CORE_RENDERER_STARLIGHT_TYPES_LAYOUT_DIRECTIONS_H_
