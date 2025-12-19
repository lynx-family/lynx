// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.render;

import android.graphics.Rect;
import android.graphics.RectF;

public class RoundedRectangle {
  private final RectF mRect;
  private final float[] mBorderRadii;

  public RoundedRectangle(RectF rect, float[] borderRadii) {
    mRect = rect;
    mBorderRadii = borderRadii;
  }

  public RectF getRectF() {
    return mRect;
  }

  public Rect getRect() {
    return new Rect((int) mRect.left, (int) mRect.top, (int) mRect.right, (int) mRect.bottom);
  }

  public boolean hasBorderRadius() {
    return mBorderRadii != null && mBorderRadii.length == 8;
  }

  public float[] getBorderRadii() {
    return mBorderRadii;
  }
}
