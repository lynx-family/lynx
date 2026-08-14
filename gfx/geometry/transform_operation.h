// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef GFX_GEOMETRY_TRANSFORM_OPERATION_H_
#define GFX_GEOMETRY_TRANSFORM_OPERATION_H_

#include <array>

#include "gfx/geometry/decomposed_transform.h"
#include "gfx/geometry/length.h"
#include "gfx/geometry/matrix44.h"
#include "gfx/gfx_export.h"

namespace lynx {
namespace gfx {

struct GFX_EXPORT TransformOperation {
  enum Type {
    kIdentity = 0,
    kTranslate = 1,
    kRotateX = 1 << 2,
    kRotateY = 1 << 3,
    kRotateZ = 1 << 4,
    kScale = 1 << 5,
    kSkew = 1 << 6,
    kMatrix = 1 << 7,
    kMatrix3d = 1 << 8,
  };

  struct Skew {
    float x{0.0f};
    float y{0.0f};
  } skew;

  struct Scale {
    float x{0.0f};
    float y{0.0f};
  } scale;

  struct Translate {
    LengthValue x;
    LengthValue y;
    LengthValue z;
  } translate;

  struct Rotate {
    float degree{0.0f};
  } rotate;

  struct Matrix {
    // CSS matrix3d() values in column-major order.
    std::array<float, 16> matrix_data = {1, 0, 0, 0,   // {{1, 0, 0, 0}
                                         0, 1, 0, 0,   //  {0, 1, 0, 0}
                                         0, 0, 1, 0,   //  {0, 0, 1, 0}
                                         0, 0, 0, 1};  //  {0, 0, 0, 1}}
  } matrix;

  Type type{kIdentity};

  bool IsIdentity() const;
  bool ApproximatelyEqual(const TransformOperation& other,
                          float tolerance) const;

  Matrix44 GetMatrix(float reference_width, float reference_height) const;

  static bool BlendTransformOperations(const TransformOperation* from,
                                       const TransformOperation* to,
                                       float progress, float reference_width,
                                       float reference_height,
                                       TransformOperation* result);
  static TransformOperation FromDecomposedTransform(
      const DecomposedTransform& decomposed_transform);
};

}  // namespace gfx
}  // namespace lynx

#endif  // GFX_GEOMETRY_TRANSFORM_OPERATION_H_
