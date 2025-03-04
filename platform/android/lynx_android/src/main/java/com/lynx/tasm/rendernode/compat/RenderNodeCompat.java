// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.rendernode.compat;

import android.graphics.Canvas;

public abstract class RenderNodeCompat {
  public RenderNodeCompat() {}

  private static boolean sEnable;

  public static void enable(boolean enable) {
    sEnable = enable;
  }

  public static boolean supportRenderNode() {
    return sEnable;
  }

  abstract public void init();

  public abstract boolean hasDisplayList();

  public abstract void setPosition(int left, int top, int right, int bottom);

  public abstract Canvas beginRecording(int width, int height);

  public abstract void drawRenderNode(Canvas canvas);
  public abstract void drawRenderNode(Canvas renderCanvas, Object renderNode);

  public abstract void endRecording(Canvas canvas);
  public abstract Object getRenderNode();
}
