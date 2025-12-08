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

public class Renderer {
  private final Rect mLynxFrame = new Rect();
  private final Point mRenderOffset = new Point();
  private final int mSign;
  private final PlatformRendererContext mPlatformRendererContext;
  private DisplayListApplier mDisplayListApplier = null;
  private final DisplayList mDisplayList = new DisplayList();
  private IRendererHost mRenderHost;

  public void setLynxFrame(int l, int t, int r, int b, int dx, int dy) {
    mLynxFrame.set(l + dx, t + dy, r + dx, b + dy);
    mRenderOffset.set(dx, dy);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      mRenderHost.getView().setClipBounds(new Rect(0, 0, mLynxFrame.width(), mLynxFrame.height()));
    }
  }

  public Point getRenderOffset() {
    return mRenderOffset;
  }

  public void setLynxFrame(int l, int t, int r, int b) {
    setLynxFrame(l, t, r, b, 0, 0);
  }

  public Rect getLynxFrame() {
    return mLynxFrame;
  }

  public Renderer(@NonNull PlatformRendererContext platformRendererContext, int sign) {
    mPlatformRendererContext = platformRendererContext;
    mSign = sign;
  }

  void setRenderHost(IRendererHost renderHost) {
    mRenderHost = renderHost;
  }

  int getSign() {
    return mSign;
  }

  public void onLayout(boolean changed, int l, int t, int r, int b) {
    ViewGroup view = mRenderHost.getView();
    for (int i = 0; i < view.getChildCount(); i++) {
      View child = view.getChildAt(i);
      if (child instanceof IRendererHost) {
        Rect childFrame = ((IRendererHost) child).getRenderer().getLynxFrame();
        child.layout(childFrame.left, childFrame.top, childFrame.right, childFrame.bottom);
      }
    }
  }

  public void onDraw(Canvas canvas) {
    mPlatformRendererContext.getDisplayList(mSign, mDisplayList);
    if (mDisplayListApplier == null) {
      mDisplayListApplier =
          new DisplayListApplier(mDisplayList, mPlatformRendererContext, mRenderHost.getView());
    } else {
      mDisplayListApplier.setDisplayList(mDisplayList);
    }
  }

  public void beforeDrawChild(Canvas canvas, View child) {
    mDisplayListApplier.drawTillNextView(canvas);
    canvas.save();
    if (child instanceof ContainerRenderer) {
      canvas.translate(-((ContainerRenderer) child).getRenderer().getRenderOffset().x,
          -((ContainerRenderer) child).getRenderer().getRenderOffset().y);
    }
  }

  public void afterDrawChild(Canvas canvas, View child) {
    canvas.restore();
  }

  public void afterDispatchDraw(Canvas canvas) {
    mDisplayListApplier.drawTillNextView(canvas);
    mDisplayListApplier.reset();
  }
}
