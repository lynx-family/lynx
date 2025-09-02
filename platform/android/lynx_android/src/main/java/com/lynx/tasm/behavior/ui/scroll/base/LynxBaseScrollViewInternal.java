// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.scroll.base;

import android.view.MotionEvent;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;

public interface LynxBaseScrollViewInternal extends LynxBaseScrollViewPublic {
  boolean isBouncingForwards(int scrollOffset, int[] scrollRange);

  boolean isBouncingBackwards(int scrollOffset, int[] scrollRange);

  View getView();

  int getScrollY();

  int getScrollX();

  boolean isVertical();

  int[] dispatchScroll(
      int deltaX, int deltaY, int type, MotionEvent event, @Nullable int[] offsetInWindow);

  int[] getFlingRange(boolean isVertical);

  boolean startNestedScroll(@ViewCompat.ScrollAxis int axes, @ViewCompat.NestedScrollType int type);

  void stopNestedScroll(int type);

  int getNestedScrollAxes();
}
