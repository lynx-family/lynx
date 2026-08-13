// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_VALUE_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_VALUE_H_

#include <vector>

#include "clay/gfx/geometry/transform_raw.h"
#include "gfx/geometry/matrix44.h"
#include "gfx/geometry/transform_operations.h"
#include "skity/geometry/matrix.hpp"

namespace clay {

// Primitive translate-z values control sibling ordering instead of the visual
// matrix, so the two results are carried separately after resolving Clay input
// types. Explicit matrix3d operations remain complete 4x4 visual transforms.
struct TransformValue {
  lynx::gfx::TransformOperations visual_operations;
  float stacking_z{0.0f};

  bool ApproximatelyEqual(const TransformValue& other, float tolerance) const;
  TransformValue Blend(const TransformValue& from, float progress,
                       float reference_width = 0.0f,
                       float reference_height = 0.0f) const;
};

TransformValue ResolveTransform(const std::vector<TransformRaw>& transform_raw,
                                float width, float height);

// Resolves GFX lengths against the current content size and splits primitive
// translate-z. Matrix operations remain untouched because their Z components
// have never participated in Clay sibling ordering.
TransformValue ResolveTransform(
    const lynx::gfx::TransformOperations& operations, float width,
    float height);

// Converts a GFX matrix to Clay's platform matrix without changing its 4x4
// contents. Clay ignores primitive translate-z before this boundary, while an
// explicit matrix3d remains a visual matrix.
skity::Matrix ToSkityMatrix(const lynx::gfx::Matrix44& matrix);

// Applies visual operations and Clay's separate perspective property, then
// applies the transform origin and layer offset.
skity::Matrix ApplyTransform(
    const lynx::gfx::TransformOperations& visual_operations, float perspective,
    float origin_x, float origin_y, float offset_x, float offset_y);

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_VALUE_H_
