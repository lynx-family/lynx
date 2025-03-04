// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/animation/transforms/decomposed_transform.h"

#include <cmath>

namespace lynx {
namespace transforms {

namespace {

float Length3(float v[3]) {
  double vd[3] = {v[0], v[1], v[2]};
  return std::sqrt(vd[0] * vd[0] + vd[1] * vd[1] + vd[2] * vd[2]);
}

template <int n>
float Dot(const float* a, const float* b) {
  double total = 0.0;
  for (int i = 0; i < n; ++i) total += a[i] * b[i];
  return total;
}

template <int n>
void Combine(float* out, const float* a, const float* b, double scale_a,
             double scale_b) {
  for (int i = 0; i < n; ++i) out[i] = a[i] * scale_a + b[i] * scale_b;
}

void Cross3(float out[3], float a[3], float b[3]) {
  float x = a[1] * b[2] - a[2] * b[1];
  float y = a[2] * b[0] - a[0] * b[2];
  float z = a[0] * b[1] - a[1] * b[0];
  out[0] = x;
  out[1] = y;
  out[2] = z;
}

// Returns false if the matrix cannot be normalized.
bool Normalize(Matrix44& m) {
  if (m.rc(3, 3) == 0.0)
    // Cannot normalize.
    return false;

  float scale = 1.0f / m.rc(3, 3);
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) m.setRC(i, j, m.rc(i, j) * scale);

  return true;
}

bool Is2dTransform(const Matrix44& matrix) {
  if (matrix.HasPerspective()) return false;

  return matrix.rc(2, 0) == 0 && matrix.rc(2, 1) == 0 && matrix.rc(0, 2) == 0 &&
         matrix.rc(1, 2) == 0 && matrix.rc(2, 2) == 1 && matrix.rc(3, 2) == 0 &&
         matrix.rc(2, 3) == 0;
}

bool Decompose2DTransform(DecomposedTransform* decomposed_transform,
                          const Matrix44& matrix) {
  if (!Is2dTransform(matrix)) {
    return false;
  }

  double m11 = matrix.rc(0, 0);
  double m21 = matrix.rc(0, 1);
  double m12 = matrix.rc(1, 0);
  double m22 = matrix.rc(1, 1);

  double determinant = m11 * m22 - m12 * m21;
  // Test for matrix being singular.
  if (determinant == 0) {
    return false;
  }

  // Translation transform.
  // [m11 m21 0 m41]    [1 0 0 Tx] [m11 m21 0 0]
  // [m12 m22 0 m42]  = [0 1 0 Ty] [m12 m22 0 0]
  // [ 0   0  1  0 ]    [0 0 1 0 ] [ 0   0  1 0]
  // [ 0   0  0  1 ]    [0 0 0 1 ] [ 0   0  0 1]
  decomposed_transform->translate[0] = matrix.rc(0, 3);
  decomposed_transform->translate[1] = matrix.rc(1, 3);

  // For the remainder of the decomposition process, we can focus on the upper
  // 2x2 sub matrix
  // [m11 m21] = [cos(R) -sin(R)] [1 K] [Sx 0 ]
  // [m12 m22]   [sin(R)  cos(R)] [0 1] [0  Sy]
  //           = [Sx*cos(R) Sy*(K*cos(R) - sin(R))]
  //             [Sx*sin(R) Sy*(K*sin(R) + cos(R))]

  // Determine sign of the x and y scale.
  if (determinant < 0) {
    // If the determinant is negative, we need to flip either the x or y scale.
    // Flipping both is equivalent to rotating by 180 degrees.
    if (m11 < m22) {
      decomposed_transform->scale[0] *= -1;
    } else {
      decomposed_transform->scale[1] *= -1;
    }
  }

  // X Scale.
  // m11^2 + m12^2 = Sx^2*(cos^2(R) + sin^2(R)) = Sx^2.
  // Sx = +/-sqrt(m11^2 + m22^2)
  decomposed_transform->scale[0] *= sqrt(m11 * m11 + m12 * m12);
  m11 /= decomposed_transform->scale[0];
  m12 /= decomposed_transform->scale[0];

  // Post normalization, the sub matrix is now of the form:
  // [m11 m21] = [cos(R)  Sy*(K*cos(R) - sin(R))]
  // [m12 m22]   [sin(R)  Sy*(K*sin(R) + cos(R))]

  // XY Shear.
  // m11 * m21 + m12 * m22 = Sy*K*cos^2(R) - Sy*sin(R)*cos(R) +
  //                         Sy*K*sin^2(R) + Sy*cos(R)*sin(R)
  //                       = Sy*K
  double scaledShear = m11 * m21 + m12 * m22;
  m21 -= m11 * scaledShear;
  m22 -= m12 * scaledShear;

  // Post normalization, the sub matrix is now of the form:
  // [m11 m21] = [cos(R)  -Sy*sin(R)]
  // [m12 m22]   [sin(R)   Sy*cos(R)]

  // Y Scale.
  // Similar process to determining x-scale.
  decomposed_transform->scale[1] *= sqrt(m21 * m21 + m22 * m22);
  m21 /= decomposed_transform->scale[1];
  m22 /= decomposed_transform->scale[1];
  decomposed_transform->skew[0] = scaledShear / decomposed_transform->scale[1];

  // Rotation transform.
  // [1-2(yy+zz)  2(xy-zw)    2(xz+yw) ]   [cos(R) -sin(R)  0]
  // [2(xy+zw)   1-2(xx+zz)   2(yz-xw) ] = [sin(R)  cos(R)  0]
  // [2(xz-yw)    2*(yz+xw)  1-2(xx+yy)]   [  0       0     1]
  // Comparing terms, we can conclude that x = y = 0.
  // [1-2zz   -2zw  0]   [cos(R) -sin(R)  0]
  // [ 2zw   1-2zz  0] = [sin(R)  cos(R)  0]
  // [  0     0     1]   [  0       0     1]
  // cos(R) = 1 - 2*z^2
  // From the double angle formula: cos(2a) = 1 - 2 sin(a)^2
  // cos(R) = 1 - 2*sin(R/2)^2 = 1 - 2*z^2 ==> z = sin(R/2)
  // sin(R) = 2*z*w
  // But sin(2a) = 2 sin(a) cos(a)
  // sin(R) = 2 sin(R/2) cos(R/2) = 2*z*w ==> w = cos(R/2)
  double angle = atan2(m12, m11);
  decomposed_transform->quaternion.set_x(0);
  decomposed_transform->quaternion.set_y(0);
  decomposed_transform->quaternion.set_z(sin(0.5 * angle));
  decomposed_transform->quaternion.set_w(cos(0.5 * angle));

  return true;
}

}  // namespace

DecomposedTransform::DecomposedTransform() {
  translate[0] = translate[1] = translate[2] = 0.0;
  scale[0] = scale[1] = scale[2] = 1.0;
  skew[0] = skew[1] = skew[2] = 0.0;
  perspective[0] = perspective[1] = perspective[2] = 0.0;
  perspective[3] = 1.0;
}

DecomposedTransform BlendDecomposedTransforms(const DecomposedTransform& to,
                                              const DecomposedTransform& from,
                                              double progress) {
  DecomposedTransform out;
  double scale_a = progress;
  double scale_b = 1.0 - progress;
  Combine<3>(out.translate, to.translate, from.translate, scale_a, scale_b);
  Combine<3>(out.scale, to.scale, from.scale, scale_a, scale_b);
  Combine<3>(out.skew, to.skew, from.skew, scale_a, scale_b);
  Combine<4>(out.perspective, to.perspective, from.perspective, scale_a,
             scale_b);
  out.quaternion = from.quaternion.Slerp(to.quaternion, progress);
  return out;
}

// Taken from http://www.w3.org/TR/css3-transforms/.
// TODO(crbug/937296): This implementation is virtually identical to the
// implementation in blink::TransformationMatrix with the main difference being
// the representation of the underlying matrix. These implementations should be
// consolidated.
bool DecomposeTransform(DecomposedTransform* decomposed_transform,
                        const Matrix44& transform) {
  if (!decomposed_transform) return false;

  if (Decompose2DTransform(decomposed_transform, transform)) return true;

  // We'll operate on a copy of the transform.
  Matrix44 matrix = transform;

  // If we cannot normalize the matrix, then bail early as we cannot decompose.
  if (!Normalize(matrix)) return false;

  Matrix44 perspective_matrix = matrix;

  for (int i = 0; i < 3; ++i) perspective_matrix.setRC(3, i, 0.0);

  perspective_matrix.setRC(3, 3, 1.0);

  // If the perspective matrix is not invertible, we are also unable to
  // decompose, so we'll bail early. Constant taken from Matrix44::invert.
  if (std::abs(perspective_matrix.determinant()) < 1e-8) return false;

  if (matrix.HasPerspective()) {
    // Not reachable in our code.
    DCHECK(false);
  } else {
    // No perspective.
    for (int i = 0; i < 3; ++i) decomposed_transform->perspective[i] = 0.0;
    decomposed_transform->perspective[3] = 1.0;
  }

  for (int i = 0; i < 3; i++)
    decomposed_transform->translate[i] = matrix.rc(i, 3);

  // Copy of matrix is stored in column major order to facilitate column-level
  // operations.
  float column[3][3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; ++j) column[i][j] = matrix.rc(j, i);

  // Compute X scale factor and normalize first column.
  decomposed_transform->scale[0] = Length3(column[0]);
  if (decomposed_transform->scale[0] != 0.0) {
    column[0][0] /= decomposed_transform->scale[0];
    column[0][1] /= decomposed_transform->scale[0];
    column[0][2] /= decomposed_transform->scale[0];
  }

  // Compute XY shear factor and make 2nd column orthogonal to 1st.
  decomposed_transform->skew[0] = Dot<3>(column[0], column[1]);
  Combine<3>(column[1], column[1], column[0], 1.0,
             -decomposed_transform->skew[0]);

  // Now, compute Y scale and normalize 2nd column.
  decomposed_transform->scale[1] = Length3(column[1]);
  if (decomposed_transform->scale[1] != 0.0) {
    column[1][0] /= decomposed_transform->scale[1];
    column[1][1] /= decomposed_transform->scale[1];
    column[1][2] /= decomposed_transform->scale[1];
  }

  decomposed_transform->skew[0] /= decomposed_transform->scale[1];

  // Compute XZ and YZ shears, orthogonalize the 3rd column.
  decomposed_transform->skew[1] = Dot<3>(column[0], column[2]);
  Combine<3>(column[2], column[2], column[0], 1.0,
             -decomposed_transform->skew[1]);
  decomposed_transform->skew[2] = Dot<3>(column[1], column[2]);
  Combine<3>(column[2], column[2], column[1], 1.0,
             -decomposed_transform->skew[2]);

  // Next, get Z scale and normalize the 3rd column.
  decomposed_transform->scale[2] = Length3(column[2]);
  if (decomposed_transform->scale[2] != 0.0) {
    column[2][0] /= decomposed_transform->scale[2];
    column[2][1] /= decomposed_transform->scale[2];
    column[2][2] /= decomposed_transform->scale[2];
  }

  decomposed_transform->skew[1] /= decomposed_transform->scale[2];
  decomposed_transform->skew[2] /= decomposed_transform->scale[2];

  // At this point, the matrix is orthonormal.
  // Check for a coordinate system flip.  If the determinant
  // is -1, then negate the matrix and the scaling factors.
  // TODO(kevers): This is inconsistent from the 2D specification, in which
  // only 1 axis is flipped when the determinant is negative. Verify if it is
  // correct to flip all of the scales and matrix elements, as this introduces
  // rotation for the simple case of a single axis scale inversion.
  float pdum3[3];
  Cross3(pdum3, column[1], column[2]);
  if (Dot<3>(column[0], pdum3) < 0) {
    for (int i = 0; i < 3; i++) {
      decomposed_transform->scale[i] *= -1.0;
      for (int j = 0; j < 3; ++j) column[i][j] *= -1.0;
    }
  }

  // See https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion.
  // Note: deviating from spec (http://www.w3.org/TR/css3-transforms/)
  // which has a degenerate case of zero off-diagonal elements in the
  // orthonormal matrix, which leads to errors in determining the sign
  // of the quaternions.
  double q_xx = column[0][0];
  double q_xy = column[1][0];
  double q_xz = column[2][0];
  double q_yx = column[0][1];
  double q_yy = column[1][1];
  double q_yz = column[2][1];
  double q_zx = column[0][2];
  double q_zy = column[1][2];
  double q_zz = column[2][2];

  double r, s, t, x, y, z, w;
  t = q_xx + q_yy + q_zz;
  if (t > 0) {
    r = std::sqrt(1.0 + t);
    s = 0.5 / r;
    w = 0.5 * r;
    x = (q_zy - q_yz) * s;
    y = (q_xz - q_zx) * s;
    z = (q_yx - q_xy) * s;
  } else if (q_xx > q_yy && q_xx > q_zz) {
    r = std::sqrt(1.0 + q_xx - q_yy - q_zz);
    s = 0.5 / r;
    x = 0.5 * r;
    y = (q_xy + q_yx) * s;
    z = (q_xz + q_zx) * s;
    w = (q_zy - q_yz) * s;
  } else if (q_yy > q_zz) {
    r = std::sqrt(1.0 - q_xx + q_yy - q_zz);
    s = 0.5 / r;
    x = (q_xy + q_yx) * s;
    y = 0.5 * r;
    z = (q_yz + q_zy) * s;
    w = (q_xz - q_zx) * s;
  } else {
    r = std::sqrt(1.0 - q_xx - q_yy + q_zz);
    s = 0.5 / r;
    x = (q_xz + q_zx) * s;
    y = (q_yz + q_zy) * s;
    z = 0.5 * r;
    w = (q_yx - q_xy) * s;
  }

  decomposed_transform->quaternion.set_x(x);
  decomposed_transform->quaternion.set_y(y);
  decomposed_transform->quaternion.set_z(z);
  decomposed_transform->quaternion.set_w(w);

  return true;
}

}  // namespace transforms
}  // namespace lynx
