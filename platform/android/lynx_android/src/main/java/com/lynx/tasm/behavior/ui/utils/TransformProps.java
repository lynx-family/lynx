// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_MATRIX;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_MATRIX_3d;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_ROTATE_Z;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SCALE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SCALE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SCALE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SKEW;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SKEW_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_SKEW_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_3d;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_X;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Y;
import static com.lynx.tasm.behavior.StyleConstants.TRANSFORM_TRANSLATE_Z;

import android.graphics.Matrix;
import android.renderscript.Matrix4f;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.utils.FloatUtils;
import com.lynx.tasm.utils.MatrixMathUtils;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class TransformProps {
  //  Dynamic transformStr;
  private float mTransformOriginX;
  private float mTransformOriginY;
  private LinkedHashMap<String, Float> mTransformPropsMap = new LinkedHashMap<>();

  public TransformProps() {
    reset();
  }

  private void setMatrixDecompositionContext(
      MatrixMathUtils.MatrixDecompositionContext matrixContext) {
    setTranslationX(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getTranslationX()));
    setTranslationY(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getTranslationY()));
    setTranslationZ(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getTranslationZ()));
    setRotation(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getRotation()));
    setRotationX(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getRotationX()));
    setRotationY(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getRotationY()));
    setScaleX(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getScaleX()));
    setScaleY(FloatUtils.sanitizeFloatPropertyValue((float) matrixContext.getScaleY()));
  }

  public void reset() {
    mTransformPropsMap.clear();
  }

  public float getTranslationX() {
    Float value = mTransformPropsMap.get(PropsConstants.TRANSLATE_X);
    return value != null ? value : 0;
  }

  public void setTranslationX(float translationX) {
    mTransformPropsMap.put(PropsConstants.TRANSLATE_X, translationX);
  }

  public float getTranslationY() {
    Float value = mTransformPropsMap.get(PropsConstants.TRANSLATE_Y);
    return value != null ? value : 0;
  }

  public void setTranslationY(float translationY) {
    mTransformPropsMap.put(PropsConstants.TRANSLATE_Y, translationY);
  }

  public void setTranslationZ(float translationZ) {
    mTransformPropsMap.put(PropsConstants.TRANSLATE_Z, translationZ);
  }

  public float getTranslationZ() {
    Float value = mTransformPropsMap.get(PropsConstants.TRANSLATE_Z);
    return value != null ? value : 0;
  }

  public float getRotation() {
    Float value = mTransformPropsMap.get(PropsConstants.ROTATE);
    return value != null ? value : 0;
  }

  public void setRotation(float rotation) {
    mTransformPropsMap.put(PropsConstants.ROTATE, rotation);
  }

  public float getRotationX() {
    Float value = mTransformPropsMap.get(PropsConstants.ROTATE_X);
    return value != null ? value : 0;
  }

  public void setRotationX(float rotationX) {
    mTransformPropsMap.put(PropsConstants.ROTATE_X, rotationX);
  }

  public float getRotationY() {
    Float value = mTransformPropsMap.get(PropsConstants.ROTATE_Y);
    return value != null ? value : 0;
  }

  public void setRotationY(float rotationY) {
    mTransformPropsMap.put(PropsConstants.ROTATE_Y, rotationY);
  }

  public float getScaleX() {
    Float value = mTransformPropsMap.get(PropsConstants.SCALE_X);
    return value != null ? value : 1;
  }

  public void setScaleX(float scaleX) {
    mTransformPropsMap.put(PropsConstants.SCALE_X, scaleX);
  }

  public float getScaleY() {
    Float value = mTransformPropsMap.get(PropsConstants.SCALE_Y);
    return value != null ? value : 1;
  }

  public void setScaleY(float scaleY) {
    mTransformPropsMap.put(PropsConstants.SCALE_Y, scaleY);
  }

  public float getSkewX() {
    Float value = mTransformPropsMap.get(PropsConstants.SKEW_X);
    return value != null ? value : 0;
  }

  public void setSkewX(float radians) {
    mTransformPropsMap.put(PropsConstants.SKEW_X, (float) Math.tan(radians));
  }

  public float getSkewY() {
    Float value = mTransformPropsMap.get(PropsConstants.SKEW_Y);
    return value != null ? value : 0;
  }

  public void setSkewY(float radians) {
    mTransformPropsMap.put(PropsConstants.SKEW_Y, (float) Math.tan(radians));
  }

  // TODO: This function is only used in flattenUI. FlattenUI's transform must be empty, these codes
  // can be deleted.
  public Matrix getTransformMatrix(float px, float py, Matrix mMatrix) {
    Matrix transformMatrix = mMatrix;
    for (Map.Entry<String, Float> entry : mTransformPropsMap.entrySet()) {
      String name = entry.getKey();
      float value = entry.getValue();

      switch (name) {
        case PropsConstants.TRANSLATE_X:
          transformMatrix.preTranslate(value, 0);
          break;
        case PropsConstants.TRANSLATE_Y:
          transformMatrix.preTranslate(0, value);
          break;
        case PropsConstants.SCALE_X:
          transformMatrix.preScale(value, 1, px, py);
          break;
        case PropsConstants.SCALE_Y:
          transformMatrix.preScale(1, value, px, py);
          break;
        case PropsConstants.ROTATE:
          transformMatrix.preRotate(value, px, py);
          break;
        default:
          break;
      }
    }

    return transformMatrix;
  }

  public float getTransformOriginX() {
    return mTransformOriginX;
  }

  public void setTransformOriginX(float transformOriginX) {
    this.mTransformOriginX = transformOriginX;
  }

  public float getTransformOriginY() {
    return mTransformOriginY;
  }

  public void setTransformOriginY(float transformOriginY) {
    this.mTransformOriginY = transformOriginY;
  }
  float GetRawLengthValue(TransformRaw transform, int index, float parentSize) {
    float result = 0;
    if (index == 0) {
      result = transform.getPlatformLengthP0().getValue(parentSize);
      if (transform.isP0Percent()) {
        result = Math.round(result);
      }
    } else if (index == 1) {
      result = transform.getPlatformLengthP1().getValue(parentSize);
      if (transform.isP1Percent()) {
        result = Math.round(result);
      }
    } else if (index == 2) {
      result = transform.getPlatformLengthP2().getValue(parentSize);
      if (transform.isP2Percent()) {
        result = Math.round(result);
      }
    }
    return result;
  }

  public static TransformProps processTransform(List<TransformRaw> transforms, float rootFontSize,
      float curFontSize, int rootWidth, int rootHeight, int width, int height) {
    TransformProps transformProps = new TransformProps();
    if (transforms == null || transforms.isEmpty()) {
      return transformProps;
    }
    for (TransformRaw transform : transforms) {
      switch (transform.getTransformType()) {
        case TRANSFORM_TRANSLATE:
        case TRANSFORM_TRANSLATE_3d:
          transformProps.setTranslationX(
              transformProps.GetRawLengthValue(transform, 0, (float) width));
          transformProps.setTranslationY(
              transformProps.GetRawLengthValue(transform, 1, (float) height));
          transformProps.setTranslationZ(transformProps.GetRawLengthValue(transform, 2, 0));
          break;
        case TRANSFORM_TRANSLATE_X:
          transformProps.setTranslationX(
              transformProps.GetRawLengthValue(transform, 0, (float) width));
          break;
        case TRANSFORM_TRANSLATE_Y:
          transformProps.setTranslationY(
              transformProps.GetRawLengthValue(transform, 0, (float) height));
          break;
        case TRANSFORM_TRANSLATE_Z:
          transformProps.setTranslationZ(transformProps.GetRawLengthValue(transform, 0, 0));
          break;
        case TRANSFORM_ROTATE:
        case TRANSFORM_ROTATE_Z:
          transformProps.setRotation(transform.getP0());
          break;
        case TRANSFORM_ROTATE_X:
          transformProps.setRotationX(transform.getP0());
          break;
        case TRANSFORM_ROTATE_Y:
          transformProps.setRotationY(transform.getP0());
          break;
        case TRANSFORM_SCALE:
          transformProps.setScaleX(transform.getP0());
          transformProps.setScaleY(transform.getP1());
          break;
        case TRANSFORM_SCALE_X:
          transformProps.setScaleX(transform.getP0());
          break;
        case TRANSFORM_SCALE_Y:
          transformProps.setScaleY(transform.getP0());
          break;
        case TRANSFORM_SKEW:
          transformProps.setSkewX((float) MatrixMathUtils.degreesToRadians(transform.getP0()));
          transformProps.setSkewY((float) MatrixMathUtils.degreesToRadians(transform.getP1()));
          break;
        case TRANSFORM_SKEW_X:
          transformProps.setSkewX((float) MatrixMathUtils.degreesToRadians(transform.getP0()));
          break;
        case TRANSFORM_SKEW_Y:
          transformProps.setSkewY((float) MatrixMathUtils.degreesToRadians(transform.getP0()));
          break;
        case TRANSFORM_MATRIX:
        case TRANSFORM_MATRIX_3d:
          Matrix4f transformMatrix3D = new Matrix4f(transform.getTransformRawData());
          MatrixMathUtils.MatrixDecompositionContext matrixDecompositionContext =
              new MatrixMathUtils.MatrixDecompositionContext();
          matrixDecompositionContext.reset();
          MatrixMathUtils.decomposeMatrix(
              TransformProps.convertFloatsToDoubles(transformMatrix3D.getArray()),
              matrixDecompositionContext);
          transformProps.setMatrixDecompositionContext(matrixDecompositionContext);
          break;
        default:
          LLog.DTHROW();
          return null;
      }
    }
    return transformProps;
  }

  public static TransformProps processTransformInOrder(List<TransformRaw> transforms,
      float rootFontSize, float curFontSize, int rootWidth, int rootHeight, int width, int height) {
    TransformProps transformProps = new TransformProps();
    if (transforms == null || transforms.isEmpty()) {
      return transformProps;
    }
    Matrix4f transformMatrix3D = new Matrix4f();
    for (TransformRaw transform : transforms) {
      switch (transform.getTransformType()) {
        case TRANSFORM_TRANSLATE:
        case TRANSFORM_TRANSLATE_3d: {
          float transX = transformProps.GetRawLengthValue(transform, 0, (float) width);
          float transY = transformProps.GetRawLengthValue(transform, 1, (float) height);
          float transZ = transformProps.GetRawLengthValue(transform, 2, 0);
          transformMatrix3D.translate(transX, transY, transZ);
          break;
        }
        case TRANSFORM_TRANSLATE_X: {
          float transX = transformProps.GetRawLengthValue(transform, 0, (float) width);
          transformMatrix3D.translate(transX, 0, 0);
          break;
        }
        case TRANSFORM_TRANSLATE_Y: {
          float transY = transformProps.GetRawLengthValue(transform, 0, (float) height);
          transformMatrix3D.translate(0, transY, 0);
          break;
        }
        case TRANSFORM_TRANSLATE_Z: {
          float transZ = transformProps.GetRawLengthValue(transform, 0, 0);
          transformMatrix3D.translate(0, 0, transZ);
          break;
        }
        case TRANSFORM_ROTATE:
        case TRANSFORM_ROTATE_Z: {
          transformMatrix3D.rotate(transform.getP0(), 0, 0, 1);
          break;
        }
        case TRANSFORM_ROTATE_X: {
          transformMatrix3D.rotate(transform.getP0(), 1, 0, 0);
          break;
        }
        case TRANSFORM_ROTATE_Y: {
          transformMatrix3D.rotate(transform.getP0(), 0, 1, 0);
          break;
        }
        case TRANSFORM_SCALE: {
          transformMatrix3D.scale(transform.getP0(), transform.getP1(), 1);
          break;
        }
        case TRANSFORM_SCALE_X: {
          transformMatrix3D.scale(transform.getP0(), 1, 1);
          break;
        }
        case TRANSFORM_SCALE_Y: {
          transformMatrix3D.scale(1, transform.getP0(), 1);
          break;
        }
        // TODO(renzhongyue): Support animation for skew.
        case TRANSFORM_SKEW: {
          transformProps.setSkewX((float) MatrixMathUtils.degreesToRadians(transform.getP0()));
          transformProps.setSkewY((float) MatrixMathUtils.degreesToRadians(transform.getP1()));
          break;
        }
        case TRANSFORM_SKEW_X: {
          transformProps.setSkewX((float) MatrixMathUtils.degreesToRadians(transform.getP0()));
          break;
        }
        case TRANSFORM_SKEW_Y: {
          transformProps.setSkewY((float) MatrixMathUtils.degreesToRadians(transform.getP0()));
          break;
        }
        case TRANSFORM_MATRIX:
        case TRANSFORM_MATRIX_3d: {
          transformMatrix3D.multiply(new Matrix4f(transform.getTransformRawData()));
          break;
        }
        default:
          LLog.DTHROW();
          return null;
      }
    }
    MatrixMathUtils.MatrixDecompositionContext matrixDecompositionContext =
        new MatrixMathUtils.MatrixDecompositionContext();
    matrixDecompositionContext.reset();
    MatrixMathUtils.decomposeMatrix(
        TransformProps.convertFloatsToDoubles(transformMatrix3D.getArray()),
        matrixDecompositionContext);
    transformProps.setMatrixDecompositionContext(matrixDecompositionContext);
    return transformProps;
  }

  public float toFinalValue(TransformRaw tr, float anchor) {
    if (tr.isP0Percent()) {
      return tr.getP0() * anchor;
    } else {
      return tr.getP0();
    }
  }

  public static TransformProps processTransformOrigin(
      TransformOrigin transformOrigin, float width, float height) {
    TransformProps transformOriginProps = new TransformProps();

    transformOriginProps.setTransformOriginX(width * 0.5f);
    transformOriginProps.setTransformOriginY(height * 0.5f);
    if (transformOrigin == null || transformOrigin == TransformOrigin.TRANSFORM_ORIGIN_DEFAULT) {
      return transformOriginProps;
    }

    if (transformOrigin.isP0Valid()) {
      float originTempX = transformOrigin.getP0();
      if (transformOrigin.isP0Percent()) {
        originTempX = width * originTempX;
      }
      transformOriginProps.setTransformOriginX(originTempX);
    }
    if (transformOrigin.isP1Valid()) {
      float originTempY = transformOrigin.getP1();
      if (transformOrigin.isP1Percent()) {
        originTempY = height * originTempY;
      }
      transformOriginProps.setTransformOriginY(originTempY);
    }
    return transformOriginProps;
  }

  public static double[] convertFloatsToDoubles(float[] input) {
    if (input == null) {
      return null;
    }
    double[] output = new double[input.length];
    for (int i = 0; i < input.length; i++) {
      output[i] = input[i];
    }
    return output;
  }
}
