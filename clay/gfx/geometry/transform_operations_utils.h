// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_OPERATIONS_UTILS_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_OPERATIONS_UTILS_H_

#include <vector>

#include "clay/gfx/geometry/transform_raw.h"
#include "gfx/geometry/transform_operations.h"
#include "skity/geometry/matrix.hpp"

namespace clay {

// Converts Clay input types to GFX operations and resolves relative lengths
// against the current content size.
lynx::gfx::TransformOperations ResolveTransform(
    const std::vector<TransformRaw>& transform_raw, float width, float height);

// Resolves relative lengths in existing GFX operations against the current
// content size.
lynx::gfx::TransformOperations ResolveTransform(
    const lynx::gfx::TransformOperations& operations, float width,
    float height);

// Primitive translate Z controls Clay sibling ordering. Explicit matrix3d
// operations remain visual matrices and are intentionally ignored here.
float GetTranslateZ(const lynx::gfx::TransformOperations& operations);

// Applies GFX operations and Clay's separate perspective property, then
// applies the transform origin and layer offset.
skity::Matrix ApplyTransform(const lynx::gfx::TransformOperations& operations,
                             float perspective, float origin_x, float origin_y,
                             float offset_x, float offset_y);

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_OPERATIONS_UTILS_H_
