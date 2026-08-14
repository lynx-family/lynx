// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_GEOMETRY_DECOMPOSED_TRANSFORM_H_
#define GFX_GEOMETRY_DECOMPOSED_TRANSFORM_H_

#include "gfx/geometry/matrix44.h"
#include "gfx/geometry/quaternion.h"
#include "gfx/gfx_export.h"

namespace lynx {
namespace gfx {
// Contains the components of a factored transform. These components may be
// blended and recomposed.
struct GFX_EXPORT DecomposedTransform {
  DecomposedTransform();

  float translate[3];
  float scale[3];
  float skew[3];
  float perspective[4];
  Quaternion quaternion;
};

// Decomposes this transform into its translation, scale, skew, perspective,
// and rotation components following the routines detailed in this spec:
// https://www.w3.org/TR/css-transforms-2/.
GFX_EXPORT bool DecomposeTransform(DecomposedTransform* decomposed_transform,
                                   const Matrix44& transform);

GFX_EXPORT DecomposedTransform
BlendDecomposedTransforms(const DecomposedTransform& to,
                          const DecomposedTransform& from, double progress);

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_GEOMETRY_DECOMPOSED_TRANSFORM_H_
