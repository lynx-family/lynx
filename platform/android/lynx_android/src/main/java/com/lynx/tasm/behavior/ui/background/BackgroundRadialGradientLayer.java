// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.utils.GradientUtils;

public class BackgroundRadialGradientLayer extends BackgroundGradientLayer {
  private static final String TAG = "RadialGradient";
  private static final int RADIAL_SHAPE_ELLIPSE = 0;
  private static final int RADIAL_SHAPE_CIRCLE = 1;
  private static final int RADIAL_SIZE_FARTHEST_CORNER = 0;
  private static final int RADIAL_SIZE_FARTHEST_SIDE = 1;
  private static final int RADIAL_SIZE_CLOSEST_CORNER = 2;
  private static final int RADIAL_SIZE_CLOSEST_SIDE = 3;
  private static final int RADIAL_SIZE_LENGTH = 4;

  private static final int RADIAL_CENTER_TYPE_PERCENTAGE = 11;
  private static final int RADIAL_CENTER_TYPE_RPX = 6;
  private static final int RADIAL_CENTER_TYPE_PX = 5;

  private int mShape = RADIAL_SHAPE_ELLIPSE;
  private int mShapeSize = RADIAL_SIZE_FARTHEST_CORNER;

  private int mCenterX = -StyleConstants.BACKGROUND_POSITION_CENTER;
  private int mCenterY = -StyleConstants.BACKGROUND_POSITION_CENTER;
  private float mCenterXValue = 0.5f;
  private float mCenterYValue = 0.5f;

  private float mShapeSizeXValue;
  private int mShapeSizeXUnit;
  private float mShapeSizeYValue;
  private int mShapeSizeYUnit;

  private PointF mAt = new PointF(0.5f, 0.5f);

  public BackgroundRadialGradientLayer(ReadableArray array) {
    if (array == null) {
      LLog.e(TAG, "native parser error, array is null");
      return;
    }

    if (array.size() != 3) {
      LLog.e(TAG, "native parser error, array.size must be 3");
    }

    ReadableArray shapeSizePosition = array.getArray(0);
    mShape = (int) shapeSizePosition.getLong(0);
    mShapeSize = (int) shapeSizePosition.getLong(1);
    // [x-position-type, x-position, y-position-type y-position]
    mCenterX = (int) shapeSizePosition.getLong(2);
    mCenterXValue = (float) shapeSizePosition.getDouble(3);
    mCenterY = (int) shapeSizePosition.getLong(4);
    mCenterYValue = (float) shapeSizePosition.getDouble(5);
    if (mShapeSize == RADIAL_SIZE_LENGTH) {
      mShapeSizeXValue = (float) shapeSizePosition.getDouble(10);
      mShapeSizeXUnit = shapeSizePosition.getInt(11);
      mShapeSizeYValue = (float) shapeSizePosition.getDouble(12);
      mShapeSizeYUnit = shapeSizePosition.getInt(13);
    }

    setColorAndStop(array.getArray(1), array.getArray(2));
  }

  @Override
  public void setBounds(Rect bounds) {
    mWidth = bounds.width();
    mHeight = bounds.height();
    calculateCenter();
    PointF radius;
    float centerX = mAt.x;
    float centerY = mAt.y;

    if (mShapeSize == RADIAL_SIZE_LENGTH) {
      float x = mShapeSizeXUnit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT
          ? mWidth * mShapeSizeXValue
          : mShapeSizeXValue;
      float y = mShapeSizeYUnit == StyleConstants.PLATFORM_LENGTH_UNIT_PERCENT
          ? mHeight * mShapeSizeYValue
          : mShapeSizeYValue;
      radius = new PointF(x, y);
    } else {
      radius = GradientUtils.getRadius(mShape, mShapeSize, centerX, centerY, mWidth, mHeight);
    }
    if (mColors == null || mColors.length < 2) {
      mShader = null;
    } else if (mPositions != null && mPositions.length != mColors.length) {
      mShader = null;
    } else {
      try {
        boolean hasZero = radius.x == 0 || radius.y == 0;
        float aspectRatio = hasZero ? 1 : radius.x / radius.y;

        // CSS can point center at view boundary, but Android can not allow radius be zero
        mShader = new RadialGradient(
            centerX, centerY, Math.max(radius.x, 1.f), mColors, mPositions, Shader.TileMode.CLAMP);
        if (aspectRatio != 1) {
          Matrix matrix = new Matrix();
          matrix.preScale(1, 1 / aspectRatio, centerX, centerY);
          mShader.setLocalMatrix(matrix);
        }
      } catch (Exception e) {
        mShader = null;
        e.printStackTrace();
        LLog.w("BackgroundRadialGradientLayer", "exception:\n" + e.toString());
      }
    }
    super.setBounds(bounds);
  }

  private void calculateCenter() {
    mAt.x = calculateValue(mCenterX, mCenterXValue, mWidth);
    mAt.y = calculateValue(mCenterY, mCenterYValue, mHeight);
  }

  private float calculateValue(int type, float value, float base) {
    switch (type) {
      case -StyleConstants.BACKGROUND_POSITION_CENTER:
        return base * 0.5f;
      case -StyleConstants.BACKGROUND_POSITION_LEFT:
      case -StyleConstants.BACKGROUND_POSITION_TOP:
        return 0.f;
      case -StyleConstants.BACKGROUND_POSITION_RIGHT:
      case -StyleConstants.BACKGROUND_POSITION_BOTTOM:
        return base;
      case RADIAL_CENTER_TYPE_PERCENTAGE:
        return base * value / 100.f;
      default:
        // TODO handle REM RPX or other length type
        return value;
    }
  }
}
