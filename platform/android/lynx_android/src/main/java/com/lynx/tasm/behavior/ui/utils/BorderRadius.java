// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.utils;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.utils.FloatUtils;
import java.util.Objects;

public class BorderRadius {
  public enum Location { TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT }

  static public class Corner {
    public PlatformLength x;
    public PlatformLength y;

    @Override
    public boolean equals(@Nullable Object obj) {
      if (obj == null) {
        return false;
      }
      Corner other = (Corner) obj;
      return Objects.equals(x, other.x) && Objects.equals(y, other.y);
    }

    public static Corner toCorner(ReadableArray ra, int startIndex) {
      BorderRadius.Corner radius = new BorderRadius.Corner();
      radius.x = new PlatformLength(ra.getDynamic(startIndex), ra.getInt(startIndex + 1));
      radius.y = new PlatformLength(ra.getDynamic(startIndex + 2), ra.getInt(startIndex + 3));
      return radius;
    }
  }

  private static final int ARR_SIZE = 8;
  private static final int CORNER_SIZE = 4;

  private float mWidth = 0, mHeight = 0;
  private @Nullable Corner[] mCornerRadii;
  private @Nullable float[] mCachedArray;

  public void clearCache() {
    mCachedArray = null;
  }

  public boolean updateSize(final float width, final float height) {
    final float newWidth = (width >= 0 && !MeasureUtils.isUndefined(width)) ? width : 0;
    final float newHeight = (height >= 0 && !MeasureUtils.isUndefined(height)) ? height : 0;
    if (FloatUtils.floatsEqual(newWidth, mWidth) && FloatUtils.floatsEqual(newHeight, mHeight)) {
      return mCachedArray == null;
    }

    mWidth = newWidth;
    mHeight = newHeight;
    mCachedArray = null;
    return true;
  }

  public boolean hasArray() {
    return mCachedArray != null;
  }

  public float[] getArray() {
    if (mCachedArray != null) {
      return mCachedArray;
    }

    mCachedArray = getBorderRadiusArrayOrDefaultTo();

    adjustBorderRadiusForBound();

    return mCachedArray;
  }

  public boolean hasRoundedBorders() {
    if (mCornerRadii != null) {
      for (final Corner borderRadii : mCornerRadii) {
        if (borderRadii == null || borderRadii.x == null)
          continue;

        if (!borderRadii.x.isZero() || !borderRadii.y.isZero()) {
          return true;
        }
      }
    }
    return false;
  }

  public boolean setCorner(int index, Corner corner) {
    if (index < 0 || index >= CORNER_SIZE)
      return false;

    if (mCornerRadii == null) {
      mCornerRadii = new Corner[CORNER_SIZE];
    }
    if (corner == null) {
      corner = new Corner();
    }

    if (!Objects.equals(corner, mCornerRadii[index])) {
      mCornerRadii[index] = corner;
      return true;
    }

    return false;
  }

  private float[] getBorderRadiusArrayOrDefaultTo() {
    float[] arr = new float[ARR_SIZE];
    if (mCornerRadii == null) {
      for (int i = 0; i < ARR_SIZE; ++i) {
        arr[i] = 0;
      }
      return arr;
    }

    // {Location.TOP_LEFT, Location.TOP_RIGHT, Location.BOTTOM_RIGHT, Location.BOTTOM_LEFT};
    for (int i = 0; i < CORNER_SIZE; ++i) {
      final Corner radius = mCornerRadii[i];
      final int xIndex = 2 * i, yIndex = 2 * i + 1;
      if (radius == null || radius.x == null) {
        arr[xIndex] = arr[yIndex] = 0;
        continue;
      }
      arr[xIndex] = radius.x.getValue(mWidth);
      arr[yIndex] = radius.y.getValue(mHeight);
    }
    return arr;
  }

  private void adjustBorderRadiusForBound() {
    if (FloatUtils.floatsEqual(mWidth, 0) || FloatUtils.floatsEqual(mHeight, 0)) {
      return;
    }

    float[] borderRadius = mCachedArray;

    float val = 1.0f, tmp = 1.0f;
    if (borderRadius[0] + borderRadius[2] > mWidth) {
      tmp = mWidth / (borderRadius[0] + borderRadius[2]);
      if (tmp < val)
        val = tmp;
    }
    if (borderRadius[4] + borderRadius[6] > mWidth) {
      tmp = mWidth / (borderRadius[4] + borderRadius[6]);
      if (tmp < val)
        val = tmp;
    }
    if (borderRadius[1] + borderRadius[7] > mHeight) {
      tmp = mHeight / (borderRadius[1] + borderRadius[7]);
      if (tmp < val)
        val = tmp;
    }
    if (borderRadius[3] + borderRadius[5] > mHeight) {
      tmp = mHeight / (borderRadius[3] + borderRadius[5]);
      if (tmp < val)
        val = tmp;
    }

    if (val < 1.0f) {
      for (int i = 0; i < 8; ++i) {
        borderRadius[i] *= val;
      }
    }
  }
}
