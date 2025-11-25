// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.behavior.LynxContext;
public class ContainerRenderer extends ViewGroup {
  private final Rect mLynxFrame = new Rect();
  private final int mSign;
  private final PlatformRendererContext mPlatformRendererContext;
  private DisplayListApplier mDisplayListApplier = null;
  private final DisplayList mDisplayList = new DisplayList();
  public void setLynxFrame(int l, int t, int r, int b) {
    mLynxFrame.set(l, t, r, b);
  }
  public Rect getLynxFrame() {
    return mLynxFrame;
  }
  public ContainerRenderer(
      LynxContext context, @NonNull PlatformRendererContext platformRendererContext, int sign) {
    super(context);
    mPlatformRendererContext = platformRendererContext;
    mSign = sign;
    setWillNotDraw(false);
  }
  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    for (int i = 0; i < getChildCount(); i++) {
      View child = getChildAt(i);
      if (child instanceof ContainerRenderer) {
        Rect childFrame = ((ContainerRenderer) child).getLynxFrame();
        child.layout(childFrame.left, childFrame.top, childFrame.right, childFrame.bottom);
      }
    }
  }
  @Override
  protected void onDraw(Canvas canvas) {
    mPlatformRendererContext.getDisplayList(mSign, mDisplayList);
    if (mDisplayListApplier == null) {
      mDisplayListApplier = new DisplayListApplier(mDisplayList, mPlatformRendererContext, this);
    } else {
      mDisplayListApplier.setDisplayList(mDisplayList);
    }
  }
  @Override
  protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
    mDisplayListApplier.drawTillNextView(canvas);
    return super.drawChild(canvas, child, drawingTime);
  }
  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    mDisplayListApplier.drawTillNextView(canvas);
    mDisplayListApplier.reset();
  }
}
