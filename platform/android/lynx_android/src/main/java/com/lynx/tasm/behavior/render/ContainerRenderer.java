// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;
import android.graphics.Canvas;
import android.graphics.Point;
import android.graphics.Rect;
import android.os.Build;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.LynxContext;

public class ContainerRenderer extends ViewGroup implements IRendererHost {
  @Override
  public Renderer createRenderer(PlatformRendererContext platformRendererContext, int sign) {
    return new Renderer(platformRendererContext, sign);
  }

  private Renderer mRenderer;

  @Override
  public void setRenderer(Renderer renderer) {
    mRenderer = renderer;
  }

  @Override
  public Renderer getRenderer() {
    return mRenderer;
  }

  @Override
  public ViewGroup getView() {
    return this;
  }

  public ContainerRenderer(LynxContext context) {
    super(context);
    setWillNotDraw(false);
    setClipChildren(false);
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    mRenderer.onLayout(changed, l, t, r, b);
  }

  @Override
  protected void onDraw(Canvas canvas) {
    mRenderer.onDraw(canvas);
  }

  @Override
  protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
    mRenderer.beforeDrawChild(canvas, child);
    boolean ret = super.drawChild(canvas, child, drawingTime);
    mRenderer.afterDrawChild(canvas, child);
    return ret;
  }

  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    mRenderer.afterDispatchDraw(canvas);
  }
}
