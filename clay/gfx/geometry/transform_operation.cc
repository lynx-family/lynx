// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/transform_operation.h"

#include <limits>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_vector3d.h"
#include "clay/gfx/geometry/math_util.h"
#include "clay/gfx/geometry/transform_operations.h"
#include "clay/gfx/geometry/transform_util.h"

namespace {
const float kAngleEpsilon = 1e-4f;
}

namespace clay {

bool TransformOperation::IsIdentity() const { return matrix.IsIdentity(); }

static bool IsOperationIdentity(const TransformOperation* operation) {
  return !operation || operation->IsIdentity();
}

static bool ShareSameAxis(const TransformOperation* from,
                          const TransformOperation* to, float* axis_x,
                          float* axis_y, float* axis_z, float* angle_from) {
  if (IsOperationIdentity(from) && IsOperationIdentity(to)) {
    return false;
  }

  if (IsOperationIdentity(from) && !IsOperationIdentity(to)) {
    *axis_x = to->rotate.axis.x;
    *axis_y = to->rotate.axis.y;
    *axis_z = to->rotate.axis.z;
    *angle_from = 0;
    return true;
  }

  if (!IsOperationIdentity(from) && IsOperationIdentity(to)) {
    *axis_x = from->rotate.axis.x;
    *axis_y = from->rotate.axis.y;
    *axis_z = from->rotate.axis.z;
    *angle_from = from->rotate.angle;
    return true;
  }

  float length_2 = from->rotate.axis.x * from->rotate.axis.x +
                   from->rotate.axis.y * from->rotate.axis.y +
                   from->rotate.axis.z * from->rotate.axis.z;
  float other_length_2 = to->rotate.axis.x * to->rotate.axis.x +
                         to->rotate.axis.y * to->rotate.axis.y +
                         to->rotate.axis.z * to->rotate.axis.z;

  if (length_2 <= kAngleEpsilon || other_length_2 <= kAngleEpsilon) {
    return false;
  }

  float dot = to->rotate.axis.x * from->rotate.axis.x +
              to->rotate.axis.y * from->rotate.axis.y +
              to->rotate.axis.z * from->rotate.axis.z;
  float error = std::abs(1.f - (dot * dot) / (length_2 * other_length_2));
  bool result = error < kAngleEpsilon;
  if (result) {
    *axis_x = to->rotate.axis.x;
    *axis_y = to->rotate.axis.y;
    *axis_z = to->rotate.axis.z;
    // If the axes are pointing in opposite directions, we need to reverse
    // the angle.
    *angle_from = dot > 0 ? from->rotate.angle : -from->rotate.angle;
  }
  return result;
}

static float BlendFloats(float from, float to, float progress) {
  return from * (1 - progress) + to * progress;
}

void TransformOperation::Bake() {
  matrix.MakeIdentity();
  switch (type) {
    case TransformOperation::kTypeTranslate:
      matrix.Translate3d(translate.x, translate.y, translate.z);
      break;
    case TransformOperation::kTypeRotate:
      matrix.RotateAbout(
          FloatVector3d(rotate.axis.x, rotate.axis.y, rotate.axis.z),
          rotate.angle);
      break;
    case TransformOperation::kTypeScale:
      matrix.Scale3d(scale.x, scale.y, scale.z);
      break;
    case TransformOperation::kTypeSkewX:
    case TransformOperation::kTypeSkewY:
    case TransformOperation::kTypeSkew:
      matrix.Skew(skew.x, skew.y);
      break;
    case TransformOperation::kTypePerspective:
      matrix.ApplyPerspectiveDepth(perspective_depth);
      break;
    case TransformOperation::kTypeMatrix:
    case TransformOperation::kTypeMatrix3d:
    case TransformOperation::kTypeIdentity:
      break;
  }
}

bool TransformOperation::ApproximatelyEqual(const TransformOperation& other,
                                            float tolerance) const {
  FML_DCHECK(0 <= tolerance);
  if (type != other.type) {
    return false;
  }
  switch (type) {
    case TransformOperation::kTypeTranslate:
      return IsApproximatelyEqual(translate.x, other.translate.x, tolerance) &&
             IsApproximatelyEqual(translate.y, other.translate.y, tolerance) &&
             IsApproximatelyEqual(translate.z, other.translate.z, tolerance);
    case TransformOperation::kTypeRotate:
      return IsApproximatelyEqual(rotate.axis.x, other.rotate.axis.x,
                                  tolerance) &&
             IsApproximatelyEqual(rotate.axis.y, other.rotate.axis.y,
                                  tolerance) &&
             IsApproximatelyEqual(rotate.axis.z, other.rotate.axis.z,
                                  tolerance) &&
             IsApproximatelyEqual(rotate.angle, other.rotate.angle, tolerance);
    case TransformOperation::kTypeScale:
      return IsApproximatelyEqual(scale.x, other.scale.x, tolerance) &&
             IsApproximatelyEqual(scale.y, other.scale.y, tolerance) &&
             IsApproximatelyEqual(scale.z, other.scale.z, tolerance);
    case TransformOperation::kTypeSkewX:
    case TransformOperation::kTypeSkewY:
    case TransformOperation::kTypeSkew:
      return IsApproximatelyEqual(skew.x, other.skew.x, tolerance) &&
             IsApproximatelyEqual(skew.y, other.skew.y, tolerance);
    case TransformOperation::kTypePerspective:
      return IsApproximatelyEqual(perspective_depth, other.perspective_depth,
                                  tolerance);
    case TransformOperation::kTypeMatrix:
    case TransformOperation::kTypeMatrix3d:
      // TODO(vollick): we could expose a tolerance on Transform, but it's
      // complex since we need a different tolerance per component. Driving this
      // with a single tolerance will take some care. For now, we will check
      // exact equality where the tolerance is 0.0f, otherwise we will use the
      // unparameterized version of Transform::ApproximatelyEqual.
      if (tolerance == 0.0f) {
        return matrix == other.matrix;
      } else {
        return matrix.ApproximatelyEqual(other.matrix);
      }
    case TransformOperation::kTypeIdentity:
      return other.matrix.IsIdentity();
  }
  FML_UNREACHABLE();
  return false;
}

bool TransformOperation::BlendTransformOperations(
    const TransformOperation* from, const TransformOperation* to,
    float progress, TransformOperation* result) {
  if (IsOperationIdentity(from) && IsOperationIdentity(to)) {
    return true;
  }

  TransformOperation::Type interpolation_type =
      TransformOperation::kTypeIdentity;
  if (IsOperationIdentity(to)) {
    interpolation_type = from->type;
  } else {
    interpolation_type = to->type;
  }
  result->type = interpolation_type;

  switch (interpolation_type) {
    case TransformOperation::kTypeTranslate: {
      float from_x = IsOperationIdentity(from) ? 0 : from->translate.x;
      float from_y = IsOperationIdentity(from) ? 0 : from->translate.y;
      float from_z = IsOperationIdentity(from) ? 0 : from->translate.z;
      float to_x = IsOperationIdentity(to) ? 0 : to->translate.x;
      float to_y = IsOperationIdentity(to) ? 0 : to->translate.y;
      float to_z = IsOperationIdentity(to) ? 0 : to->translate.z;
      result->translate.x = BlendFloats(from_x, to_x, progress),
      result->translate.y = BlendFloats(from_y, to_y, progress),
      result->translate.z = BlendFloats(from_z, to_z, progress), result->Bake();
      break;
    }
    case TransformOperation::kTypeRotate: {
      float axis_x = 0;
      float axis_y = 0;
      float axis_z = 1;
      float from_angle = 0;
      float to_angle = IsOperationIdentity(to) ? 0 : to->rotate.angle;
      if (ShareSameAxis(from, to, &axis_x, &axis_y, &axis_z, &from_angle)) {
        result->rotate.axis.x = axis_x;
        result->rotate.axis.y = axis_y;
        result->rotate.axis.z = axis_z;
        result->rotate.angle = BlendFloats(from_angle, to_angle, progress);
        result->Bake();
      } else {
        if (!IsOperationIdentity(to)) {
          result->matrix = to->matrix;
        }
        Transform from_matrix;
        if (!IsOperationIdentity(from)) {
          from_matrix = from->matrix;
        }
        if (!result->matrix.Blend(from_matrix, progress)) {
          return false;
        }
      }
      break;
    }
    case TransformOperation::kTypeScale: {
      float from_x = IsOperationIdentity(from) ? 1 : from->scale.x;
      float from_y = IsOperationIdentity(from) ? 1 : from->scale.y;
      float from_z = IsOperationIdentity(from) ? 1 : from->scale.z;
      float to_x = IsOperationIdentity(to) ? 1 : to->scale.x;
      float to_y = IsOperationIdentity(to) ? 1 : to->scale.y;
      float to_z = IsOperationIdentity(to) ? 1 : to->scale.z;
      result->scale.x = BlendFloats(from_x, to_x, progress);
      result->scale.y = BlendFloats(from_y, to_y, progress);
      result->scale.z = BlendFloats(from_z, to_z, progress);
      result->Bake();
      break;
    }
    case TransformOperation::kTypeSkewX:
    case TransformOperation::kTypeSkewY:
    case TransformOperation::kTypeSkew: {
      float from_x = IsOperationIdentity(from) ? 0 : from->skew.x;
      float from_y = IsOperationIdentity(from) ? 0 : from->skew.y;
      float to_x = IsOperationIdentity(to) ? 0 : to->skew.x;
      float to_y = IsOperationIdentity(to) ? 0 : to->skew.y;
      result->skew.x = BlendFloats(from_x, to_x, progress);
      result->skew.y = BlendFloats(from_y, to_y, progress);
      result->Bake();
      break;
    }
    case TransformOperation::kTypePerspective: {
      float from_perspective_depth = IsOperationIdentity(from)
                                         ? std::numeric_limits<float>::max()
                                         : from->perspective_depth;
      float to_perspective_depth = IsOperationIdentity(to)
                                       ? std::numeric_limits<float>::max()
                                       : to->perspective_depth;
      if (from_perspective_depth == 0.f || to_perspective_depth == 0.f) {
        return false;
      }

      float blended_perspective_depth = BlendFloats(
          1.f / from_perspective_depth, 1.f / to_perspective_depth, progress);

      if (blended_perspective_depth == 0.f) {
        return false;
      }

      result->perspective_depth = 1.f / blended_perspective_depth;
      result->Bake();
      break;
    }
    case TransformOperation::kTypeMatrix:
    case TransformOperation::kTypeMatrix3d: {
      if (!IsOperationIdentity(to)) {
        result->matrix = to->matrix;
      }
      Transform from_matrix;
      if (!IsOperationIdentity(from)) {
        from_matrix = from->matrix;
      }
      if (!result->matrix.Blend(from_matrix, progress)) {
        return false;
      }
      break;
    }
    case TransformOperation::kTypeIdentity:
      // Do nothing.
      break;
  }

  return true;
}

}  // namespace clay
