// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Shader;
import androidx.annotation.NonNull;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;

public abstract class BackgroundGradientLayer extends BackgroundLayerDrawable {
  protected Shader mShader;
  protected int mWidth = 0, mHeight = 0;
  protected final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);

  protected int[] mColors = null;
  protected float[] mPositions = null;
  static protected float positionNotSet = -2;

  @Override
  public boolean isReady() {
    return true;
  }

  @Override
  public int getImageWidth() {
    return mWidth;
  }

  @Override
  public int getImageHeight() {
    return mHeight;
  }

  @Override
  public void onAttach() {}

  @Override
  public void onDetach() {}

  @Override
  public void onSizeChanged(int width, int height) {}

  public Shader getShader() {
    return mShader;
  }

  @Override
  public void draw(@NonNull Canvas canvas) {
    if (mShader == null) {
      LLog.e("gradient", "BackgroundGradientLayer.draw() must be called after setBounds()");
    }

    mPaint.setShader(mShader);
    if (getPathEffect() != null) {
      canvas.drawPath(getPathEffect(), mPaint);
    } else {
      canvas.drawRect(getBounds(), mPaint);
    }
  }

  protected void setColorAndStop(ReadableArray colors, ReadableArray stops) {
    if (stops.size() != 0 && colors.size() != stops.size()) {
      LLog.e("Gradient", "native parser error, color and stop must have same size");
      return;
    }

    mColors = new int[colors.size()];
    if (stops.size() == colors.size()) {
      mPositions = new float[stops.size()];
    }

    for (int i = 0; i < mColors.length; i++) {
      mColors[i] = (int) colors.getLong(i);
      if (mPositions != null) {
        mPositions[i] = (float) stops.getDouble(i) / 100.f;
      }
    }
  }
}
