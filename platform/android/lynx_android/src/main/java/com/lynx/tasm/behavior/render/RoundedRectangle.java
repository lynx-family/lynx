// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.render;

import android.graphics.Rect;
import android.graphics.RectF;
import androidx.annotation.Nullable;
import java.util.Arrays;

public class RoundedRectangle {
  private final RectF mRect;
  private final float[] mBorderRadii;

  private boolean mHasBorderRadius = false;

  public RoundedRectangle(RectF rect, float[] borderRadii) {
    mRect = rect;
    mBorderRadii = borderRadii;
    mHasBorderRadius = hasNonZero(mBorderRadii);
  }

  private boolean hasNonZero(float[] array) {
    if (array == null) {
      return false;
    }

    // The length must be 8 since the rectangle have eight radius.
    if (array.length != 8) {
      return false;
    }

    for (float v : array) {
      if (v > 0.0f) {
        return true;
      }
    }
    return false;
  }

  public RectF getRectF() {
    return mRect;
  }

  public Rect getRect() {
    return new Rect((int) mRect.left, (int) mRect.top, (int) mRect.right, (int) mRect.bottom);
  }

  public boolean hasBorderRadius() {
    return mHasBorderRadius;
  }

  public float[] getBorderRadii() {
    return mBorderRadii;
  }

  public float getTopLeftRadiusX() {
    return mBorderRadii != null ? mBorderRadii[0] : 0.0f;
  }

  public float getTopLeftRadiusY() {
    return mBorderRadii != null ? mBorderRadii[1] : 0.0f;
  }

  public float getTopRightRadiusX() {
    return mBorderRadii != null ? mBorderRadii[2] : 0.0f;
  }

  public float getTopRightRadiusY() {
    return mBorderRadii != null ? mBorderRadii[3] : 0.0f;
  }

  public float getBottomLeftRadiusX() {
    return mBorderRadii != null ? mBorderRadii[4] : 0.0f;
  }

  public float getBottomLeftRadiusY() {
    return mBorderRadii != null ? mBorderRadii[5] : 0.0f;
  }

  public float getBottomRightRadiusX() {
    return mBorderRadii != null ? mBorderRadii[6] : 0.0f;
  }

  public float getBottomRightRadiusY() {
    return mBorderRadii != null ? mBorderRadii[7] : 0.0f;
  }

  @Override
  public boolean equals(@Nullable Object obj) {
    if (this == obj)
      return true;

    if (!(obj instanceof RoundedRectangle)) {
      return false;
    }

    RoundedRectangle other = (RoundedRectangle) obj;

    if (mRect != null && mBorderRadii != null && hasBorderRadius() && other.hasBorderRadius()) {
      return mRect.equals(other.mRect) && Arrays.equals(mBorderRadii, other.mBorderRadii);
    }

    if (mRect == null && other.mRect == null && hasBorderRadius() && other.hasBorderRadius()) {
      return Arrays.equals(mBorderRadii, other.mBorderRadii);
    }

    if (mRect != null && other.mRect != null && !hasBorderRadius() && !other.hasBorderRadius()) {
      return mRect.equals(other.mRect);
    }

    return mRect == null && other.mRect == null && !hasBorderRadius() && !other.hasBorderRadius();
  }
}
