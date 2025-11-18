// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.SweepGradient;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.utils.PlatformLength;

public class BackgroundConicGradientLayer extends BackgroundGradientLayer {
  private double mAngle;
  private PlatformLength mCenterX;
  private PlatformLength mCenterY;

  public BackgroundConicGradientLayer(ReadableArray array) {
    if (array == null) {
      LLog.e("ConicGradient", "native parse error array is null");
      return;
    }

    // [angle, center, colors, stops]
    if (array.size() < 4) {
      LLog.e("ConicGradient", "native parse error, array.size must be 4  ");
      return;
    }

    mAngle = array.getDouble(0);

    ReadableArray center = array.getArray(1);
    mCenterX = new PlatformLength(center.getDynamic(0), center.getInt(1));
    mCenterY = new PlatformLength(center.getDynamic(2), center.getInt(3));

    setColorAndStop(array.getArray(2), array.getArray(3));
  }

  @Override
  public void setBounds(Rect bounds) {
    mWidth = Math.max(bounds.width(), 1);
    mHeight = Math.max(bounds.height(), 1);
    if (mColors == null || mColors.length < 2) {
      mShader = null;
    } else if (mPositions != null && mPositions.length != mColors.length) {
      mShader = null;
    } else {
      float x = mCenterX.getValue(mWidth);
      float y = mCenterY.getValue(mHeight);
      mShader = new SweepGradient(x, y, mColors, mPositions);
      double rotation = mAngle - 90;

      if (rotation != 0) {
        Matrix matrix = new Matrix();
        matrix.preRotate((float) rotation, x, y);
        mShader.setLocalMatrix(matrix);
      }
    }
    super.setBounds(bounds);
  }
}
