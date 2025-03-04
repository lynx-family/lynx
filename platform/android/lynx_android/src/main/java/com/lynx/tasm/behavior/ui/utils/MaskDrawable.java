// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.utils;

import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.PixelFormat;
import android.graphics.RectF;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.background.MaskLayerManager;

public class MaskDrawable extends LayerDrawable<MaskLayerManager> {
  private int mAlpha = 255;

  public MaskDrawable(LynxContext lynxContext, float curFontSize) {
    super(lynxContext, curFontSize);
  }

  @Override
  protected MaskLayerManager createLayerManager() {
    return new MaskLayerManager(mContext, this, mCurFontSize);
  }

  @Override
  public void draw(Canvas canvas) {
    drawMask(canvas);
  }

  public void drawMask(Canvas canvas) {
    if (mLayerManager.hasImageLayers()) {
      RectF borderRect = new RectF(getBounds());
      RectF paddingRect = new RectF(mPaddingBox);
      RectF contentRect = new RectF(mContentBox);
      RectF clipBox = borderRect;
      mLayerManager.draw(
          canvas, borderRect, paddingRect, contentRect, clipBox, null, null, mBorderWidth != null);
    }
  }

  @Override
  public void setAlpha(int alpha) {
    if (alpha != mAlpha) {
      mAlpha = alpha;
      invalidateSelf();
    }
  }

  @Override
  public int getAlpha() {
    return mAlpha;
  }

  @Override
  public void setColorFilter(ColorFilter cf) {
    // do nothing
  }

  @Override
  public int getOpacity() {
    return PixelFormat.TRANSLUCENT;
  }
}
