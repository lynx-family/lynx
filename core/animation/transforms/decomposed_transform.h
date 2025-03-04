// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_ANIMATION_TRANSFORMS_DECOMPOSED_TRANSFORM_H_
#define CORE_ANIMATION_TRANSFORMS_DECOMPOSED_TRANSFORM_H_

#include "core/animation/transforms/matrix44.h"
#include "core/animation/transforms/quaternion.h"

namespace lynx {
namespace transforms {
// Contains the components of a factored transform. These components may be
// blended and recomposed.
struct DecomposedTransform {
  DecomposedTransform();

  float translate[3];
  float scale[3];
  float skew[3];
  float perspective[4];
  Quaternion quaternion;
};

// Decomposes this transform into its translation, scale, skew, perspective,
// and rotation components following the routines detailed in this spec:
// http://www.w3.org/TR/css3-3d-transforms/.
bool DecomposeTransform(DecomposedTransform* decomposed_transform,
                        const Matrix44& transform);

DecomposedTransform BlendDecomposedTransforms(const DecomposedTransform& to,
                                              const DecomposedTransform& from,
                                              double progress);

}  // namespace transforms
}  // namespace lynx

#endif  // CORE_ANIMATION_TRANSFORMS_DECOMPOSED_TRANSFORM_H_
