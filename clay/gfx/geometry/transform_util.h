// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_UTIL_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_UTIL_H_

#include <string>

#include "clay/gfx/geometry/point.h"
#include "clay/gfx/geometry/quaternion.h"
#include "clay/gfx/geometry/transform.h"

namespace clay {

// Returns a scale transform at |anchor| point.
Transform GetScaleTransform(const Point& anchor, float scale);

// Contains the components of a factored transform. These components may be
// blended and recomposed.
struct DecomposedTransform {
  // The default constructor initializes the components in such a way that
  // if used with Compose below, will produce the identity transform.
  DecomposedTransform();

  float translate[3];
  float scale[3];
  float skew[3];
  float perspective[4];
  Quaternion quaternion;

  std::string ToString() const;

  // Copy and assign are allowed.
};

// Interpolates the decomposed components |to| with |from| using the
// routines described in http://www.w3.org/TR/css3-3d-transform/.
// |progress| is in the range [0, 1]. If 0 we will return |from|, if 1, we will
// return |to|.
DecomposedTransform BlendDecomposedTransforms(const DecomposedTransform& to,
                                              const DecomposedTransform& from,
                                              double progress);

// Decomposes this transform into its translation, scale, skew, perspective,
// and rotation components following the routines detailed in this spec:
// http://www.w3.org/TR/css3-3d-transforms/.
bool DecomposeTransform(DecomposedTransform* out, const Transform& transform);

// Composes a transform from the given translation, scale, skew, perspective,
// and rotation components following the routines detailed in this spec:
// http://www.w3.org/TR/css3-3d-transforms/.
Transform ComposeTransform(const DecomposedTransform& decomp);

// Calculates a transform with a transformed origin. The resulting transform is
// created by composing P * T * P^-1 where P is a constant transform to the new
// origin.
Transform TransformAboutPivot(const Point& pivot, const Transform& transform);

// Generates projection matrix and returns it as a Transform.
Transform OrthoProjectionMatrix(float left, float right, float bottom,
                                float top);

// Generates window matrix and returns it as a Transform.
Transform WindowMatrix(int x, int y, int width, int height);

FloatVector2d ComputeTransform2dScaleComponents(const Transform& transform,
                                                float fallback_value);

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_UTIL_H_
