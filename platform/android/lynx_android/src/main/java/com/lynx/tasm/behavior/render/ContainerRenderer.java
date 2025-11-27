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
public class ContainerRenderer extends ViewGroup {
  private final Rect mLynxFrame = new Rect();
  public final Point mRenderOffset = new Point();
  private final int mSign;
  private final PlatformRendererContext mPlatformRendererContext;
  private DisplayListApplier mDisplayListApplier = null;
  private final DisplayList mDisplayList = new DisplayList();
  public void setLynxFrame(int l, int t, int r, int b, int dx, int dy) {
    mLynxFrame.set(l + dx, t + dy, r + dx, b + dy);
    mRenderOffset.set(dx, dy);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      setClipBounds(new Rect(0, 0, mLynxFrame.width(), mLynxFrame.height()));
    }
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
    setClipChildren(false);
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
    canvas.save();
    if (child instanceof ContainerRenderer) {
      canvas.translate(-((ContainerRenderer) child).mRenderOffset.x,
          -((ContainerRenderer) child).mRenderOffset.y);
    }
    boolean ret = super.drawChild(canvas, child, drawingTime);
    canvas.restore();
    return ret;
  }
  @Override
  protected void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);
    mDisplayListApplier.drawTillNextView(canvas);
    mDisplayListApplier.reset();
  }
}
