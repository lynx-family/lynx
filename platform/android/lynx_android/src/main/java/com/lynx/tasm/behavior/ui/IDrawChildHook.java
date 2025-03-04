// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import android.graphics.Canvas;
import android.graphics.Rect;
import android.view.View;

public interface IDrawChildHook {
  interface IDrawChildHookBinding {
    void bindDrawChildHook(IDrawChildHook hook);
  }
  void beforeDraw(Canvas canvas);
  void beforeDispatchDraw(Canvas canvas);

  void afterDispatchDraw(Canvas canvas);

  Rect beforeDrawChild(Canvas canvas, View child, long drawingTime);

  void afterDrawChild(Canvas canvas, View child, long drawingTime);

  int getChildDrawingOrder(int childCount, int index);

  boolean hasOverlappingRendering();

  void performLayoutChildrenUI();

  void performMeasureChildrenUI();

  void afterDraw(Canvas canvas);
}
