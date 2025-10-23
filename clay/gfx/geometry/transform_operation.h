// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_GEOMETRY_TRANSFORM_OPERATION_H_
#define CLAY_GFX_GEOMETRY_TRANSFORM_OPERATION_H_

#include "clay/gfx/geometry/transform.h"

namespace clay {

struct TransformOperation {
  enum Type {
    kTypeTranslate,
    kTypeRotate,
    kTypeScale,
    kTypeSkewX,
    kTypeSkewY,
    kTypeSkew,
    kTypePerspective,
    kTypeMatrix,
    kTypeMatrix3d,
    kTypeIdentity
  };

  TransformOperation() : type(kTypeIdentity) {}

  Type type;
  Transform matrix;

  union {
    float perspective_depth;

    struct {
      float x, y;
    } skew;

    struct {
      float x, y, z;
    } scale;

    struct {
      float x, y, z;
    } translate;

    struct {
      struct {
        float x, y, z;
      } axis;

      float angle;
    } rotate;
  };

  bool IsIdentity() const;

  // Sets |matrix| based on type and the union values.
  void Bake();

  bool ApproximatelyEqual(const TransformOperation& other,
                          float tolerance) const;

  static bool BlendTransformOperations(const TransformOperation* from,
                                       const TransformOperation* to,
                                       float progress,
                                       TransformOperation* result);
};

}  // namespace clay

#endif  // CLAY_GFX_GEOMETRY_TRANSFORM_OPERATION_H_
