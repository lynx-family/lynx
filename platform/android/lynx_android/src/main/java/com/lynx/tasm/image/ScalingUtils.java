// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.graphics.Matrix;
import android.graphics.Rect;
import com.lynx.tasm.utils.PixelUtils;

/** Performs scale type calculations. */
public class ScalingUtils {
  public interface ScaleType {
    ScaleType FIT_XY = ScaleTypeFitXY.INSTANCE;

    ScaleType FIT_CENTER = ScaleTypeFitCenter.INSTANCE;

    ScaleType CENTER = ScaleTypeCenter.INSTANCE;

    ScaleType CENTER_CROP = ScaleTypeCenterCrop.INSTANCE;

    Matrix getTransform(Matrix outTransform, Rect parentBounds, int childWidth, int childHeight,
        float focusX, float focusY);
  }

  public static abstract class AbstractScaleType implements ScaleType {
    @Override
    public Matrix getTransform(Matrix outTransform, Rect parentRect, int childWidth,
        int childHeight, float focusX, float focusY) {
      getTransformImpl(outTransform, parentRect.width(), parentRect.height(), childWidth,
          childHeight, parentRect.left, parentRect.top);
      return outTransform;
    }

    public abstract void getTransformImpl(Matrix matrix, float availableWidth,
        float availableHeight, float sourceWidth, float sourceHeight, float offsetStartX,
        float offsetStartY);
  }

  private static class ScaleTypeFitXY extends AbstractScaleType {
    public static final ScaleType INSTANCE = new ScaleTypeFitXY();

    @Override
    public String toString() {
      return "fit_xy";
    }

    // Scale to fix the width and height
    @Override
    public void getTransformImpl(Matrix matrix, float availableWidth, float availableHeight,
        float sourceWidth, float sourceHeight, float offsetStartX, float offsetStartY) {
      matrix.setScale(availableWidth / sourceWidth, availableHeight / sourceHeight);
      matrix.postTranslate(offsetStartX, offsetStartY);
    }
  }

  private static class ScaleTypeFitCenter extends AbstractScaleType {
    public static final ScaleType INSTANCE = new ScaleTypeFitCenter();

    @Override
    public String toString() {
      return "fit_center";
    }

    // According to the width and height factor to shrink the image displaying
    // center
    @Override
    public void getTransformImpl(Matrix matrix, float availableWidth, float availableHeight,
        float sourceWidth, float sourceHeight, float offsetStartX, float offsetStartY) {
      float w_rate = availableWidth / sourceWidth;
      float h_rate = availableHeight / sourceHeight;
      if (w_rate > h_rate) {
        float finalWidth = sourceWidth * h_rate;
        float startW = (availableWidth - finalWidth) / 2 + offsetStartX;
        matrix.setScale(h_rate, h_rate);
        matrix.postTranslate(startW, offsetStartY);
      } else {
        float finalHeight = sourceHeight * w_rate;
        float startH = (availableHeight - finalHeight) / 2 + offsetStartY;
        matrix.setScale(w_rate, w_rate);
        matrix.postTranslate(offsetStartX, startH);
      }
    }
  }

  private static class ScaleTypeCenter extends AbstractScaleType {
    public static final ScaleType INSTANCE = new ScaleTypeCenter();

    @Override
    public String toString() {
      return "center";
    }

    // Center bitmap in view, and scaling.
    // same to browser and ios, use source image w/h as dip
    @Override
    public void getTransformImpl(Matrix matrix, float availableWidth, float availableHeight,
        float sourceWidth, float sourceHeight, float offsetStartX, float offsetStartY) {
      final float sourceSizeScale = PixelUtils.dipToPx(1.0f);
      sourceWidth *= sourceSizeScale;
      sourceHeight *= sourceSizeScale;
      float coordinateX = Math.round((availableWidth - sourceWidth) * 0.5f);
      float coordinateY = Math.round((availableHeight - sourceHeight) * 0.5f);
      matrix.setScale(sourceSizeScale, sourceSizeScale);
      matrix.postTranslate(coordinateX, coordinateY);
    }
  }

  private static class ScaleTypeCenterCrop extends AbstractScaleType {
    public static final ScaleType INSTANCE = new ScaleTypeCenterCrop();
    @Override
    public String toString() {
      return "center_crop";
    }

    // According to the width and height factor to enlarge the image displaying
    // center
    @Override
    public void getTransformImpl(Matrix matrix, float availableWidth, float availableHeight,
        float sourceWidth, float sourceHeight, float offsetStartX, float offsetStartY) {
      float w_rate = availableWidth / sourceWidth;
      float h_rate = availableHeight / sourceHeight;
      if (w_rate > h_rate) {
        float finalHeight = sourceHeight * w_rate;
        float translateX = offsetStartX;
        float translateY = (availableHeight - finalHeight) / 2 + offsetStartY;
        matrix.setScale(w_rate, w_rate);
        matrix.postTranslate(translateX, translateY);
      } else {
        float finalWidth = sourceWidth * h_rate;
        float translateX = (availableWidth - finalWidth) / 2 + offsetStartX;
        float translateY = offsetStartY;
        matrix.setScale(h_rate, h_rate);
        matrix.postTranslate(translateX, translateY);
      }
    }
  }
}
