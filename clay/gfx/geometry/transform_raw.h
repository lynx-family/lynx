// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_RAW_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_RAW_H_

#include "clay/gfx/style/length.h"

namespace clay {

struct TransformRaw {
  int type;
  Length values[3];
  double matrix[16];
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_RAW_H_
