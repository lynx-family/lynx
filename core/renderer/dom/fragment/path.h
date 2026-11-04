// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_PATH_H_
#define CORE_RENDERER_DOM_FRAGMENT_PATH_H_

#include <cstdint>
#include <variant>
#include <vector>

#include "gfx/public/rounded_rectangle.h"

namespace lynx {
namespace tasm {

// TODO(songshourui.null): Move the path types to gfx when another drawing
// producer needs them. CSS clip-path is currently the only producer.
enum class PathKind : uint32_t {
  kCommands = 0,
  kCircle = 1,
  kEllipse = 2,
  kRect = 3,
  kSuperEllipse = 5,
};
enum class PathFillType : uint32_t { kNonZero = 0, kEvenOdd = 1 };
enum class PathVerb : uint32_t { kMove, kLine, kCubic, kClose };
struct PathCommand {
  PathVerb verb;
  float x, y, x1, y1, x2, y2;  // Endpoint, then cubic controls.
};
struct Path {
  PathFillType fill_type = PathFillType::kNonZero;
  std::vector<PathCommand> commands;
};
struct CirclePath {
  float cx, cy, radius;
};
struct EllipsePath {
  float cx, cy, rx, ry;
};
// DisplayList uses the native field order: x, y, width, height; four X
// radii, four Y radii (TL, TR, BR, BL in each group); then both exponents.
struct RectPath {
  RoundedRectangle rect;
  float exponent_x = 2, exponent_y = 2;
};
struct SuperEllipsePath {
  float cx, cy, rx, ry, exponent_x, exponent_y;
};

using PathGeometry =
    std::variant<Path, CirclePath, EllipsePath, RectPath, SuperEllipsePath>;

}  // namespace tasm
}  // namespace lynx
#endif  // CORE_RENDERER_DOM_FRAGMENT_PATH_H_
