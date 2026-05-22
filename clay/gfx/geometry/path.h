// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_PATH_H_
#define CLAY_GFX_GEOMETRY_PATH_H_

#include <string>
#include <utility>
#include <vector>

#include "clay/gfx/rendering_backend.h"
#include "clay/public/clay.h"
#include "clay/public/style_types.h"

namespace clay {

struct ClipPathData {
  enum class ClipType {
    kUnknown = 0,
    kCircle,
    kEllipse,
    kPath,
    kSuperEllipse,
    kInset,
  };
  enum class CornerType {
    kUnknown,
    kCornerRect,
    kCornerRounded,
    kCornerSuperElliptical,
  };
  struct BorderRadius {
    ClayPlatformLength x;
    ClayPlatformLength y;
  };
  ClipType clip_type = ClipType::kUnknown;
  CornerType corner_type = CornerType::kUnknown;
  std::vector<ClayPlatformLength> params = {};
  std::vector<double> exponents = {};
  std::vector<BorderRadius> radius = {};
};

struct PathBuilder {
  static bool ParsePathString(const char data[], GrPath* result);
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_PATH_H_
