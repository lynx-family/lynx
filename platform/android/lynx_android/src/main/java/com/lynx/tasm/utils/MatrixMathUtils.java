/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import com.lynx.tasm.base.LLog;
/**
 * A utils class for matrix math operation.
 */
public class MatrixMathUtils {
  private static final double EPSILON = .00001d;

  public static class MatrixDecompositionContext {
    public double[] perspective = new double[4];
    public double[] scale = new double[3];
    public double[] skew = new double[3];
    public double[] translation = new double[3];
    public double[] rotationDegrees = new double[3];

    private static void resetArray(double[] arr) {
      for (int i = 0; i < arr.length; i++) {
        arr[i] = 0;
      }
    }

    public void reset() {
      MatrixDecompositionContext.resetArray(perspective);
      MatrixDecompositionContext.resetArray(scale);
      MatrixDecompositionContext.resetArray(skew);
      MatrixDecompositionContext.resetArray(translation);
      MatrixDecompositionContext.resetArray(rotationDegrees);
    }

    public float getTranslationX() {
      return (float) translation[0];
    }
    public float getTranslationY() {
      return (float) translation[1];
    }
    public float getTranslationZ() {
      return (float) translation[2];
    }
    public float getRotationX() {
      return (float) rotationDegrees[0];
    }
    public float getRotationY() {
      return (float) rotationDegrees[1];
    }
    public float getRotation() {
      return (float) rotationDegrees[2];
    }
    public float getScaleX() {
      return (float) scale[0];
    }
    public float getScaleY() {
      return (float) scale[1];
    }
    public float getSkewX() {
      return (float) skew[0];
    }
    public float getSkewY() {
      return (float) skew[1];
    }
  }

  private static boolean isZero(double d) {
    if (Double.isNaN(d)) {
      return false;
    }
    return Math.abs(d) < EPSILON;
  }

  private static boolean isMatrix3D(double[] matrix) {
    if (matrix.length < 16) {
      return false;
    }
    return true;
  }

  private static double determinant(double[] matrix) {
    double m00 = matrix[0], m01 = matrix[1], m02 = matrix[2], m03 = matrix[3], m10 = matrix[4],
           m11 = matrix[5], m12 = matrix[6], m13 = matrix[7], m20 = matrix[8], m21 = matrix[9],
           m22 = matrix[10], m23 = matrix[11], m30 = matrix[12], m31 = matrix[13], m32 = matrix[14],
           m33 = matrix[15];
    return (m03 * m12 * m21 * m30 - m02 * m13 * m21 * m30 - m03 * m11 * m22 * m30
        + m01 * m13 * m22 * m30 + m02 * m11 * m23 * m30 - m01 * m12 * m23 * m30
        - m03 * m12 * m20 * m31 + m02 * m13 * m20 * m31 + m03 * m10 * m22 * m31
        - m00 * m13 * m22 * m31 - m02 * m10 * m23 * m31 + m00 * m12 * m23 * m31
        + m03 * m11 * m20 * m32 - m01 * m13 * m20 * m32 - m03 * m10 * m21 * m32
        + m00 * m13 * m21 * m32 + m01 * m10 * m23 * m32 - m00 * m11 * m23 * m32
        - m02 * m11 * m20 * m33 + m01 * m12 * m20 * m33 + m02 * m10 * m21 * m33
        - m00 * m12 * m21 * m33 - m01 * m10 * m22 * m33 + m00 * m11 * m22 * m33);
  }

  /**
   * Inverse of a matrix. Multiplying by the inverse is used in matrix math instead of division.
   *
   * <p>Formula from:
   * http://www.euclideanspace.com/maths/algebra/matrix/functions/inverse/fourD/index.htm
   */
  private static double[] inverse(double[] matrix) {
    double det = determinant(matrix);
    if (isZero(det)) {
      return matrix;
    }
    double m00 = matrix[0], m01 = matrix[1], m02 = matrix[2], m03 = matrix[3], m10 = matrix[4],
           m11 = matrix[5], m12 = matrix[6], m13 = matrix[7], m20 = matrix[8], m21 = matrix[9],
           m22 = matrix[10], m23 = matrix[11], m30 = matrix[12], m31 = matrix[13], m32 = matrix[14],
           m33 = matrix[15];
    return new double[] {(m12 * m23 * m31 - m13 * m22 * m31 + m13 * m21 * m32 - m11 * m23 * m32
                             - m12 * m21 * m33 + m11 * m22 * m33)
            / det,
        (m03 * m22 * m31 - m02 * m23 * m31 - m03 * m21 * m32 + m01 * m23 * m32 + m02 * m21 * m33
            - m01 * m22 * m33)
            / det,
        (m02 * m13 * m31 - m03 * m12 * m31 + m03 * m11 * m32 - m01 * m13 * m32 - m02 * m11 * m33
            + m01 * m12 * m33)
            / det,
        (m03 * m12 * m21 - m02 * m13 * m21 - m03 * m11 * m22 + m01 * m13 * m22 + m02 * m11 * m23
            - m01 * m12 * m23)
            / det,
        (m13 * m22 * m30 - m12 * m23 * m30 - m13 * m20 * m32 + m10 * m23 * m32 + m12 * m20 * m33
            - m10 * m22 * m33)
            / det,
        (m02 * m23 * m30 - m03 * m22 * m30 + m03 * m20 * m32 - m00 * m23 * m32 - m02 * m20 * m33
            + m00 * m22 * m33)
            / det,
        (m03 * m12 * m30 - m02 * m13 * m30 - m03 * m10 * m32 + m00 * m13 * m32 + m02 * m10 * m33
            - m00 * m12 * m33)
            / det,
        (m02 * m13 * m20 - m03 * m12 * m20 + m03 * m10 * m22 - m00 * m13 * m22 - m02 * m10 * m23
            + m00 * m12 * m23)
            / det,
        (m11 * m23 * m30 - m13 * m21 * m30 + m13 * m20 * m31 - m10 * m23 * m31 - m11 * m20 * m33
            + m10 * m21 * m33)
            / det,
        (m03 * m21 * m30 - m01 * m23 * m30 - m03 * m20 * m31 + m00 * m23 * m31 + m01 * m20 * m33
            - m00 * m21 * m33)
            / det,
        (m01 * m13 * m30 - m03 * m11 * m30 + m03 * m10 * m31 - m00 * m13 * m31 - m01 * m10 * m33
            + m00 * m11 * m33)
            / det,
        (m03 * m11 * m20 - m01 * m13 * m20 - m03 * m10 * m21 + m00 * m13 * m21 + m01 * m10 * m23
            - m00 * m11 * m23)
            / det,
        (m12 * m21 * m30 - m11 * m22 * m30 - m12 * m20 * m31 + m10 * m22 * m31 + m11 * m20 * m32
            - m10 * m21 * m32)
            / det,
        (m01 * m22 * m30 - m02 * m21 * m30 + m02 * m20 * m31 - m00 * m22 * m31 - m01 * m20 * m32
            + m00 * m21 * m32)
            / det,
        (m02 * m11 * m30 - m01 * m12 * m30 - m02 * m10 * m31 + m00 * m12 * m31 + m01 * m10 * m32
            - m00 * m11 * m32)
            / det,
        (m01 * m12 * m20 - m02 * m11 * m20 + m02 * m10 * m21 - m00 * m12 * m21 - m01 * m10 * m22
            + m00 * m11 * m22)
            / det};
  }

  /** Turns columns into rows and rows into columns. */
  private static double[] transpose(double[] m) {
    return new double[] {m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15]};
  }

  /** Based on: http://tog.acm.org/resources/GraphicsGems/gemsii/unmatrix.c */
  private static void multiplyVectorByMatrix(double[] v, double[] m, double[] result) {
    double vx = v[0], vy = v[1], vz = v[2], vw = v[3];
    result[0] = vx * m[0] + vy * m[4] + vz * m[8] + vw * m[12];
    result[1] = vx * m[1] + vy * m[5] + vz * m[9] + vw * m[13];
    result[2] = vx * m[2] + vy * m[6] + vz * m[10] + vw * m[14];
    result[3] = vx * m[3] + vy * m[7] + vz * m[11] + vw * m[15];
  }

  /** From: https://code.google.com/p/webgl-mjs/source/browse/mjs.js */
  private static double v3Length(double[] a) {
    return Math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
  }

  /** Based on: https://code.google.com/p/webgl-mjs/source/browse/mjs.js */
  private static double[] v3Normalize(double[] vector, double norm) {
    double im = 1 / (isZero(norm) ? v3Length(vector) : norm);
    return new double[] {vector[0] * im, vector[1] * im, vector[2] * im};
  }

  /**
   * The dot product of a and b, two 3-element vectors. From:
   * https://code.google.com/p/webgl-mjs/source/browse/mjs.js
   */
  private static double v3Dot(double[] a, double[] b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  }

  /**
   * From:
   * http://www.opensource.apple.com/source/WebCore/WebCore-514/platform/graphics/transforms/TransformationMatrix.cpp
   */
  private static double[] v3Combine(double[] a, double[] b, double aScale, double bScale) {
    return new double[] {aScale * a[0] + bScale * b[0], aScale * a[1] + bScale * b[1],
        aScale * a[2] + bScale * b[2]};
  }

  /**
   * From:
   * http://www.opensource.apple.com/source/WebCore/WebCore-514/platform/graphics/transforms/TransformationMatrix.cpp
   */
  private static double[] v3Cross(double[] a, double[] b) {
    return new double[] {
        a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
  }

  private static double roundTo3Places(double n) {
    return Math.round(n * 1000d) * 0.001;
  }

  /** @param transformMatrix 16-element array of numbers representing 4x4 transform matrix */
  public static void decomposeMatrix(double[] transformMatrix, MatrixDecompositionContext ctx) {
    if (!isMatrix3D(transformMatrix)) {
      LLog.e("lynx", "Decompose transform matrix error! transformMatrix's length less than 16!");
      return;
    }

    // output values
    final double[] perspective = ctx.perspective;
    final double[] scale = ctx.scale;
    final double[] skew = ctx.skew;
    final double[] translation = ctx.translation;
    final double[] rotationDegrees = ctx.rotationDegrees;

    // create normalized, 2d array matrix
    // and normalized 1d array perspectiveMatrix with redefined 4th column
    if (isZero(transformMatrix[15])) {
      return;
    }
    double[][] matrix = new double[4][4];
    double[] perspectiveMatrix = new double[16];
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        double value = transformMatrix[(i * 4) + j] / transformMatrix[15];
        matrix[i][j] = value;
        perspectiveMatrix[(i * 4) + j] = j == 3 ? 0 : value;
      }
    }
    perspectiveMatrix[15] = 1;

    // test for singularity of upper 3x3 part of the perspective matrix
    if (isZero(determinant(perspectiveMatrix))) {
      return;
    }

    // isolate perspective
    if (!isZero(matrix[0][3]) || !isZero(matrix[1][3]) || !isZero(matrix[2][3])) {
      // rightHandSide is the right hand side of the equation.
      // rightHandSide is a vector, or point in 3d space relative to the origin.
      double[] rightHandSide = {matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]};

      // Solve the equation by inverting perspectiveMatrix and multiplying
      // rightHandSide by the inverse.
      double[] inversePerspectiveMatrix = inverse(perspectiveMatrix);
      double[] transposedInversePerspectiveMatrix = transpose(inversePerspectiveMatrix);
      multiplyVectorByMatrix(rightHandSide, transposedInversePerspectiveMatrix, perspective);
    } else {
      // no perspective
      perspective[0] = perspective[1] = perspective[2] = 0d;
      perspective[3] = 1d;
    }

    // translation is simple
    for (int i = 0; i < 3; i++) {
      translation[i] = matrix[3][i];
    }

    // Now get scale and shear.
    // 'row' is a 3 element array of 3 component vectors
    double[][] row = new double[3][3];
    for (int i = 0; i < 3; i++) {
      row[i][0] = matrix[i][0];
      row[i][1] = matrix[i][1];
      row[i][2] = matrix[i][2];
    }

    // Compute X scale factor and normalize first row.
    scale[0] = v3Length(row[0]);
    row[0] = v3Normalize(row[0], scale[0]);

    // Compute XY shear factor and make 2nd row orthogonal to 1st.
    skew[0] = v3Dot(row[0], row[1]);
    row[1] = v3Combine(row[1], row[0], 1.0, -skew[0]);

    // Now, compute Y scale and normalize 2nd row.
    scale[1] = v3Length(row[1]);
    row[1] = v3Normalize(row[1], scale[1]);
    skew[0] /= scale[1];

    // Compute XZ and YZ shears, orthogonalize 3rd row
    skew[1] = v3Dot(row[0], row[2]);
    row[2] = v3Combine(row[2], row[0], 1.0, -skew[1]);
    skew[2] = v3Dot(row[1], row[2]);
    row[2] = v3Combine(row[2], row[1], 1.0, -skew[2]);

    // Next, get Z scale and normalize 3rd row.
    scale[2] = v3Length(row[2]);
    row[2] = v3Normalize(row[2], scale[2]);
    skew[1] /= scale[2];
    skew[2] /= scale[2];

    // At this point, the matrix (in rows) is orthonormal.
    // Check for a coordinate system flip.  If the determinant
    // is -1, then negate the matrix and the scaling factors.
    double[] pdum3 = v3Cross(row[1], row[2]);
    if (v3Dot(row[0], pdum3) < 0) {
      for (int i = 0; i < 3; i++) {
        scale[i] *= -1;
        row[i][0] *= -1;
        row[i][1] *= -1;
        row[i][2] *= -1;
      }
    }

    // Now, get the rotations out
    // Based on: http://nghiaho.com/?page_id=846
    double conv = 180 / Math.PI;
    rotationDegrees[0] = roundTo3Places(-Math.atan2(row[2][1], row[2][2]) * conv);
    rotationDegrees[1] = roundTo3Places(
        -Math.atan2(-row[2][0], Math.sqrt(row[2][1] * row[2][1] + row[2][2] * row[2][2])) * conv);
    rotationDegrees[2] = roundTo3Places(-Math.atan2(row[1][0], row[0][0]) * conv);
  }
  public static double degreesToRadians(double degrees) {
    return degrees * Math.PI / 180;
  }
}
