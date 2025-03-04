// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.Matrix;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import java.util.Objects;

public class LynxScaleTypeDrawable extends Drawable {
  ScalingUtils.ScaleType mScaleType;
  Object mScaleTypeState;

  int mUnderlyingWidth = 0;
  int mUnderlyingHeight = 0;

  Matrix mDrawMatrix;

  private Matrix mTempMatrix = new Matrix();

  private Drawable mCurrentDelegate;

  String mCapInsets;
  String mCapInsetsScale;

  public LynxScaleTypeDrawable(Drawable drawable, ScalingUtils.ScaleType scaleType) {
    super();
    mCurrentDelegate = drawable;
    mScaleType = scaleType;
  }

  public void setCurrent(Drawable newDelegate) {
    mCurrentDelegate = newDelegate;
    configureBounds();
  }

  public void setCapInsets(String capInsets, String capInsetsScale) {
    mCapInsets = capInsets;
    mCapInsetsScale = capInsetsScale;
  }

  public ScalingUtils.ScaleType getScaleType() {
    return mScaleType;
  }

  public void setScaleType(ScalingUtils.ScaleType scaleType) {
    if (Objects.equals(mScaleType, scaleType)) {
      return;
    }
    mScaleType = scaleType;
    mScaleTypeState = null;
    configureBounds();
    invalidateSelf();
  }

  @Override
  public void draw(Canvas canvas) {
    if (mCurrentDelegate != null) {
      configureBoundsIfUnderlyingChanged();
      if (!TextUtils.isEmpty(mCapInsets) && mCurrentDelegate instanceof BitmapDrawable) {
        Bitmap bitmap = ((BitmapDrawable) mCurrentDelegate).getBitmap();
        if (bitmap != null) {
          NinePatchHelper.drawNinePatch(getBounds().width(), getBounds().height(),
              bitmap.getWidth(), bitmap.getHeight(), mScaleType, mCapInsets, mCapInsetsScale,
              canvas, ((BitmapDrawable) mCurrentDelegate).getBitmap());
          return;
        }
      }
      if (mDrawMatrix != null) {
        int saveCount = canvas.save();
        canvas.clipRect(getBounds());
        canvas.concat(mDrawMatrix);
        mCurrentDelegate.draw(canvas);
        canvas.restoreToCount(saveCount);
      } else {
        mCurrentDelegate.draw(canvas);
      }
    }
  }

  @Override
  public void setAlpha(int alpha) {
    if (mCurrentDelegate != null) {
      mCurrentDelegate.setAlpha(alpha);
    }
  }

  @Override
  public void setColorFilter(@Nullable ColorFilter colorFilter) {
    if (mCurrentDelegate != null) {
      mCurrentDelegate.setColorFilter(colorFilter);
    }
  }

  @Override
  public int getOpacity() {
    if (mCurrentDelegate == null) {
      return PixelFormat.UNKNOWN;
    }
    return mCurrentDelegate.getOpacity();
  }

  @Override
  protected void onBoundsChange(Rect bounds) {
    if (getCurrent() != null) {
      configureBounds();
    }
  }

  private void configureBoundsIfUnderlyingChanged() {
    boolean underlyingChanged = mUnderlyingWidth != getCurrent().getIntrinsicWidth()
        || mUnderlyingHeight != getCurrent().getIntrinsicHeight();
    if (underlyingChanged) {
      configureBounds();
    }
  }

  @Override
  public Drawable getCurrent() {
    return mCurrentDelegate;
  }

  void configureBounds() {
    Drawable underlyingDrawable = getCurrent();
    Rect bounds = getBounds();
    int viewWidth = bounds.width();
    int viewHeight = bounds.height();
    int underlyingWidth = mUnderlyingWidth = underlyingDrawable.getIntrinsicWidth();
    int underlyingHeight = mUnderlyingHeight = underlyingDrawable.getIntrinsicHeight();

    if (underlyingWidth <= 0 || underlyingHeight <= 0) {
      underlyingDrawable.setBounds(bounds);
      mDrawMatrix = null;
      return;
    }

    if (underlyingWidth == viewWidth && underlyingHeight == viewHeight) {
      underlyingDrawable.setBounds(bounds);
      mDrawMatrix = null;
      return;
    }

    if (mScaleType == ScalingUtils.ScaleType.FIT_XY) {
      underlyingDrawable.setBounds(bounds);
      mDrawMatrix = null;
      return;
    }

    underlyingDrawable.setBounds(0, 0, underlyingWidth, underlyingHeight);
    mScaleType.getTransform(mTempMatrix, bounds, underlyingWidth, underlyingHeight, 0.5f, 0.5f);
    mDrawMatrix = mTempMatrix;
  }
}
