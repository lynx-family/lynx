// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;

import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.view.View;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.utils.BorderRadius;

public class ScrollContainerDrawHelper {
  private Rect mUiBound = null;

  public void setUiBound(Rect bound) {
    mUiBound = bound;
  }

  public Path getClipPath(@NonNull final View scroller) {
    if (scroller == null) {
      return null;
    }
    Path clipPath = new Path();
    Drawable drawable = scroller.getBackground();
    final int scrollX = scroller.getScrollX();
    final int scrollY = scroller.getScrollY();
    final int width = scroller.getWidth();
    final int height = scroller.getHeight();
    if (drawable instanceof BackgroundDrawable) {
      BackgroundDrawable backgroundDrawable = (BackgroundDrawable) drawable;
      RectF borderWidth = backgroundDrawable.getDirectionAwareBorderInsets();
      BorderRadius borderCornerRadii = backgroundDrawable.getBorderRadius();
      Rect bounds = mUiBound == null ? drawable.getBounds() : mUiBound;
      RectF rect = new RectF(scrollX + bounds.left + borderWidth.left,
          scrollY + bounds.top + borderWidth.top, scrollX + bounds.right - borderWidth.right,
          scrollY + bounds.bottom - borderWidth.bottom);
      if (borderCornerRadii == null) {
        clipPath.addRect(rect, Path.Direction.CW);
      } else {
        float[] radiusArray = borderCornerRadii.getArray();
        radiusArray =
            BackgroundDrawable.RoundRectPath.newBorderRadius(radiusArray, borderWidth, 1.0f);
        clipPath.addRoundRect(rect, radiusArray, Path.Direction.CW);
      }
    } else {
      Rect bounds = mUiBound != null ? mUiBound : new Rect(0, 0, width, height);
      RectF rect = new RectF(scrollX + bounds.left, scrollY + bounds.top, scrollX + bounds.right,
          scrollY + bounds.bottom);
      clipPath.addRect(rect, Path.Direction.CW);
    }
    return clipPath;
  }
}
